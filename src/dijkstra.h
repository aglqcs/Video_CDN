#ifndef _DIJKSTRA_H_
#define _DIJKSTRA_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "log.h"
#include "nameserver.h"

#define LENTH 50

typedef struct Name{
	char name_char[LENTH];
} Name_t;

typedef struct graph_node{
	Name_t name;
	int seq;
	int count_neighbors;
	Name_t neighbors[LENTH];
} graph_node_t;

typedef struct list{
	int index;
	graph_node_t node; 
	struct list *next;
}list_t;

int graph_init(char *);
int* dijkstra(int **,int, int);
server_list_t* query_dns(char *src, server_list_t *head);
#endif
