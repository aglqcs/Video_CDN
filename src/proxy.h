#ifndef _PROXY_H_
#define _PROXY_H_

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_LENGTH 8192

char *log_file;
float alpha;
int listen_port;
char *fake_ip;
char *dns_ip;
int dns_port;
char *www_ip;

typedef struct proxy_session{
	int client_fd;
	int server_fd;
} proxy_session_t;

typedef struct proxy_session_list{
	proxy_session_t session;
	struct proxy_session_list *next;
} proxy_session_list_t;

#endif