#include "handle.h"

int connect_to_server(){
	struct sockaddr_in server_addr, client_addr;
	int client_fd;

    if (( client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    	LOG("connect_to_server() Failed to create client_fd\n");
        return -1;
    }
    LOG("connect_to_server() get clientfd = %d\n", client_fd);

    bzero((char *) &client_addr, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(0);
	inet_pton(AF_INET, fake_ip, &(client_addr.sin_addr));

	int ret_bind = bind( client_fd, (struct sockaddr *)&client_addr, sizeof(client_addr));
    if ( ret_bind < 0) {
		LOG("connect_to_server() Failed to bind client ret = %d\n", errno);
        close( client_fd);
        return -1;
    }
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
   	inet_pton(AF_INET, www_ip, &(server_addr.sin_addr));

   	int ret_connect = connect( client_fd,  (struct sockaddr *) &server_addr, sizeof(server_addr));
    if ( ret_connect < 0) {
    	LOG("connect_to_server() Failed to connect %d\n", ret_connect);
        close(client_fd);
        return -1;
    }

	return client_fd;
}

void handle_client_recv(proxy_session_list_t *node){
	int fd = node->session.client_fd;

	LOG("handle_client_recv fd = %d\n", fd);
    char buffer[MAX_LENGTH];
    memset(buffer, 0 , MAX_LENGTH);
    read(fd, buffer, MAX_LENGTH);
    LOG("%s", buffer);
    int connect;
    if ( (connect = connect_to_server()) == -1){
    	LOG("handle_client_recv() connect_to_server return -1\n");
    }
    LOG("connect = %d\n", connect);
    write(connect, buffer, MAX_LENGTH);
    FD_SET(connect, &ready_to_read);
    node->session.server_fd = connect;
}
void handle_server_recv(proxy_session_list_t *node){
	int fd = node->session.server_fd;
	char buffer[MAX_LENGTH];
    memset(buffer, 0 , MAX_LENGTH);
    read(fd, buffer, MAX_LENGTH);
    LOG("%s", buffer);
    write(node->session.client_fd, buffer, MAX_LENGTH);
    FD_CLR(connect, &ready_to_read);
}







