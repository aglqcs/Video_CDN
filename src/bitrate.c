#include "bitrate.h"
 
chunk_tracker_list_t *tracker_head = NULL;

// Estimate throughput
double est_tp(double alpha, double curr_tp, struct timespec ts, double buck_size, double *tput, double *duration){
	// Calculate time
	struct timespec tf;
	clock_gettime(CLOCK_MONOTONIC, &tf);	/* mark the end time */
	uint64_t diff = BILLION * (tf.tv_sec - ts.tv_sec) + tf.tv_nsec - ts.tv_nsec;
	double tdiff = (double)diff/(double)1000000000;

	double new_tp = buck_size*8 / tdiff / 1024;
	*tput = new_tp;
	*duration = tdiff;
	printf("tdiff: %lf\n", tdiff);
	printf("buck_size: %lf\n", buck_size);
	printf("new_tp: %lf\n", new_tp);
	return new_tp * alpha + curr_tp * (1 - alpha); 
}

chunk_tracker_list_t* create_tracker(char* file, proxy_session_list_t* node){
	
	if(strstr(file, "-Frag") == NULL){	
		LOG("This is not video chunks, ignore it\n");
		return NULL;
	}
	
	chunk_tracker_list_t* tracker_list = search_seg(node);

	if(tracker_list == NULL){
		LOG("This is a new chunk, create new list\n");
		printf("This is a new chunk, create new list\n");
		tracker_list = (chunk_tracker_list_t *)malloc( sizeof(chunk_tracker_list_t));
		tracker_list->next = NULL;
		tracker_list->ps = node;
		tracker_list->throughput = 100;
		tracker_list->bitrate = 100;
		tracker_list->chunks = NULL;
		if(tracker_head == NULL){
			tracker_list->next = NULL;
			tracker_head = tracker_list;
		}
		else{
			tracker_list->next = tracker_head;
			tracker_head = tracker_list;
		}
	}

	chunk_tracker_t *new_tracker = (chunk_tracker_t *)malloc( sizeof(chunk_tracker_t));
	strncpy(new_tracker->file, file, 48);

	if(tracker_list->chunks == NULL){
		tracker_list->chunks = new_tracker;
		new_tracker->next = NULL;
	}
	else{
		new_tracker->next = tracker_list->chunks;
		tracker_list->chunks = new_tracker;
	} 
	return tracker_list;
}

chunk_tracker_list_t* search_seg(proxy_session_list_t* pl){
	if(tracker_head == NULL){
		LOG("search_seg is NULL\n");
		printf("search_seg is NULL\n");
		return NULL;
	}
	chunk_tracker_list_t *node;
	for(node=tracker_head; node!=NULL; node=node->next){
		if(node->ps->session.client_fd == pl->session.client_fd){
			LOG("search_seg is not NULL\n");
			return node;
		}
	}
	LOG("search_seg is NULL, but tracker_head is not NULL\n");
	printf("search_seg is NULL, but tracker_head is not NULL\n");
	return NULL;
}

void update_bitrate(char* buffer, chunk_tracker_list_t *tl, proxy_session_list_t* node){
	char first[8192];
	char second[8192];
	double rate = 100;
	//
	double rate_get[4] = {10, 100, 500, 1000};
	double throughput = tl->throughput;
	printf("throughput: %lf\n", throughput);
	int i;
	for(i=3; i>=0; i--){
		if(rate_get[i]*1.5 <= throughput){
			rate = rate_get[i];
			printf("---------------rate: %d--------------\n", (int)rate);
			break;
		}
	}

	// Update bitrate in chunk_tracker_list_t
	tl->bitrate = (int)rate;
	
	printf("Next rate = %d\n", tl->bitrate);

	memset( first,0,8192);
	memset( second, 0, 8192);
	// Retrieve the value of first and second part of string except bitrate
	char *result = strstr(buffer, "Seg");
	strcpy(second, result);
	while(*result != '/'){
		result--;
	}
	char *p;
	for( i = 0, p = buffer; p != result + 1;i ++, p++ ){
		first[i] = *p;
	}
	first[i ] = '\0';
	char bit_rate[20];
	sprintf(bit_rate, "%d", (int)rate);

	strcpy(buffer, first);
	strcat(buffer, bit_rate);
	printf("bitrate: %s\n", bit_rate);
	strcat(buffer, second);

	char *http = strstr(buffer, "HTTP/1.1");
	http--;
	char tmp[48];
	memset(tmp, 0 ,48);
	memset(tl->chunks->file, 0 ,64);
	int pos = http - buffer -4;
	if(pos > 48)
		LOG("ERROR: not possible to assign new file name\n");
	strncpy(tmp, (buffer+4), pos);
	memset(tl->chunks->file, 0, 64);
	strncpy(tl->chunks->file, tmp, 48);

	printf("+++++++++++++++++tmp: %s\n", tmp);
	printf("---tl->chunks->file: %s\n", tl->chunks->file);
	return;
}

void update_throughput(double alpha, double buck_size, proxy_session_list_t* node, char *ip, struct timespec ts){
	double throughput, duration;
	LOG("Update throughput\n");
	chunk_tracker_list_t* tracker_list = search_seg(node);
	chunk_tracker_t *tracker;
	chunk_tracker_t *tmp_tracker = NULL;
	for(tracker = tracker_list->chunks; tracker->next !=NULL; tracker = tracker->next){
		tmp_tracker = tracker;
	}
	tracker_list->throughput = est_tp(alpha, tracker_list->throughput, ts, buck_size, &throughput, &duration);
	LOG("New throughput is %lf\n", tracker_list->throughput);
	printf("New throughput is %lf\n", tracker_list->throughput);
	if(tmp_tracker != NULL)
		tmp_tracker->next = NULL;
		
	TEST_LOG("%u %lf %lf %lf %d %s %s\n", 
		(unsigned)ts.tv_sec, duration, throughput, tracker_list->throughput, tracker_list->bitrate, ip, tracker->file);
	LOG("free tracker\n");
	return;
}


