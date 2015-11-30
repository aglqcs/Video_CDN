#include "bitrate.h"

chunk_tracker_list_t *tracker_head = NULL;

// Estimate throughput
double est_tp(double alpha, double curr_tp, time_t ts, time_t tf, double buck_size){
	double new_tp = buck_size / difftime(tf, ts);
	return new_tp * alpha + curr_tp * (1 - alpha); 
}

chunk_tracker_list_t* create_tracker(char* file, proxy_session_list_t* node){
	int seg;
	int frag;
		
	if(2 != sscanf(file, "%*sSeg%d-Frag%d", &seg, &frag)){
		LOG("This is not video chunks, ignore it\n");
		return NULL;
	}

	chunk_tracker_list_t* tracker_list = search_seg(seg, node);
	if(tracker_list == NULL){
		LOG("This is a new chunk, create new list\n");
		tracker_list = (chunk_tracker_list_t *)malloc( sizeof(chunk_tracker_list_t));
		tracker_list->next = NULL;
		tracker_list->ps = node;
		tracker_list->segmentation = seg;
		tracker_list->throughput = 10;
		tracker_list->chunks = NULL;
		if(tracker_head == NULL)
			tracker_head = tracker_list;
		else{
			tracker_list->next = tracker_head;
			tracker_head = tracker_list;
		}
	}

	chunk_tracker_t *new_tracker = (chunk_tracker_t *)malloc( sizeof(chunk_tracker_t));
	new_tracker->fragment = frag;
	new_tracker->start_time = time(NULL);

	if(tracker_list->chunks == NULL){
		tracker_list->chunks = new_tracker;
		tracker_list->chunks->next = NULL;
	}
	else{
		new_tracker->next = tracker_list->chunks;
		tracker_list->chunks = new_tracker;
	} 
	return tracker_list;
}

chunk_tracker_list_t* search_seg(int seg, proxy_session_list_t* pl){
	if(tracker_head == NULL)
		return NULL;
	chunk_tracker_list_t *node;
	for(node=tracker_head; node!=NULL; node=node->next){
		if(seg == node->segmentation && node->ps->session.client_fd == pl->session.client_fd && node->ps->session.server_fd == pl->session.server_fd)
			return node;
	}
	return NULL;
}

void update_bitrate(char* buffer, double throughput){
	char first[8192];
	char second[8192];
	double rate = 10;
	double rates[4] = {10, 50, 100, 500};
	int i;
	for(i=3; i>=0; i--){
		if(rates[i]*1.5 <= throughput){
			rate = rates[i];
			break;
		}
	}
	// Retrieve the value of first and second part of string except bitrate
	sscanf(buffer, "%s%*d%s", first, second);
	// Convert bit rate from it to string
	char bit_rate[5];
	sprintf(bit_rate, "%d", (int)rate);
	strcpy(buffer, "");
	strcat(buffer, first);
	strcat(buffer, bit_rate);
	strcat(buffer, second);
	return;
}

void update_throughput(double alpha, double buck_size, proxy_session_list_t* node, int seg, int frag){
	chunk_tracker_list_t* tracker_list = search_seg(seg, node);
	chunk_tracker_t *tracker;
	chunk_tracker_t *tmp_tracker = NULL;
	for(tracker = tracker_list->chunks; tracker!=NULL; tracker = tracker->next){
		if(tracker->fragment == frag){
			tracker_list->throughput = est_tp(alpha, tracker_list->throughput, tracker->start_time, time(NULL), buck_size);
			tmp_tracker->next = tracker->next;
			free(tracker);
			return;
		}
		tmp_tracker = tracker;
	}
	LOG("update_throughput: Shouldn't come here\n");
}

