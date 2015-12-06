#include "nameserver.h"

int main(int argc, char* argv[]){
	int rr_flag = 0;
	char *log, *ip, *port, *servers, *lsa;
	fd_set ready_set, read_set;
	int sockfd, optval;
	struct sockaddr_in serveraddr;
	struct sockaddr_in remaddr;
	int i;
	char buffer[BUFSIZE];
	server_list_t* serv_list = NULL;
	int recvlen;

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

	// GET server lists


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
	FD_SET(sockfd, read_set);
	
	// Start receiving message from clients
	while(1){
		ready_set = read_set;
		int selectnum = 0;
		if ((selectnum = select(sockfd+1, &ready_set, NULL, NULL, NULL)) < 0){
			perror ("error on select\n");
			exit (EXIT_FAILURE);
		}
		if(selectnum == 1){
			int cLen = 0;
			memset(buffer, 0, BUFSIZE);
			// start receiving packet
			recvlen = recvfrom(sockfd, buffer, BUFSIZE, 0, (struct sockaddr *)&remaddr, sizeof(remaddr));
			if(recvlen < sizeof(dns_header_t)){
				perror("receive data to samll\n");
				exit(EXIT_FAILURE);
			}
			// Check and parse the receive packet
			handle_dns_request(buffer, recvlen, remaddr, rr_flag, servers);
		}
	}
}

void get_servers(char* servers, server_list_t *list){
	FILE *fp;
	char *line = NULL;
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

void handle_dns_request(char* buffer, int recvlen, struct sockaddr remaddr, int rr_flag){
	int head_len = sizeof(dns_header_t);

	// Check the first 18 byte to be as same as 5video2cs3cmu3edu
	if(recvlen < (head_len+NAMESIZE))
		send_error(remaddr);
	if(memcmp(valid_dns, buffer+head_len, NAMESIZE))
		send_error(remaddr);

	// Round robin
	if(rr_flag){
		
	}

}

