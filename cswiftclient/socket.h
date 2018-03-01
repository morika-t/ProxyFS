#ifndef __PFS_SOCKET_H__
#define __PFS_SOCKET_H__

#include <stdio.h>
#include <stdlib.h>

// TODO - need init method to open 10/100 sockets to Swift proxy

#if 0
void sock_close(int sockfd);
int  sock_read(int sock_read, char** buf, int* error);
#endif

// Open the socket and return the file descriptor
int  sock_open(char* rpc_server, int rpc_port);

// Write buf on socket and return error
int  sock_write(int sockfd, const char* buf, int len);

#endif
