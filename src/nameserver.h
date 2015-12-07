#ifndef _NAMESERVER_H_
#define _NAMESERVER_H_

#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include "log.h"
#include "mydns.h"

typedef struct server_list{
	char sname[15];	
	char hex[4];
	struct server_list *next;
} server_list_t;

server_list_t* get_servers(char* servers, server_list_t *list);
void translate_ip_to_hex(server_list_t* tmp);
void handle_dns_request(char* buffer, int recvlen, struct sockaddr *remaddr, int rr_flag, server_list_t *serv_list, server_list_t **rr_ptr);
void send_error(struct sockaddr *remaddr);

#endif

