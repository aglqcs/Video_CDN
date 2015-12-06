#ifndef _NAMESERVER_H_
#define _NAMESERVER_H_

#include "mydns.h"

typedef struct server_list{
	char sname[15];	
	char hex[4];
	struct server_list *next;
} server_list_t;

#endif

