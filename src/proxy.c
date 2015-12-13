/*
*    shuo chen
*    shuoc@andrew.cmu.edu
*/

#include "proxy.h"
#include "log.h"
#include "handle.h"
#include "mydns.h"

proxy_session_list_t *head;


int main(int argc, char* argv[]){
    int server_sockfd, client_sockfd;       
    struct sockaddr_in addr;

    signal(SIGPIPE, SIG_IGN);

    log_file = argv[1];
    alpha = atof(argv[2]);
    listen_port = atoi(argv[3]);
    fake_ip = argv[4];
    dns_ip = argv[5];
    dns_port = atoi(argv[6]);
    www_ip = argv[7];

    head = NULL;

    LOG_start();
	TEST_LOG_start(log_file);
	
	// Initate dns listen
	init_mydns(dns_ip, dns_port, fake_ip);
	struct addrinfo *dnsinfo;
	resolve("video.cs.cmu.edu", "9999", NULL, &dnsinfo);
	struct sockaddr_in *serv_addrin = (struct sockaddr_in*)dnsinfo->ai_addr;
	char ip_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(serv_addrin->sin_addr), ip_str, sizeof(ip_str));
	printf("Server IP resolved: %s\n", ip_str);
	printf("about to get conn\n");

    LOG("server listening on port = %d\n", listen_port);
    if((server_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        return EXIT_FAILURE;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(listen_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if(bind(server_sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0){
        close(server_sockfd);
        return EXIT_FAILURE;
    }    

    if(listen(server_sockfd, 5) < 0){
        close(server_sockfd);
        return EXIT_FAILURE;
    }
    FD_ZERO(&ready_to_read);
    FD_ZERO(&ready_to_write);

    FD_SET( server_sockfd, &ready_to_read);

    while(1){
        int select_count = select( FD_SETSIZE, &ready_to_read, &ready_to_write, NULL, NULL);
        LOG(" server_sock = %d select_count = %d\n", server_sockfd, select_count);
        if( select_count < 0){
            // select error
			continue;
			//return EXIT_FAILURE;
        }
        if( FD_ISSET(server_sockfd, &ready_to_read)){
           	LOG("GET NEW CONNECTION\n");
			struct sockaddr_in client_addr;
		   	socklen_t size = sizeof(client_addr);
			int client_fd = accept(server_sockfd, (struct sockaddr *)&client_addr, &size);
            if( client_fd < 0 ){
                // accept error
                return EXIT_FAILURE;
            }
			LOG("get %s\n",inet_ntoa(client_addr.sin_addr ));
		   	FD_SET(client_fd, &ready_to_read);
            proxy_session_list_t *new_element = (proxy_session_list_t *)malloc( sizeof(proxy_session_list_t));
            new_element->session.client_fd = client_fd;
            new_element->session.server_fd = -1; 
		   	int i;
			for( i = 0;i < 10;i ++) new_element->session.bitrate[i] = -1;
			if( head == NULL){
                new_element->next = NULL;
                head = new_element;
            } 
            else{
                new_element->next = head;
                head = new_element;
            }
        }

        proxy_session_list_t *p;
        for( p = head ; p != NULL; p = p->next){
            if (FD_ISSET(p->session.client_fd, &ready_to_read)) {
                handle_client_recv(p);                
            }
            if (FD_ISSET(p->session.client_fd, &ready_to_write)) {
                LOG("2\n");
               // handle_client_send();                
            }
            if (FD_ISSET(p->session.server_fd, &ready_to_read)) {
                handle_server_recv(www_ip, alpha, p);                
            }
            if (FD_ISSET(p->session.server_fd, &ready_to_write)) {
                LOG("4\n");

               // handle_server_send();                
            }
        }

    }

    // if come here then error happens
    printf("Should never come here!\n");
    close(server_sockfd);
    return EXIT_FAILURE;
}
