#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>

#include "socket.h"

// Open a socket to rpc_server and port
// Return the file descriptor
int
sock_open(char* rpc_server, int rpc_port)
{
    char* hostname = rpc_server;
    int   portno   = rpc_port;
    int   sockfd   = -1;
    int   flag     = 0;

    // Lookup the IP address of the host.  By default, getaddrinfo(3) chooses
    // the best IP address for a host according to RFC 3484. I believe this
    // means it will perfer IPv6 addresses if they exist and this host can reach
    // them.  In theory, multiple addresses can be returned and this code should
    // cycle through them until it finds one that works.  This code just uses
    // the first one.
    struct addrinfo    *resp;
    char                portstr[20];
    int                 err;
    snprintf(portstr, sizeof(portstr), "%d", portno);
    err = getaddrinfo(hostname, portstr, NULL, &resp);
    if (err != 0) {
        printf("ERROR: sockopen(): getaddrinfo(%s) returned %s\n", hostname, gai_strerror(err));
        return -1;
    }
    if (resp->ai_family != AF_INET && resp->ai_family != AF_INET6) {
        printf("ERROR: sock_open(): got unkown address family %d for hostname %s\n",
                resp->ai_family, hostname);
        return -1;
    }
    printf("sock_open(): got IPv%d server addrlen %u and socktype %d for hostname %s\n",
            resp->ai_family == AF_INET ? 4 : 6, resp->ai_addrlen, resp->ai_socktype, hostname);

    // Set errno to zero before system calls
    errno = 0;

    // Create the socket
    sockfd = socket(resp->ai_family, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("ERROR: sock_open(): %s opening %s socket\n", strerror(errno),
                resp->ai_family == AF_INET ? "AF_INET" : "AF_INET6");
        freeaddrinfo(resp);
        return -1;
    }

    // Connect to the far end
    if (connect(sockfd, resp->ai_addr, resp->ai_addrlen) < 0) {
        printf("ERROR: sock_open(): %s connecting socket\n", strerror(errno));
        freeaddrinfo(resp);
        return -1;
    }

    flag = 1;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0) {
        printf("ERROR %s setting TCP_NODELAY option\n", strerror(errno));
        freeaddrinfo(resp);
        return -1;
    }

    printf("socket %s:%d opened successfully.\n",hostname,portno);

    freeaddrinfo(resp);
    return sockfd;
}

// TODO - this should really be a deinit() operation to close sockets
// when we shut down....
void sock_close(int sockfd)
{
    close(sockfd);
}

// Write buf onto sockfd and return number of bytes written.
int sock_write(int sockfd, const char* buf, int len) {
    int n = 0;

    // TODO - Handle partial writes and return any error!!!
    n = write(sockfd, buf, len);

    return n;
}