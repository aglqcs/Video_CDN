#include "dijkstra.h"

#define LINE_LENGTH 200
#define INT_MAX 9
// global list
list_t *graph_list;

void print_graph(){
	//debug functions
	list_t *p;
	LOG("---------start---------\n");
	for( p = graph_list; p != NULL; p = p->next){
		LOG("%s %d\n", p->node.name.name_char, p->node.seq);
		LOG("%d\n\n", p->node.count_neighbors);
	}
	LOG("---------*end*---------\n");
}

void print_graph_array(int **graph, int V){
	LOG("printing graph\n");
	int i,j;
	for(i = 0; i < V; i ++){
		for(j = 0;j <V ;j ++){
			LOG("%d ",graph[i][j]);
		}
		LOG("\n");
	}
	
}


int graph_init(char *filename){
	LOG("call graph_init()\n");
	graph_list = NULL;

	FILE *fp = fopen( filename, "r");
	if( fp == NULL) {
		LOG("%s\n","error open lsa file");
		return -1;
	}

	char line[LINE_LENGTH];
	while( fscanf(fp, "%s", line) > 0 ) {
		int scanf_ret = 0;
		int exist = 0;

		list_t *new_element = (list_t *)malloc( sizeof( list_t));
		strcpy(new_element->node.name.name_char, line);
		
		list_t *p;
		for( p = graph_list ; p != NULL; p = p->next){
			if( strcmp(p->node.name.name_char, new_element->node.name.name_char) == 0 ){
				exist = 1;
				break;
			}
		}

		memset(line, 0 , LINE_LENGTH);
		if( (scanf_ret = fscanf(fp , "%s", line)) < 0 ){
			LOG("fatal: lsa format error\n");
		}
		new_element->node.seq = atoi(line);

		if( exist && new_element->node.seq < p->node.seq ){
			free(new_element);
			continue;
		}
		if( !exist) {
			new_element->node.count_neighbors = 0;

			memset(line, 0 , LINE_LENGTH);
			if( (scanf_ret = fscanf(fp , "%s", line)) < 0 ){
				LOG("fatal: lsa format error\n");
			}
			char *token = strtok(line, ",");
			while( token) {
				strcpy(new_element->node.neighbors[ new_element->node.count_neighbors].name_char , token);
				new_element->node.count_neighbors ++;
				token = strtok(NULL,",");
			}

			if( graph_list == NULL){
				new_element->next = NULL;
				graph_list = new_element;
			}
			else{
				new_element->next = graph_list;
				graph_list = new_element;
			}
		}
		else{
			p->node.seq = new_element->node.seq;
			free(new_element);
			p->node.count_neighbors = 0;
			memset(line, 0 , LINE_LENGTH);
		  	if( (scanf_ret = fscanf(fp , "%s", line)) < 0 ){
		       	LOG("fatal: lsa format error\n");
		    }
		    char *token = strtok(line, ",");
		    while( token) {
				memset( p->node.neighbors[p->node.count_neighbors].name_char, 0 , LENTH);
		    	strcpy(p->node.neighbors[ p->node.count_neighbors].name_char , token);
	        	p->node.count_neighbors ++;
				token = strtok(NULL,",");
			}
		}
	}
	return 0;
}

int exist_route(int src, int dst){
	// if yes return 1. else return 0.
	if( src == dst) return INT_MAX;
	list_t *p;
	for( p = graph_list; p != NULL; p = p->next){
		if(p->index == dst) break;
	}
	list_t *q;
	for( q = graph_list; q != NULL; q = q->next){
		if( q->index == src){
			int i;
			for(i = 0; i < q->node.count_neighbors; i ++ ){
				if( strcmp(q->node.neighbors[i].name_char , p->node.name.name_char) == 0){
					return 1;
				}
			}
			break;
		}
	}
	return INT_MAX;
}
server_list_t* query_dns(char *src, server_list_t *head){
  	// assign index for the list
  	LOG("src = %s\n", src);
  	list_t *p;
	
	int src_index = -1;
	for( p = graph_list; p != NULL; p = p->next){
		if( strcmp(src, p->node.name.name_char) == 0 ){
			src_index = p->index;
			break;
		}
	}

	if( src_index == -1){
		LOG("fatal: query src does not exist in graph\n");
		//return -1;
	}
	int count;
	int i, j;
	for( p = graph_list, count = 0; p != NULL; p = p ->next,count ++){
		p->index = count;
		LOG("%s -> %d\n", p->node.name.name_char, count);
	}
	int **array = (int **)malloc( count * sizeof(int *));
	for( i = 0; i < count; i ++){
		array[i] = (int *)malloc( count * sizeof(int));
	}

	// init dijkstra array 
	for( i = 0; i < count; i ++){
		for( j = 0 ; j  <count; j  ++){
			array[i][j] = exist_route(i,j);
		}
	}
	//call dijkstra
	int *dist = dijkstra( array, 0, count);
	
	/*
	int min = INT_MAX;
	for( i = 0;i < count; i ++){
		min = min < dist[i] ? min : dist[i];
		LOG("%d ", dist[i]);
	}
	LOG("\n");
	for( i = 0; i < count; i ++){
		if( dist[i] != min) continue;
		for( p = graph_list; p != NULL; p = p->next){
			if( p->index == i) break;
		}
		LOG("searching for %s\n", p->node.name.name_char);
		server_list_t *q;
		for( q = head; q != NULL ; q = q ->next){
			LOG("matching %s\n", q->sname);
			if( strcmp(q->sname, p->node.name.name_char) == 0 ){
				return q;
			}
		}
	}*/
	int min = INT_MAX;
	server_list_t *ret = NULL;
	for( i = 0;i < count ;i ++){
		for( p = graph_list; p != NULL; p = p->next){
			if(p->index == i ) break;
		}
		server_list_t *q;
		for( q = head; q!= NULL; q = q->next){
			LOG("%s matching %s\n", q->sname, p->node.name.name_char);
			if( strncmp(q->sname, p->node.name.name_char, strlen(p->node.name.name_char)) == 0){
				// it is a server
				LOG("server %s dist = %d\n", q->sname, dist[i]);
				if( min > dist[i]) {
					ret = q;
					min = dist[i];
				}
			}
		}
	}
	return ret;
}



int minDistance(int dist[], int sptSet[], int V){
	int min = INT_MAX, min_index;
	int v;
	for(  v = 0; v < V; v++){
		if( sptSet[v] == -1 && dist[v] <= min){
			min = dist[v];
			min_index = v;
		}
	}
	return min_index;
}

int* dijkstra(int **graph, int src, int V){
	 int *dist = (int *)malloc( sizeof(int) * V);
	 int *sptSet = (int *)malloc( sizeof(int) * V);
	 int i,count;
	 for (i = 0; i < V; i++){
        dist[i] = INT_MAX;
		sptSet[i] = -1;
	 }
 
     dist[src] = 0;
 
     for ( count = 0; count < V-1; count++){
	 	int u = minDistance(dist, sptSet, V);
       	sptSet[u] = 1;
       	for (int v = 0; v < V; v++){
			if ( -1 == sptSet[v] && graph[u][v] && dist[u] != INT_MAX && dist[u]+graph[u][v] < dist[v]){
				dist[v] = dist[u] + graph[u][v];
			}
		}
     }
	 free(sptSet);
	 return dist;
}

