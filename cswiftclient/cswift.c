#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "socket.h"
#include "cswift.h"

void form_get(char *path, char *ip, int port, range_t *ranges, int range_index,
              char **buf, int *buf_len);
int read_headers(int fd, header_t **headers, int *bytes_read);
int read_body(int fd, header_t *headers, range_t *ranges, int range_index);

// TODO - Not implemented
int
get_auth_token(int fd, char *path, char *usr, char *passwd, char **auth_token) {
    *auth_token = NULL;
    return 0;
}

// TODO - change to use pool
int
get_fd() {
    return sock_open("127.0.0.1", 8090);
}

// TODO - not implemented.
void
release_fd(int fd) {
}

// TODO - not implemented
// Initializes FDs/sockets to server
// Opens free_count sockets, does connect()s, etc
void
init_fds(char *server, int port, int free_count) {
}

// Return a valid HTTP GET request
void
form_get(char *path, char *ip, int port, range_t *ranges, int range_index, char **buf, int *buf_len) {
    *buf_len =
        strlen("GET /v1/%s HTTP/1.1\r\nHost: 127.0.0.1:8090\r\nUser-Agent: ProxyFS\r\nRange: bytes=%d-%d\r\n\r\n");

    // TODO - remove 100 fudge and calculate string lengths of bytes arguments.
    *buf_len = *buf_len + strlen(path) + sizeof(int) + sizeof(int) + 1 + 100;

    *buf = (char *) malloc(*buf_len);
    memset(*buf, 0, *buf_len); // TODO - remove once remove fudge above
                               // - avoids valgrind uninitialized error

    // Send request
    *buf[0] = '\0';
    sprintf(*buf,
        "GET /v1/%s HTTP/1.1\r\nHost: 127.0.0.1:8090\r\nUser-Agent: ProxyFS\r\nRange: bytes=%d-%d\r\n\r\n",
        path, ranges[0].start, (ranges[range_index].start + ranges[range_index].count));
}

// Send an HTTP GET request to swift and don't wait for the response
int
get_request(int fd, char *path, char *auth_token, range_t *ranges, int range_index) {
    if (fd <= 0) {
        return ENOENT;
    }

    char *buf = NULL;
    int buf_len = 0;
    form_get(path, "127.0.0.1", 8090, ranges, range_index, &buf, &buf_len);

    int bytes_written = sock_write(fd, buf, buf_len);

    // TODO - need error handling and return error!!

    free(buf);
    return 0;
}

// Receive the response from an HTTP GET
// Setup ranges[range_index] entry with buf, etc.
int
get_response(int fd, header_t **headers, range_t *ranges, int range_index) {
    int bytes_read;

    // First read the response headers.
    int error = read_headers(fd, headers, &bytes_read);
    if (error) {
        return error;
    }

    // TODO - better check??? Other status codes?
    if ((strcmp("200 OK", find_value(*headers, "HTTP/1.1")) != 0) &&
        (strcmp("206 Partial Content", find_value(*headers, "HTTP/1.1")) != 0)) {
        printf("RETURN WAS: %s\n", find_value(*headers, "HTTP/1.1"));
        error = EIO;
        return error;
    }

    // TODO - handle multipart response
    // Read body directly into buf passed from VFS
    error = read_body(fd, *headers, ranges, range_index);

    return error;
}

// Read the body length and the body from the socket and put into range
// entry.
//
// We already read all the headers from the socket.  Rest is just the body.
int
read_body(int fd, header_t *headers, range_t *ranges, int range_index) {

    if (fd <= 0) {
        return ENOENT;
    }

    if (find_value(headers, "Content-Length") == NULL)  {
        // NOTE: This may happen if we get a chunked response to a GET.
        //
        // We do not think that Swift does this but it is something to be
        // aware of.
        printf("ERROR: read_body() returned NULL!");
        return EIO;
    }
    int content_length = atoi(find_value(headers, "Content-Length"));

    int bytes_read = 0;
    int total_bytes_read = 0;
    while (1) {
        bytes_read = read(fd, (ranges[range_index].buf + total_bytes_read),
            content_length);
        if (bytes_read < 0) {
            if (errno == EAGAIN) {
                continue;
            }

            // TODO - assume we will die here if problem talking to 
            // Swift proxy server in any case other than rcnt > 0?
        }

        total_bytes_read += bytes_read;
        if (total_bytes_read == content_length) {
            break;
        }
    }

    if (total_bytes_read != content_length) {
        printf("%s(): total_bytes_read: %d content_length: %d !!!!!!!\n",
            __FUNCTION__, total_bytes_read, content_length);
    }
    ranges[range_index].buf_len = total_bytes_read;

    return 0;
}
    

void
print_headers(header_t *headers) {
    printf("headers->free_count: %d headers->count: %d\n", headers->free_count,
        headers->count);

    int i;
    for (i = 0; i < headers->count; i++) {
        printf("headers->tags[%d].key: %s - headers->tags[%d].vals: %s\n", i,
            headers->tags[i].key, i, headers->tags[i].vals);
    }
}

// Free headers data structure and associated memory
void
free_headers(header_t **headers) {
    free((*headers)->rbuf);
    free(*headers);
    *headers = NULL;
}

