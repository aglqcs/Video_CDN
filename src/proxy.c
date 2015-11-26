/*
*    shuo chen
*    shuoc@andrew.cmu.edu
*/

#include "proxy.h"
#include "log.h"
#include "handle.h"

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

    LOG_start(log_file);
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
        LOG("select_count = %d\n", select_count);
        if( select_count < 0){
            // select error
            return EXIT_FAILURE;
        }
        if( FD_ISSET(server_sockfd, &ready_to_read)){
            int client_fd = accept(server_sockfd, NULL, NULL);
            if( client_fd < 0 ){
                // accept error
                return EXIT_FAILURE;
            }
            FD_SET(client_fd, &ready_to_read);
            proxy_session_list_t *new_element = (proxy_session_list_t *)malloc( sizeof(proxy_session_list_t));
            new_element->session.client_fd = client_fd;
            new_element->session.server_fd = -1; 
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
                handle_server_recv(p);                
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
