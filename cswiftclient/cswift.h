#include <stdbool.h>

/*
 * cswift provides a C interface to Swift.
 * 
 * It manages the sockets, sends the HTTP request and parses
 * the HTTP response.
 */
typedef struct range_s {
    int start;      // Starting offset
    int count;      // Number of bytes
    char *buf;
    int buf_len;    // For GET, length of buf response
} range_t;

typedef struct tag_s {
    char *key;
    char *vals;
} tag_t;

typedef struct headers_s {
    int count;
    int free_count;
    char *rbuf; // Original read buffer that must be freed
    tag_t tags[];
} header_t;

// Dump all headers
void print_headers(header_t *headers);

// Free headers and associated memory
void free_headers(header_t **headers);

// Find the value of the given header
char *find_value(header_t *headers, char *header);

// Called when process starts to open/connect free_count
// sockets to server.
// TODO -
void init_fds(char *server, int port, int free_count);

// Grab an fd from the free pool.  fd will have already
// connected to server during init_fds().
int get_fd();

// Release an fd back to the free pool
// TODO -
void release_fd(int fd);

int get_auth_token(int fd, char *path, char *usr, char *passwd, char **auth_token);

int head(int fd, char *path, char *auth_token, header_t **headers);
int post(int fd, char *path, char *auth_token, header_t *headers);

// Send a GET request
// Path must be of the form:
//    "<account>/<container>/<object>"
// range_index is the entry in ranges for the GET
int get_request(int fd, char *path, char *auth_token, range_t *ranges, int range_index);

// Receive a the response for a GET request.
int get_response(int fd, header_t **headers, range_t *ranges, int range_index);

int put(int fd, char *path, char *auth_token, header_t *headers, char *body, int body_len, bool is_chunked);