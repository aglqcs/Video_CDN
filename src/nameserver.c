#include "nameserver.h"

int sockfd;

int main(int argc, char* argv[]){
	int rr_flag = 0;
	char *log, *ip, *port, *servers, *lsa;
	fd_set ready_set, read_set;
	int optval;
	struct sockaddr_in serveraddr;
	//struct sockaddr_in remaddr;
	char buffer[BUFSIZE];
	int recvlen;
	server_list_t* serv_list = NULL;
	server_list_t* rr_ptr = NULL;

	// not round robin
	if(argc == 6){
		log = argv[1];
		ip = argv[2];
		port = argv[3];
		servers = argv[4];
		lsa = argv[5];
	}
	// round robin
	else if(argc == 7){
		if(strncmp(argv[1], "-r", 2)){	
			fprintf(stderr, "Usage: ./nameserver [-r] <log> <ip> <port> <servers> <LSAs>\n");
			return -1;
		}
		rr_flag = 1;
		log = argv[2];
		ip = argv[3];
		port = argv[4];
		servers = argv[5];
		lsa = argv[6];
	}
	else{
		fprintf(stderr, "Usage: ./nameserver [-r] <log> <ip> <port> <servers> <LSAs>\n");
		return -1;
	}
	
	N_LOG_start(log);
	N_LOG("start nameserver.....\n");

	// Build server lists
	serv_list = get_servers(servers, serv_list);
	// Build udp connection
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		perror("Failed create socket\n");
	
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &(serveraddr.sin_addr));
	serveraddr.sin_port = htons(atoi(port));
	
	if(bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
		perror("Error on binding");
	
	// Set and initiallize the fdsets
	FD_ZERO (&read_set);
	FD_SET(sockfd, &read_set);

	printf("start receiving msg from clients\n");
	// Start receiving message from clients
	while(1){
		ready_set = read_set;
		int selectnum = 0;
		if ((selectnum = select(sockfd+1, &ready_set, NULL, NULL, NULL)) < 0){
			perror ("error on select\n");
			exit (EXIT_FAILURE);
		}
		// Receive message
		if(selectnum == 1){
			struct sockaddr_in remaddr;
			printf("receive message\n");
			memset(buffer, 0, BUFSIZE);
			// start receiving packet
			socklen_t addrlen = (socklen_t) sizeof(remaddr);
			recvlen = recvfrom(sockfd, buffer, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
			char from_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(remaddr.sin_addr), from_str, INET_ADDRSTRLEN);
			if(recvlen < sizeof(dns_header_t)){
				perror("receive data to samll\n");
				exit(EXIT_FAILURE);
			}
			// socklen_t rem_len = 
			// Check and parse the receive packet
			handle_dns_request(buffer, recvlen, (struct sockaddr *)&remaddr, rr_flag, serv_list, &rr_ptr);
			printf("Curren rr_ptr: %s\n", rr_ptr->sname);
		}
	}
}

server_list_t* get_servers(char* servers, server_list_t *list){
	FILE *fp;
	char *line = NULL;
	size_t len;
	// Read the topo file
	fp = fopen(servers, "r");
	if(fp == NULL){
	perror("failed to read topo server file\n");
		exit(EXIT_FAILURE);
	}
	while (getline(&line, &len, fp) > 0) {
		server_list_t *tmp = (server_list_t*)malloc(sizeof(server_list_t));
		strncpy(tmp->sname, line, 15);
		translate_ip_to_hex(tmp);
		tmp->next = NULL;
		if(list == NULL){
			list = tmp;
		}
		else{
			server_list_t *fl = list;
			while(fl->next != NULL){
				fl = fl->next;
			}
			fl->next = tmp;
		}
	}
	return list;
	// rr_ptr = list;	
}

void translate_ip_to_hex(server_list_t* tmp){
	char line[15];
	char *ptr;
	int i = 0;
	strncpy(line, tmp->sname, 15);
	ptr = strtok(line, ".");
	while(ptr != NULL){
		int a = atoi(ptr);
		if(i>=4){
			perror("DNS Name error when parsing to hex\n");
			return;
		}
		tmp->hex[i++] = a;
		ptr = strtok(NULL, ".");
	}
}

void handle_dns_request(char* buffer, int recvlen, struct sockaddr* remaddr, int rr_flag, server_list_t *serv_list, server_list_t** rr_ptr){
	int head_len = sizeof(dns_header_t);
	int resp_len = 0;
	int send_len;
	char sbuf[BUFSIZE];
	// Check the first 18 byte to be as same as 5video2cs3cmu3edu
	if(recvlen < (head_len+NAMESIZE)){
		send_error(remaddr);
		return;
	}
	else if(memcmp(valid_dns, buffer+head_len, NAMESIZE)){
		send_error(remaddr);
		return;
	}

	// Round robin
	if(rr_flag){
		if(*rr_ptr == NULL){
			*rr_ptr = serv_list; 
		}
		else if((*rr_ptr)->next == NULL)
			*rr_ptr = serv_list;
		else{
			*rr_ptr = (*rr_ptr)->next;
		}
		if((resp_len=gen_resp_pkt(sbuf, (*rr_ptr)->hex)) == 0){
			perror("gen_resp_pkt error\n");
			return;
		}
	}
	else{
		printf("==========dijkstra===========\n");
		// TODO: get ip and gen_resp_pkt
	}
	
	send_len = sendto(sockfd, sbuf, resp_len, 0, remaddr, (socklen_t)sizeof(struct sockaddr_in));
	if(send_len < 0)
		perror("send response packet back error\n");

}

void send_error(struct sockaddr* remaddr){
	char buffer[BUFSIZE];
	int len;
	int n;
	if((len=gen_error_pkt(buffer)) == 0){
		perror("gen_error_pkt failed\n");
		return;
	}
	n = sendto(sockfd, buffer, len, 0, remaddr, (socklen_t)sizeof(struct sockaddr_in));
	if(n < 0)
		perror("send error back error\n");
	return;
}

