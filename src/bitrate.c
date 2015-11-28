#include "bitrate.h"

// Estimate throughput
double est_tp(double alpha, double curr_tp, time_t ts, time_t tf, double buck_size){
	double new_tp = buck_size / difftime(tf, ts);
	return new_tp * alpha + curr_tp * (1 - alpha); 
}


