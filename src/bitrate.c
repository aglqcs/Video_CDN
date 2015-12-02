#include "bitrate.h"
 
chunk_tracker_list_t *tracker_head = NULL;

// Estimate throughput
double est_tp(double alpha, double curr_tp, struct timeval ts, double buck_size){
	// Calculate time
	struct timeval tval_result, tf;
	gettimeofday(&tf, NULL);
	timersub(&tf, &ts, &tval_result);
	double tdiff = (double)tval_result.tv_sec + (double)tval_result.tv_usec / 1000000000;

	double new_tp = buck_size*8 / tdiff / 1024;
	printf("tdiff: %lf\n", tdiff);
	printf("buck_size: %lf\n", buck_size);
	// LOG("new_tp: %lf\n", new_tp);
	printf("new_tp: %lf\n", new_tp);
	return new_tp * alpha + curr_tp * (1 - alpha); 
}

chunk_tracker_list_t* create_tracker(char* file, proxy_session_list_t* node){
	
	if(strstr(file, "-Frag") == NULL){	
		LOG("This is not video chunks, ignore it\n");
		return NULL;
	}
	/*
	if(3 != sscanf(file, "/vod/%dSeg%d-Frag%d", &init_bitrate, &seg, &frag)){
		LOG("This is not video chunks, ignore it\n");
		return NULL;
	} */

	chunk_tracker_list_t* tracker_list = search_seg(node);
	if(tracker_list == NULL){
		LOG("This is a new chunk, create new list\n");
		tracker_list = (chunk_tracker_list_t *)malloc( sizeof(chunk_tracker_list_t));
		tracker_list->next = NULL;
		tracker_list->ps = node;
		tracker_list->throughput = 10;
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
	gettimeofday(&new_tracker->start_time, NULL);

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
		return NULL;
	}
	chunk_tracker_list_t *node;
	for(node=tracker_head; node!=NULL; node=node->next){
		if(node->ps->session.client_fd == pl->session.client_fd && node->ps->session.server_fd == pl->session.server_fd){
			LOG("search_seg is not NULL\n");
			return node;
		}
	}
	LOG("search_seg is NULL, but tracker_head is not NULL\n");
	return NULL;
}

void update_bitrate(char* buffer, double throughput, proxy_session_list_t* node){
	char first[8192];
	char second[8192];
	double rate = 10;
	int i;
	for(i=9; i>=0; i--){
		if(node->session.bitrate[i] == -1)
			continue;
		if(node->session.bitrate[i]*1.5 <= throughput){
			rate = (double)node->session.bitrate[i];
			break;
		}
	}
	// Retrieve the value of first and second part of string except bitrate
	// sscanf(buffer, "%[] %d %[]", first, &init_bitrate, second);
	char *result = strstr(buffer, "Seg");
	strcpy(second, result);
	while(*result != '/'){
		result--;
	}
	strncpy(first, buffer, (result-buffer+1));
	// Convert bit rate from it to string
	char bit_rate[5];
	sprintf(bit_rate, "%d", (int)rate);
	strcpy(buffer, "");
	strcat(buffer, first);
	strcat(buffer, bit_rate);
	printf("bitrate: %s\n", bit_rate);
	strcat(buffer, second);
	return;
}

void update_throughput(double alpha, double buck_size, proxy_session_list_t* node){
	LOG("Update throughput\n");
	chunk_tracker_list_t* tracker_list = search_seg(node);
	chunk_tracker_t *tracker;
	chunk_tracker_t *tmp_tracker = NULL;
	for(tracker = tracker_list->chunks; tracker->next !=NULL; tracker = tracker->next){
		tmp_tracker = tracker;
	}
	tracker_list->throughput = est_tp(alpha, tracker_list->throughput, tracker->start_time, buck_size);
	LOG("New throughput is %lf\n", tracker_list->throughput);
	printf("New throughput is %lf\n", tracker_list->throughput);
	if(tmp_tracker != NULL)
		tmp_tracker->next = NULL;
		
	//free(tracker);

	LOG("free tracker\n");
	return;
}

