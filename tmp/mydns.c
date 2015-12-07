#include "mydns.h"

struct sockaddr_in dns_addr;
int dns_sock;

int init_mydns(const char *dns_ip, unsigned int dns_port, const char *local_ip){
	struct sockaddr_in myaddr;
	fd_set readfds;
	
	N_LOG("Initialize dns server\n");

	/* Create dns address and port */
	bzero(&dns_addr, sizeof(dns_addr));
	dns_addr.sin_family = AF_INET;
	dns_addr.sin_port = htons(dns_port);
	inet_pton(AF_INET, dns_ip, &(dns_addr.sin_addr));

	/* Create the socket fd for proxy to communicate with dns server */
	if((dns_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1){
		perror("init_mydns can not create socket\n");
		exit(-1);
	}

	bzero(&myaddr, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(0);
	inet_pton(AF_INET, local_ip, &(myaddr.sin_addr)); // IPv4

	if(bind(sock, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1){
		perror("peer_run could not bind socket\n");
		exit(-1);
	}
	return 0;
}

int resolve(const char *node, const char *service,
			const struct addrinfo *hints, struct addrinfo ** res){
	struct addrinfo *tmp_address;
	// char request_buffer[NAMESIZE];
	int buf_len;
	char request_buffer[BUFSIZE];
	dns_packet_t *request_packet;

	/* Initiallize address info */
	LOG("Start resolve() function\n");
	tmp_address = init_addrinfo(res);

	/* TODO: Generate the dns packet send to our name server */
	LOG("Generate dns packet\n");
	request_packet = init_dns_packet();
	buf_len = gen_dns_request(request_buffer, request_packet, node);

	/* TODO: send packet to nameserver */
	// sendto(dns_sock, &request_buffer, sizeof(dns_packet_t), 0, (struct sockaddr *) &dns_addr, sizeof(struct sockaddr));
	sendto(dns_sock, &request_buffer, buf_len, 0, (struct sockaddr *) &dns_addr, sizeof(struct sockaddr));

	/* TODO: parse the receive packet from nameserver */

	/* TODO: retrieve the address and send back to proxy */

}

struct addrinfo* init_addrinfo(struct addrinfo ** res){
	struct addrinfo *addr;
	*res = (struct addrinfo*)malloc(sizeof(struct addrinfo));
	addr = *res;
	addr->ai_flags = AI_PASSIVE;
	addr->ai_family = AF_INET;
	addr->ai_socktype = SOCK_STREAM;
	addr->ai_protocol = 0;
	addr->ai_addrlen = sizeof(struct sockaddr_in);
	addr->ai_canonname = NULL;
	addr->ai_addr = (struct sockaddr*) malloc(sizeof(struct sockaddr_in));
	addr->ai_next = NULL;
	return addr;
}

dns_pakcet_t* init_dns_packet(){
	char buffer[NAMESIZE];
	/* Initiallize the packet */
	dns_packet_t *dns_p = (dns_packet_t*)malloc(sizeof(dns_packet_t));
	// header setup
	dns_p->header.id = 0;     // TODO: should we random generate id or set to zero?
	/*
	dns_p->header.qr = 0;
	dns_p->header.opcode = 0;
	dns_p->header.aa = 0;
	dns_p->header.tc = 0;
	dns_p->header.rd = 0;
	dns_p->header.ra = 0;
	dns_p->header.z = 0;
	dns_p->header.rcode = 0;
	*/
	dns_p->header.fields = 0;
	dns_p->header.qdcount = 1;
	dns_p->header.ancount = 0;
	dns_p->header.nscount = 0;
	dns_p->header.arcount = 0;
	// question setup
	// transfer_dns_name(node, buffer);
	// memcpy(dns_p->question.name, buffer, NAMESIZE);
	// dns_p->question.name = (char*)malloc(sizeof(node));
	dns_p->question.qtype = 1;
	dns_p->question.qclass = 1;
	// answer setup
	// dns_p->answer.name = NULL;
	dns_p->answer.type = 0;
	dns_p->answer.aclass = 0;
	dns_p->answer.ttl = 0;
	dns_p->answer.rdlength = 0;
	dns_p->answer.rdata = 0;
	return dns_p;
}


int gen_dns_request(char *buffer, dns_packet_t *packet, const char *node){
	int idx = 0;
	char dns_buffer[NAMESIZE];
	int h_size = sizeof(dns_header_t);
	int q_type_size = sizeof(packet->question.qtype);
	int q_class_size = sizeof(packet->question.qclass);

	// Change packet to network format
	packet->question.qtype = htons(packet->question.qtype);
	packet->question.qclass = htons(packet->question.qclass);
	packet->header.qdcount = htons(packet->header.qdcount);

	// copy all the data to buffer
	memset(buffer, 0, BUF_SIZE);
	// put header
	memcpy(buffer, packet->header, h_size);
	idx += h_size;
	// put dns name
	transfer_dns_name(node, dns_buffer);
	int i;
	// For testing use
	for(i=0; i<NAMESIZE; i++){
		printf("%c", dns_buffer[i]);
	}
	printf("\n");
	memcpy(buffer+idx, dns_buffer, q_name_size);
	idx += NAMESIZE;
	memcpy(buffer+idx, packet->question.qtype, q_type_size);
	idx += q_type_size;
	memcpy(buffer+idx, packet->question.qclass, q_class_size);
	idx += q_class_size;
	return idx;
	// Ignore answer part, since we're sending a request.
}


void transfer_dns_name(const char *dns, char *buffer){
	char* tmp1 = dns;
	char* tmp2 = buffer;
	int dns_len = strlen(dns);
	int sublen = 0;
	int i;
	// Clear buffer
	memset(buffer, 0, NAMESIZE);
	// Put the word in the buffer
	for(i=0; i<dns_len; i++){
		if(i==17){
			if(sublen > 0){
				memset(tmp2, sublen, 1);
				tmp2++;
				memcpy(tmp2, tmp1, sublen);
			}
			return;
		}
		if(dns[i] == '.'){
			// set the number of substring
			memset(tmp2, sublen, 1);
			tmp2++;
			// put the substring to buffer
			memcpy(tmp2, tmp1, sublen);
			tmp1 += sublen+1;
			tmp2 += sublen;
			// init sublen
			sublen = -1;
		}
		sublen++;
	}
}

int gen_error_pkt(char* buffer){
	int pkt_size = -1;
	
	dns_pakcet_t* pack = init_dns_packet();
	pack->header.fields = 3;
	pkt_size = (int)sizeof(dns_packet_t);

	buffer = (char*)malloc(pkt_size);
	memcpy(buffer, pack, pkt_size);
	return pkt_size;
}


