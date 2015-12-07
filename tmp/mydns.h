#ifndef _MYDNS_H_
#define _MYDNS_H_

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <netdb.h>
#include "log.h"

#define BUFSIZE 8192
#define NAMESIZE 18

/* MASK to parse different fields */
const uint16_t qr = 0x8000;
const uint16_t opcode = 0x7800;
const uint16_t aa = 0x0400;
const uint16_t tc = 0x0200;
const uint16_t rd = 0x0100;
const uint16_t ra = 0x00800;
const uint16_t z = 0x0070;
const uint16_t rcode = 0x000F;

/* position (shifting bit) of each field */
const int qr_s = 15;
const int opcode_s = 11;
const int aa_s = 10;
const int tc_s = 9;
const int rd_s = 8;
const int ra_s = 7;
const int z_s = 4;
const int rocde_s = 0;

/* The only dns we care about */
const char valid_dns[18] = {0x05, 0x76, 0x69, 0x64, 0x65, 0x6f,
				  			0x02, 0x63, 0x73,
				  			0x03, 0x63, 0x6d, 0x75,
				  			0x03, 0x65, 0x64, 0x75, 0x00}

/* Structure to build the dns request/response packet */
typedef struct dns_header{
	uint16_t id;
/*	
	uint16_t qr : 1;
	uint16_t opcode : 4;
	uint16_t aa : 1;
	uint16_t tc : 1;
	uint16_t rd : 1;
	uint16_t ra : 1;
	uint16_t z : 3;
	uint16_t rcode : 4;
*/	
	// Concentrate the subfields in one uint16_t
	uint16_t fields;
	uint16_t qdcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
} dns_header_t;

typedef struct dns_question{
	// char qname[18];
	uint16_t qtype;
	uint16_t qclass;
} dns_question_t;

typedef struct dns_answer{
	// char* name;
	uint16_t type;
	uint16_t aclass;
	uint32_t ttl;
	uint16_t rdlength;
	uint32_t rdata;
} dns_answer_t;

// char dns_qa_name[18];

typedef struct dns_packet_s{
	dns_header_t header;
	dns_question_t question;
	dns_answer_t answer;
} dns_packet_t;


/**
 * Initialize your client DNS library with the IP address and port number of
 * your DNS server.
 *
 * @param  dns_ip  The IP address of the DNS server.
 * @param  dns_port  The port number of the DNS server.
 *
 * @return 0 on success, -1 otherwise
 */
int init_mydns(const char *dns_ip, unsigned int dns_port, const char* local_ip);


/**
 * Resolve a DNS name using your custom DNS server.
 *
 * Whenever your proxy needs to open a connection to a web server, it calls
 * resolve() as follows:
 *
 * struct addrinfo *result;
 * int rc = resolve("video.cs.cmu.edu", "8080", null, &result);
 * if (rc != 0) {
 *     // handle error
 * }
 * // connect to address in result
 * free(result);
 *
 *
 * @param  node  The hostname to resolve.
 * @param  service  The desired port number as a string.
 * @param  hints  Should be null. resolve() ignores this parameter.
 * @param  res  The result. resolve() should allocate a struct addrinfo, which
 * the caller is responsible for freeing.
 *
 * @return 0 on success, -1 otherwise
 */

int resolve(const char *node, const char *service, 
            const struct addrinfo *hints, struct addrinfo **res);

struct addrinfo* init_addrinfo(struct addrinfo **res);
dns_packet_t* init_dns_packet(const char *node);
int gen_dns_request(char *buffer, dns_packet_t *packet);

#endif

