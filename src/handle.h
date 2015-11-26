#ifndef _HANDLE_H_
#define _HANDLE_H_

#include "log.h"
#include "proxy.h"
#include <arpa/inet.h>


void handle_client_recv(proxy_session_list_t *node);
int connect_to_server();

#endif
