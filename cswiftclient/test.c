#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include "cswift.h"
#include "socket.h"

// Flag to control debug prints. Defaulted to off for now.
int debug_flag = 0;

int
main(int argc, char *argv[])
{
    // Setup connection to Swift via the NO_AUTH port
    int fd = get_fd();
    if (fd <= 0) {
        printf("Unable to open socket to Swift!\n");
        exit(-1);
    }

    // Setup sample path for object in read plan
    char *a = "AUTH_CommonVolume";
    char *c = "Bob-Container";
    char *o = "bob-object";
    int len = strlen(a) + strlen(c) + strlen(o) + 2 + 1;
    char *path = malloc(len);
    sprintf(path, "%s/%s/%s", a, c, o);

    // Setup sample read plan range
    range_t ranges[1];
    memset(ranges, 0, sizeof(range_t));
    ranges[0].start = 0;
    ranges[0].count = 100;
    ranges[0].buf = malloc(ranges[0].count);
    memset(ranges[0].buf, 0, ranges[0].count);
    ranges[0].buf[0] = '\0';

    // Send the GET
    int err = get_request(fd, path, NULL, ranges, 0);
    free(path);

    // Now retrieve the response
    header_t *headers = NULL;
    char *body = NULL;
    int body_len;
    // NOTE: We are simulating getting the response for read plan entry #1
    err = get_response(fd, &headers, ranges, 0 /* range_index */ );

    // Dump headers returned
    print_headers(headers);
    printf("find HTTP/1.1 value: %s\n", find_value(headers, "HTTP/1.1"));
    printf("find Last-Modified value: %s\n", find_value(headers, "Last-Modified"));
    free_headers(&headers);

    printf("%s() - body returned is: %s\n", __FUNCTION__, ranges[0].buf);
    free(ranges[0].buf);
    ranges[0].buf = NULL;

    // Leave socket open for now since should be a long lived socket
}