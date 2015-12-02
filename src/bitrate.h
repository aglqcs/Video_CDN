#ifndef _BITRAT_H__
#define _BITRAT_H__

#include <sys/time.h>
#include "proxy.h"
#include "log.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct chunk_tracker{
	struct timeval start_time;
	int fragment;
	struct chunk_tracker *next;
} chunk_tracker_t;

typedef struct chunk_tracker_list{
	proxy_session_list_t *ps;
	chunk_tracker_t* chunks;
	struct chunk_tracker_list* next;
	int segmentation;
	double throughput;
} chunk_tracker_list_t;


double est_tp(double alpha, double curr_tp, struct timeval ts, double buck_size);
chunk_tracker_list_t* create_tracker(char* file, proxy_session_list_t* node);
chunk_tracker_list_t* search_seg(int seg, proxy_session_list_t* pl);
void update_bitrate(char* buffer, double throughput);
void update_throughput(double alpha, double buck_size, proxy_session_list_t* node, int seg, int frag);

#endif