char *
find_value(header_t *headers, char *h) {
    int i;
    for (i = 0; i < headers->count; i++) {
        if (strcmp(headers->tags[i].key, h) == 0) {
            return headers->tags[i].vals;
        }
    }
    return NULL;
}

// Add this new header to headers.
void
add_header(header_t *headers, char *h) {

    // TODO - do remalloc() if free_count == 0
    headers->free_count -= 1;
    int index = headers->count;
    headers->count += 1;
    
    headers->tags[index].key = h;
}

// Add/Replace value of header indexed by headers->count
void
add_value(header_t *headers, char *v) {
    int index = headers->count - 1;
    headers->tags[index].vals = v;
}

// Read all of the headers off the socket and put into header_t struct.
//
// NOTE: While reading this code, it is important to understand the response
// buffer looks like this example response for a GET:
//      "HTTP/1.1 200 OK\r\nContent-Length: 16\r\nAccept-Ranges: bytes\r\n"
//      "Last-Modified: Tue, 27 Feb 2018 22:10:58 GMT\r\n"
//      "Etag: 6d21ca1eb0f2e97dec7007e243f8c91c\r\nX-Timestamp: 1519769457.25783\r\n"
//      "Content-Type: application/octet-stream\r\nX-Trans-Id: txd4b308ca61584416b6901-005a95d775"
//      "\r\nX-Openstack-Request-Id: txd4b308ca61584416b6901-005a95d775\r\nDate: Tue, 27 Feb 2018"
//      " 22:11:01 GMT\r\n\r\nthis is CHUNK #1"
//
// read_headers() reads everything from the socket up to the body.  The body in the above
// example is "this is CHUNK #1".
int
read_headers(int fd, header_t **headers, int *total_bytes_read) {

    if (fd <= 0) {
        return ENOENT;
    }

    int rcnt;                   // Number of bytes currently read
    int rbuf_pos;                // Current position within rbuf
    int cr_cnt, lf_cnt;         // Count of \r and \n seen
    bool already_reading_value = false; // True if we are in the middle of reading
                                // the value of a header since a value can
                                // include delimiters like ":".
    bool wait_head = true;      // Waiting to see first byte of header
    bool wait_value = false;    // Waiting to see first byte of value
    bool first_value = true;    // The first value is not delimited by a :.
                                // Therefore, set this flag so we correctly
                                // pickup header - "HTTP/1.1" and value "200 OK".
    char *rbuf = malloc(1024);  // Buffer used to read data from socket.

    // TODO - remalloc rbuf if too small?
    // TODO - how do we free rbuf since headers points to it's contents?
    //        probably should be doing strdup()?

    // Initially, create room for 20 header entries.
    // TODO - cleaner way? remalloc()???

    // Want caller to be able to just do "free(headers);"
    int h_entries = 20;
    int h_size = sizeof(header_t) + sizeof(tag_t *) + (sizeof(tag_t) * h_entries);
    *headers = malloc(h_size);
    memset(*headers, 0, h_size);
    (*headers)->free_count = 20;
    (*headers)->rbuf = rbuf;
    
    // We read the headers 1 byte at a time
    *total_bytes_read = 0;
    while (1) {
        rcnt = read(fd, (rbuf + *total_bytes_read), 1);
        if (rcnt < 0) {
            if (errno == EAGAIN) {
                continue;
            }

            // TODO - assume we will die here if problem talking to 
            // Swift proxy server in any case other than rcnt > 0?
        }

        rbuf_pos = *total_bytes_read;
        *total_bytes_read += rcnt;

        // We hit the end of this header.  Wait for value.
        //
        // This is only true if the value does not include a ":"
        // which headers like Last-Modified do.
        if (rbuf[rbuf_pos] == ':') {
            if (!already_reading_value) {
                rbuf[rbuf_pos] = '\0';
                wait_value = true;
            }
            continue;
        }

        // We are at the end of a header or end of all headers.
        if (rbuf[rbuf_pos] == '\r') {
            rbuf[rbuf_pos] = '\0';
            cr_cnt++;
            already_reading_value = false;
            continue;
        }

        // We are waiting for either another header or
        // the end of the headers.
        if (rbuf[rbuf_pos] == '\n') {
            lf_cnt++;
            wait_head = true;
            already_reading_value = false;

            // If we have reached the end of the headers we
            // will see \r\n\r\n.  Break here since the rest
            // is the body of the message.
            if ((cr_cnt == 2) && (lf_cnt == 2)) {
                break;
            }
        } else {
            // Now we are either reading header, value or " ".
            cr_cnt = 0;
            lf_cnt = 0;

            if (rbuf[rbuf_pos] == ' ') {
                if (first_value) {
                    rbuf[rbuf_pos] = '\0';
                    wait_value = true;
                    first_value = false;
                }
            } else if (wait_head) {
                    add_header(*headers, &rbuf[rbuf_pos]);
                    wait_head = false;
                } else {
                    if (wait_value) {
                        add_value(*headers, &rbuf[rbuf_pos]);
                        already_reading_value = true;
                        wait_value = false;
                    } else {
                        // The current character is from a header
                        // or a value but is not the first character.
                        continue;
                    }
            }
        }
    }

    // rbuf will be freed with call to free_headers()
    return 0;
}