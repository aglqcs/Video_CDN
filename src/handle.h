#ifndef _HANDLE_H_
#define _HANDLE_H_

#include "log.h"
#include "proxy.h"
#include "bitrate.h"
#include <arpa/inet.h>

int parse_f4m_response(char *, int, char *, proxy_session_list_t *);
void handle_client_recv(proxy_session_list_t *node);
void handle_server_recv(char* ip, float alpha, proxy_session_list_t *node);
int connect_to_server();

#endif
