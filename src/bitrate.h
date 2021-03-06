#ifndef _BITRAT_H__
#define _BITRAT_H__

#include <time.h>
#include "proxy.h"
#include "log.h"
#include <stdlib.h>
#include <stdio.h>

#define BILLION 1000000000L

typedef struct chunk_tracker{
	char file[64];
	struct chunk_tracker *next;
} chunk_tracker_t;

typedef struct chunk_tracker_list{
	proxy_session_list_t *ps;
	chunk_tracker_t* chunks;
	struct chunk_tracker_list* next;
	int bitrate;
	double throughput;
} chunk_tracker_list_t;


double est_tp(double alpha, double curr_tp, struct timespec ts, double buck_size, double *tpost, double *duration);
chunk_tracker_list_t* create_tracker(char* file, proxy_session_list_t* node);
chunk_tracker_list_t* search_seg(proxy_session_list_t* pl);
void update_bitrate(char* buffer, chunk_tracker_list_t* tl, proxy_session_list_t* node);
void update_throughput(double alpha, double buck_size, proxy_session_list_t* node, char* ip, struct timespec ts);

#endif
