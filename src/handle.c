#include "handle.h"

extern chunk_tracker_list_t* tracker_head;


int connect_to_server( ){
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
 
 //	bzero((char *) &server_addr, sizeof(server_addr));
 //   server_addr.sin_family = AF_INET;
 //   server_addr.sin_port = htons(8080);
  // 	inet_pton(AF_INET, www_ip, &(server_addr.sin_addr));
	
	init_mydns(dns_ip, dns_port, fake_ip);
	struct addrinfo *dnsinfo;
	resolve("video.cs.cmu.edu", "9999", NULL, &dnsinfo);
	struct sockaddr_in *serv_addrin = (struct sockaddr_in*)dnsinfo->ai_addr;
	char ip_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(serv_addrin->sin_addr), ip_str, sizeof(ip_str));
	printf("Server IP resolved: %s\n", ip_str);
	printf("about to get conn\n");

   	int ret_connect = connect( client_fd,  (struct sockaddr *) &serv_addrin,  sizeof(serv_addrin));
   //	int ret_connect = connect( client_fd,  (struct sockaddr *) &server_addr, sizeof(server_addr));
    if ( ret_connect < 0) {
    	LOG("connect_to_server() Failed to connect %d\n", ret_connect);
        close(client_fd);
        return -1;
    }
	
	int optval = 1;
	if( setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(int)) < 0 ){
		LOG("error set sock no delay");
	}
	return client_fd;
}

