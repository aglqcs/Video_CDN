#ifndef _NAMESERVER_H_
#define _NAMESERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include "log.h"
#include "mydns.h"

dns_packet_t* parse_request(char* buffer);

#endif

