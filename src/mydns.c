#include "mydns.h"

struct sockaddr_in dns_addr;
int dns_sock;
const char valid_dns[18] = {0x05, 0x76, 0x69, 0x64, 0x65, 0x6f,
							0x02, 0x63, 0x73,
							0x03, 0x63, 0x6d, 0x75,
							0x03, 0x65, 0x64, 0x75, 0x00};

int init_mydns(const char *dns_ip, unsigned int dns_port, const char *local_ip){
	struct sockaddr_in myaddr;
	
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

	if(bind(dns_sock, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1){
		perror("peer_run could not bind socket\n");
		exit(-1);
	}
	return 0;
}

int resolve(const char *node, const char *service,
			const struct addrinfo *hints, struct addrinfo ** res){
	int buf_len;
	char request_buffer[BUFSIZE];
	char receive_buffer[BUFSIZE];
	struct addrinfo* tmp_address;
	struct sockaddr_in getr_addr;
	socklen_t slen = sizeof(getr_addr);
	dns_packet_t *request_packet;

	/* Initiallize address info */
	LOG("Start resolve() function\n");
	tmp_address = init_addrinfo(res);

	/* Generate the dns packet send to our name server */
	LOG("Generate dns packet\n");
	request_packet = init_dns_packet();
	buf_len = gen_dns_request(request_buffer, request_packet, node);

	/* send packet to nameserver */
	printf("sending packet to dns server\n");
	int ssize = sendto(dns_sock, &request_buffer, buf_len, 0, (struct sockaddr *) &dns_addr, sizeof(struct sockaddr));
	if(ssize <= 0){
		perror("sendto dns server error\n");
		return -1;
	}
	printf("finish sending packet to dns server\n");

	/* TODO: parse the receive packet from nameserver */
	printf("start receiving response\n");
	ssize = 0;
	ssize = recvfrom(dns_sock, receive_buffer, BUFSIZE, 0, (struct sockaddr *)&getr_addr, &slen);
	printf("recv size: %d\n", ssize);
	parse_response(receive_buffer, ssize, tmp_address);

	/* TODO: retrieve the address and send back to proxy */
	((struct sockaddr_in*)tmp_address->ai_addr)->sin_port = htons(atoi(service));
	return 0;
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

dns_packet_t* init_dns_packet(){
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
	dns_p->header.qdcount = 0x01;
	dns_p->header.ancount = 0;
	dns_p->header.nscount = 0;
	dns_p->header.arcount = 0;
	// question setup
	// transfer_dns_name(node, buffer);
	// memcpy(dns_p->question.name, buffer, NAMESIZE);
	// dns_p->question.name = (char*)malloc(sizeof(node));
	dns_p->question.qtype = 0x01;
	dns_p->question.qclass = 0x01;
	// answer setup
	// dns_p->answer.name = NULL;
	dns_p->answer.type = 0;
	dns_p->answer.aclass = 0;
	dns_p->answer.ttl = 0;
	dns_p->answer.rdlength = 0;
	// dns_p->answer.rdata = 0;
	return dns_p;
}


int gen_dns_request(char *buffer, dns_packet_t *packet, const char *node){
	printf("gen_dns_request\n");
	int idx = 0;
	char dns_buffer[NAMESIZE];
	int h_size = sizeof(dns_header_t);
	int q_type_size = sizeof(packet->question.qtype);
	int q_class_size = sizeof(packet->question.qclass);
	int i;

	// Change packet to network format
	packet->question.qtype = htons(packet->question.qtype);
	packet->question.qclass = htons(packet->question.qclass);
	packet->header.qdcount = htons(packet->header.qdcount);

	// copy all the data to buffer
	memset(buffer, 0, BUFSIZE);
	// put header
	memcpy(buffer, &packet->header, h_size);
	// for(i=0; i<h_size; i++){
	// 	printf("header[%d]: 0x%x\n", i, buffer[i]);
	// }
	idx += h_size;
	// put dns name
	transfer_dns_name(node, dns_buffer);
	// For testing use
	for(i=0; i<NAMESIZE; i++){
		printf("dns_buffer[%d]: 0x%x\n", i, dns_buffer[i]);
	}
	printf("\n");
	memcpy(buffer+idx, dns_buffer, NAMESIZE);
	idx += NAMESIZE;
	memcpy(buffer+idx, &packet->question.qtype, q_type_size);
	idx += q_type_size;
	memcpy(buffer+idx, &packet->question.qclass, q_class_size);
	idx += q_class_size;
	// for(i=0; i<idx; i++){
	// 	printf("buffer[%d]: 0x%x\n", i, buffer[i]);
	// }
	return idx;
	// Ignore answer part, since we're sending a request.
}


void transfer_dns_name(const char *dns, char *buffer){
	const char* tmp1 = dns;
	char* tmp2 = buffer;
	int dns_len = strlen(dns);
	int sublen = 0;
	int i;
	// Clear buffer
	memset(buffer, 0, NAMESIZE);
	// Put the word in the buffer
	for(i=0; i<dns_len; i++){
		if(i == 15){
			if(sublen > 0){
				memset(tmp2, sublen+1, 1);
				tmp2++;
				memcpy(tmp2, tmp1, sublen+1);
			}
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
	
	dns_packet_t* pack = init_dns_packet();
	pack->header.fields = 3;
	pkt_size = (int)sizeof(dns_packet_t);

	buffer = (char*)malloc(pkt_size);
	memcpy(buffer, pack, pkt_size);
	return pkt_size;
}

// TODO: generate response packet
int gen_resp_pkt(char* buffer, char* hex){
	int hsize = sizeof(dns_header_t);
	int qsize = sizeof(dns_question_t);
	int asize = sizeof(dns_answer_t);

	memset(buffer, 0, BUFSIZE);
	int pkt_size = 0;
	dns_packet_t* pack = init_dns_packet();
	pack->header.fields = htons(0x8400);
	pack->header.qdcount = htons(0x0001);
	pack->header.ancount = htons(0x0001);

	pack->question.qtype = 0;
	pack->question.qclass = 0;
	
	pack->answer.type = htons(0x0001);
	pack->answer.aclass = htons(0x0001);
	pack->answer.ttl = 0;
	pack->answer.rdlength = htons(0x0004);
	int i;
	dns_answer_t* strucPtr=&pack->answer;
	unsigned char* charPtr=(unsigned char*)strucPtr;
	for(i=0; i<asize-2; i++){
		printf("p_ans[%d]: 0x%x\n", i, charPtr[i]);
	}
	
	// header
	memcpy(buffer, &pack->header, hsize);
	pkt_size += hsize;
	printf("pkt_size: %d\n", pkt_size);
	// question
	memcpy(buffer+pkt_size, valid_dns, NAMESIZE);
	pkt_size += NAMESIZE;
	printf("pkt_size: %d\n", pkt_size);
	memcpy(buffer+pkt_size, &pack->question, qsize);
	pkt_size += qsize;
	printf("pkt_size: %d\n", pkt_size);
	// answer
	memcpy(buffer+pkt_size, valid_dns, NAMESIZE);
	pkt_size += NAMESIZE;
	printf("pkt_size: %d\n", pkt_size);
	memcpy(buffer+pkt_size, &pack->answer, asize-2);
	pkt_size += asize-2;
	printf("pkt_size: %d\n", pkt_size);
	memcpy(buffer+pkt_size, hex, 4);
	pkt_size += 4;
	printf("pkt_size: %d\n", pkt_size);

	printf("gen_resp_pkt done------> response packet size: %d\n", pkt_size);

	return pkt_size;
}

void parse_response(char* buffer, int size, struct addrinfo* tmp_address){
	uint32_t *ip;
	char ip_str[4];
	memcpy(ip_str, buffer+size-4, 4);
	ip = (uint32_t*)ip_str;
	((struct sockaddr_in*)tmp_address->ai_addr)->sin_addr.s_addr = *ip;
	return;
}