void handle_client_recv(proxy_session_list_t *node){
	int fd = node->session.client_fd;

	printf("\n\nhandle_client_recv fd = %d\n", fd);
	LOG("\n\nhandle_client_recv fd = %d\n", fd);
    char buffer[MAX_LENGTH];
    memset(buffer, 0 , MAX_LENGTH);
    int ret_read = read(fd, buffer, MAX_LENGTH);
    if( ret_read < 0){
		LOG("handle_client_recv() fd = %d, errno = %d\n",fd, errno );
		close(fd);
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
    node->session.server_fd = connect;
	
	// Parse the buffer
	char http_method[8];
	char http_request_file[128];
	char http_version[16];
	LOG("==============start parsing client input=============\n");
	sscanf(buffer, "%s %s %s", http_method, http_request_file, http_version);
	LOG("http_request_file:%s\n", http_request_file);
	if(strncmp(http_method, "GET", 3) != 0 || strncmp(http_version, "HTTP/1.1", 8) != 0){
		LOG("error parsing data\n");
		return;
	}

	chunk_tracker_list_t* tl;
	
	if((tl=create_tracker(http_request_file, node)) != NULL){
		LOG("Video chunk file\n");
		printf("tl->throughput: %lf\n", tl->throughput);
		clock_gettime(CLOCK_MONOTONIC, &(node->session.ts));	/* mark start time */
		update_bitrate(buffer, tl, node);
	}
   	
	sscanf(buffer, "%s %s %s", http_method, http_request_file, http_version);
	LOG("MODIFIED http_request_file:%s\n", http_request_file);
	if(strncmp(http_method, "GET", 3) != 0 || strncmp(http_version, "HTTP/1.1", 8) != 0){
		LOG("error parsing data\n");
		return;
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
	LOG("ending client_Recv()\n");
}

void handle_server_recv(char* ip, double alpha, proxy_session_list_t *node){
	LOG("\n\nIn handle_server_recv()\n");
	printf("\n\nIn handle_server_recv()\n");
	int fd = node->session.server_fd;
	int total_read = 0;
	char buffer[MAX_LENGTH];
    char *big_buffer;
	char *new_buffer;
	memset(buffer, 0 ,MAX_LENGTH );
    int ret_read = -1;
	// Start timer

	time_t tmp_start = time(NULL);	
/*	while( (ret_read = read(fd, buffer,6 *  MAX_LENGTH))  > 0 ){
//	while( (ret_read = read(fd, buffer, MAX_LENGTH))  > 0 ){
		LOG("ret_read = %d\n", ret_read);
		memcpy(big_buffer + total_read, buffer, ret_read);
		total_read += ret_read;
		memset(buffer, 0, 6 * MAX_LENGTH);
	}
	time_t tmp_end = time(NULL);
	LOG("time difference read = %d\n", tmp_end - tmp_start );
	LOG("%d\n\n%s", total_read ,big_buffer);
*/
	char ch;
	int state = 0;
	while( (ret_read = read( fd, &ch , 1)) > 0){
		buffer[total_read] = ch;
		total_read ++;
		if( ch == '\r') {
			if( state == 0) state ++;
			if( state == 2) state ++;
		}
		else if( ch == '\n'){
			if( state == 1) state ++;
			if( state == 3) break;
		}
		else{
			if( state == 2) state = 0;
		}
	}
	int next_read = get_response_content_length(buffer);	
	big_buffer = (char *)malloc( next_read + total_read);
	new_buffer = (char *)malloc( next_read + total_read);
	memset(big_buffer , 0 , next_read + total_read);
	memset(new_buffer, 0 , next_read+total_read);
	memcpy(big_buffer, buffer, total_read);

//	int t = total_read + next_read;
//	while( t > total_read ){
//		total_read += read(fd, big_buffer + total_read, 8912);
//	}
	recv(fd, big_buffer + total_read, next_read, MSG_WAITALL);
	total_read += next_read;
	
	printf("content-length = %d, total_read = %d", next_read, total_read);
	
	int parse_ret = parse_f4m_response(big_buffer, total_read, new_buffer, node);
	int ret_write = 0;
	if( parse_ret != -1 && parse_ret != -2 ){
		ret_write = write(node->session.client_fd, new_buffer,parse_ret);
		LOG("f4m ret_write = %d\n",ret_write);
	}
	else{
		// Update the throughput once get all the data
		if(parse_ret == -2){
			char *type = strstr(big_buffer, "Content-Type: video/f4f");

			if(type == NULL)
				LOG("is is not video chunks, ignore it\n");
			else
				LOG("this is video chunks from server\n");
			update_throughput(alpha, total_read, node, ip, node->session.ts);
			printf("tracker_head->throughput: %lf\n", tracker_head->throughput);
			LOG("Update throughput done\n");
		}
		ret_write = write(node->session.client_fd, big_buffer, total_read);
		LOG("data ret_write = %d\n",ret_write);
	}

	LOG("Exit handle_server_recv() last ret_read = %d total_recv = %d\n", ret_read, total_read);
    FD_CLR(node->session.server_fd, &ready_to_read);
	FD_CLR(node->session.client_fd, &ready_to_read);
	close(node->session.client_fd);
	free(big_buffer);
	free(new_buffer);
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

int get_response_content_length(char *buffer){
	int length = 0;
	char line[MAX_LENGTH];
	memset(line, 0, MAX_LENGTH);
	int readline = 0;
	int total_read = 0;
	while( (readline = read_line(line, buffer + total_read, MAX_LENGTH)) > 0 ){
		total_read += readline;
		if( 1 == start_with(line, "Content-Length:")){
			char tmp_number[30];
			int i,j;
			for(i = 0, j = 16; line[j] != '\r' ; i ++, j ++){
				tmp_number[i] = line[j];
			} 
			tmp_number[i] = '\0';
			length = atoi(tmp_number);
			break;
		}
		memset(line, 0 ,MAX_LENGTH);
	} 
	return length;
}
int write_f4m_nolist(char *src,int offset, char *newbuffer, proxy_session_list_t *list_node){
	int i;
	char headbuffer[MAX_LENGTH];
	char buffer[256000];
	memset(buffer, 0, 256000);
	memset(headbuffer, 0 ,MAX_LENGTH);
	for(i = 0; i < offset ;  i ++){
		headbuffer[i] = src[i];
	}

	char *xml_start = src + offset;
	int read_length = 0;
	int total = 0;
	int write_offset = 0;
	int no_write = 0;
	char line[MAX_LENGTH];
	memset(line,0,MAX_LENGTH);
	while( (read_length = read_line( line, xml_start + total, MAX_LENGTH)) > 0){
		if( 1 == start_with(line, "bitrate") ){
			// sample line is bitrate="100"
			char bitrate[10];
			int i,j;
			for(i = 0; i < strlen(line); i ++){
				if( line[i] == '"') break;
			}
			for( i = i + 1, j = 0; i < strlen(line);i ++,j ++){
				bitrate[j] = line[i];
			}
			bitrate[j] = 0;
			int sample_rate = atoi(bitrate);
			LOG("available rate = %d\n", sample_rate);
			for(i = 0;i < 10; i ++){
				if( list_node->session.bitrate[i] == -1 ){
					list_node->session.bitrate[i] = sample_rate;
					break;
				}
			}
		}
		else if( 1 == start_with(line, "</media>") ){
			if( no_write == 0){
				memcpy( buffer + write_offset ,line, read_length);
				write_offset += read_length;
			}
			//break;
			no_write = 1;
		}
		else if(1 == start_with(line, "</manifest>")){
			char *end_manifest = "</manifest>";
			memcpy(buffer + write_offset, end_manifest, strlen(end_manifest));
			write_offset += strlen(end_manifest);
			break;
		}
		else{
			if(no_write == 0){
				memcpy(buffer + write_offset, line, read_length);
				write_offset += read_length;
			}
		}
		total += read_length;	
		memset(line, 0, MAX_LENGTH);
	}
	printf("END F4M\n");	
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
	newbuffer[buffer_offset] = '\r';
	newbuffer[buffer_offset+1] = '\n';
	buffer_offset += 1;
	memcpy(newbuffer + buffer_offset, buffer, write_offset);
	return buffer_offset + write_offset;
}

int parse_f4m_response(char *buffer, int total_read, char *newbuffer, proxy_session_list_t *list_node){
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
				return write_f4m_nolist(buffer , total + 1, newbuffer, list_node);
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





