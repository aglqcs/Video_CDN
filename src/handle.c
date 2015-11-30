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

	LOG("\n\nhandle_client_recv fd = %d\n", fd);
    char buffer[MAX_LENGTH];
    memset(buffer, 0 , MAX_LENGTH);
    int ret_read = read(fd, buffer, MAX_LENGTH);
    if( ret_read < 0){
		LOG("handle_client_recv() fd = %d, errno = %d\n",fd, errno );
		return;
	}
    int connect;
    if( node->session.server_fd != -1) {
		LOG("already has the connection, close the old and open new\n");
		close(node->session.server_fd);
	}
	if ( (connect = connect_to_server()) == -1){
    	LOG("handle_client_recv() connect_to_server return -1\n");
    	return;
	}
    LOG("connect = %d\n", connect);
	
	// Parse the buffer
	char http_method[8];
	char http_request_file[128];
	char http_version[16];

	sscanf(buffer, "%s %s %s", http_method, http_request_file, http_version);
	if(strncmp(http_method, "GET", 3) != 0 || strncmp(http_version, "HTTP/1.1", 8) != 0){
		LOG("http_method %s || http_version %s is not correct\n", http_method, http_version);
		return;
	}

	chunk_tracker_list_t* tl;
	
	if((tl=create_tracker(http_request_file, node)) != NULL){
		LOG("Video chunk file\n");
		update_bitrate(buffer, tl->throughput);
	}
   
	int ret_write = write(connect, buffer, ret_read);
	LOG("ret_write = %d\n%s", ret_write, buffer);
	

	if( ret_write < 0){   
		LOG("handle_client_recv() error clientfd = %d, errno = %d\n",fd, errno);
		return;
	}
 	if( ret_write > 0){
    	FD_SET(connect, &ready_to_read);
	}
    node->session.server_fd = connect;
	LOG("ending client_Recv()\n");
}

void handle_server_recv(proxy_session_list_t *node){
	LOG("\n\nIn handle_server_recv()\n");
	int fd = node->session.server_fd;
	int total_read = 0;
	char buffer[MAX_LENGTH ];
    char big_buffer[256000];
	char new_buffer[256000];
	memset(buffer, 0 , MAX_LENGTH );
	memset(big_buffer, 0 , 256000);
	memset(new_buffer, 0 ,256000);
    int ret_read = -1;
	while( (ret_read = read(fd, buffer, MAX_LENGTH))  > 0 ){
		LOG("ret_read = %d\n", ret_read);
		memcpy(big_buffer + total_read, buffer, ret_read);
		total_read += ret_read;
		memset(buffer, 0, MAX_LENGTH);
	}
	int parse_ret = parse_f4m_response(big_buffer, total_read, new_buffer);
	int ret_write = 0;
	if( parse_ret != -1 || parse_ret != -2 ){
		ret_write = write(node->session.client_fd, new_buffer,parse_ret);
		LOG("f4m ret_write = %d\n",ret_write);
	}
	else{
		ret_write = write(node->session.client_fd, big_buffer, total_read);
		LOG("data ret_write = %d\n",ret_write);
		// Update the throughput once get all the data
		if(parse_ret == -2){
			int seg;
			int frag;   
			if(2 != sscanf(big_buffer, "%*sSeg%d-Frag%d", &seg, &frag))
				LOG("This is not video chunks, ignore it\n");
			update_throughput(0.3, ret_write, node, seg, frag);
			LOG("Update throughput\n");
		}
	}
	LOG("Exit handle_server_recv() last ret_read = %d total_recv = %d\n", ret_read, total_read);
    FD_CLR(node->session.server_fd, &ready_to_read);
}

int read_line(char *dst, char *src, int size){
	int i;
	for( i = 0;i < size; i ++){
		dst[i] = src[i];
		if( src[i] == '\n'){
			break;
		}
	}
	return i + 1;
}

int start_with(char *src, char *dst){
	int size = strlen(dst);
	int i;
	int j;
	for(i = 0;i < size;i ++){
		if( src[i] == ' ' || src[i] == '\t') continue;
		else break;
	}
	for(j = 0;i < size; j ++,i ++){
		if( src[i] != dst[j]) return -1;
	}
	return 1;
}
	
int write_f4m_nolist(char *src,int offset, char *newbuffer){
	int i;
	char headbuffer[MAX_LENGTH];
	char buffer[256000];
	memset(buffer, 0, 256000);
	memset(headbuffer, 0 ,MAX_LENGTH);
	for(i = 0; i < offset;  i ++){
		headbuffer[i] = src[i];
	}

	char *xml_start = src + offset;
	int read_length = 0;
	int total = 0;
	int write_offset = 0;
	char line[MAX_LENGTH];
	memset(line,0,MAX_LENGTH);
	while( (read_length = read_line( line, xml_start + total, MAX_LENGTH)) > 0){
		if( 1 == start_with(line, "bitrate") ){
		}
		else if( 1 == start_with(line, "</media>") ){
			memcpy( buffer + write_offset ,line, read_length);
			write_offset += read_length;
			break;
		}
		else{
			memcpy(buffer + write_offset, line, read_length);
			write_offset += read_length;
		}
		total += read_length;	
		memset(line, 0, MAX_LENGTH);
	}
	char *end_manifest = "</manifest>";
	memcpy(buffer + write_offset, end_manifest, strlen(end_manifest));
	write_offset += strlen(end_manifest);
	
	
	memset(line, 0 ,MAX_LENGTH);
	total = 0;
	int buffer_offset = 0;
	while( (read_length = read_line( line, headbuffer + total, MAX_LENGTH)) > 0){
		if( 1 == start_with(line,"Content-Length")){
			char temp[50];
			sprintf(temp, "Content-Length: %d\r\n", write_offset -  1);
			memcpy(newbuffer + buffer_offset, temp, strlen(temp));
			buffer_offset += strlen(temp);
		}
		else if( 1 == start_with(line,"Content-Type") ){
			memcpy(newbuffer + buffer_offset, line, read_length);
			buffer_offset += read_length;
			break;
		}
		else{
			memcpy( newbuffer + buffer_offset, line, read_length);
			buffer_offset += read_length;
		}
		memset(line, 0, MAX_LENGTH);
		total += read_length;
	}

	memcpy(newbuffer + buffer_offset, buffer, write_offset);
	return buffer_offset + write_offset;
}

int parse_f4m_response(char *buffer, int total_read, char *newbuffer){
	char line[MAX_LENGTH];
	memset(line, 0, MAX_LENGTH);
	int read_length = 0;
	int total= 0;
	while( total < total_read && (read_length = read_line(line,buffer+total, MAX_LENGTH))  > 0 ){
		total += read_length;
		if( 1 == start_with(line, "Content-Type") ){
			char type[MAX_LENGTH];
			sscanf(line,"Content-Type: %s\n", type);
			if( strcmp(type,"video/f4f") == 0 ) {
				LOG("Parse: type = video, continue ..\n");
				return -2;
			}
			else if( strcmp(type, "text/xml") == 0){
				return write_f4m_nolist(buffer , total + 1, newbuffer);
			}
			else{ 
				LOG("Parse: type = %s\n",type);
				return -1;
			}
		}
		memset(line,0,MAX_LENGTH);
	}
	LOG("Parse: should never come here\n");
	return -1;
}





