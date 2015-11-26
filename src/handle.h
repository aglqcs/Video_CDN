#ifndef _HANDLE_H_
#define _HANDLE_H_

#include "log.h"
#include "proxy.h"
#include <arpa/inet.h>


void handle_client_recv(int fd);
int connect_to_server();

#endif
