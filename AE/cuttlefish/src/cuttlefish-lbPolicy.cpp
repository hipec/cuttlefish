#include <cuttlefish-policy.h>
#include <math.h>
namespace cuttlefish {

static int counter = 0; // policy iteration
static int global_current_freq_core=MAX_FREQ_CORE; // current core freq set by policy
static int global_current_freq_uncore=MAX_FREQ_UNCORE; // current uncore freq set by policy
static double last_tipi_slab=UNEXPLORED; // store last tipi slab

// return current core frequency
int get_core_Freq(){
	return global_current_freq_core;
}

// return current uncore frequency
int get_uncore_Freq(){
	return global_current_freq_uncore;
}


// Predict the exploration space for searching uncore optimum based on core optimum found
// Range of exploration depends on avaibility of FREQUENCY RANGE in Architecture
void guess_uncore_frequency(Node *n){
#if defined(UNCORE)
// Only for uncore policy ,exploration space should be equal to default Frequency range
	n->data->LU = MIN_FREQ_UNCORE;
	n->data->RU = MAX_FREQ_UNCORE;
#else
	/*
	 * guess optimum uncore frequency based on optimum core frequency which is already found at this stage
	 * then, decide uncore exploration space based on guess frequency
	 * always uncore exploration space should be near around guess frequency
	 */
	// exploration space for searching optimum uncore frequency
	int range = (int)round((double)AVAIL_FREQ_UNCORE/(double)(AVAIL_FREQ_CORE)) * 4;
	// alpha is a factor that tells us how to project core optimum on available uncore frequency range
	double alpha = (double)(MAX_FREQ_UNCORE - MIN_FREQ_UNCORE) / (double)(MAX_FREQ_CORE - MIN_FREQ_CORE);
	// it is sort of equation of line y - y1 = alpha * (x - x1) which comes from trend observe between core and uncore
	int guess_freq = round(MAX_FREQ_UNCORE - (alpha * (double)(n->data->opt_freq_core - MIN_FREQ_CORE)));
	// set left and right bound for uncore exploration with the help of range variable we decided
	n->data->LU = (MIN_FREQ_UNCORE > (guess_freq - range/2)) ? (int)MIN_FREQ_UNCORE : (int)(guess_freq - range/2);
	n->data->RU = (MAX_FREQ_UNCORE < (guess_freq + range/2)) ? (int)MAX_FREQ_UNCORE : (int)(guess_freq + range/2);

	if(MAX_FREQ_UNCORE - guess_freq <= range/2){
		n->data->LU = n->data->LU - (guess_freq + range/2 - MAX_FREQ_UNCORE);
	}
	if(guess_freq - MIN_FREQ_UNCORE <= range/2){
		n->data->RU = n->data->RU + (MIN_FREQ_UNCORE - (guess_freq - range/2));
	}
	//fprintf(stdout, "optimum core %d estimated optimum uncore %d Left bound of uncore %d Right bound of uncore %d\n", n->data->opt_freq_core , guess_freq , n->data->LU , n->data->RU);
#endif
	// optimization 3 for uncore
	if(n->prev!=NULL){
		if(n->prev->data->opt_freq_uncore!=0){ // discovered
			if(n->prev->data->opt_freq_uncore!=UNEXPLORED){ //explored
				n->data->LU =  n->prev->data->opt_freq_uncore;
			}
			else{
				n->data->LU = n->prev->data->LU;
			}
		}
	}
	if(n->next!=NULL){
		if(n->next->data->opt_freq_uncore!=0){ // discovered
			if(n->next->data->opt_freq_uncore!=UNEXPLORED){ // explored
				n->data->RU =  n->next->data->opt_freq_uncore;
			}
			else{
				n->data->RU = n->next->data->RU;
			}
		}
	}

}

// Searching uncore optimum frequency based search policy
void search_uncore_optimum(Node *n, int freq , double edp){
	if(n->data->opt_freq_uncore==0){ // UNCORE part is not discovered
		// set the parameters when uncore exploration starts
		n->data->opt_freq_uncore = UNEXPLORED;
		n->data->exec_time_uncore = UNEXPLORED;
		guess_uncore_frequency(n);
		n->data->LG=n->data->LU;
		n->data->RG=n->data->RU;
	}
	//calculation of average value of JPI
	if(global_current_freq_core == n->data->opt_freq_core && last_tipi_slab==n->data->min){
	// enter if TIPI will same in previous and current execution slab otherwise discard the current JPI value
		n->data->G_EDPU[freq-MIN_FREQ_UNCORE] += edp;
		n->data->countU[freq-MIN_FREQ_UNCORE] += 1;
		n->data->EDPU[freq-MIN_FREQ_UNCORE] = (n->data->G_EDPU[freq-MIN_FREQ_UNCORE]) / (n->data->countU[freq-MIN_FREQ_UNCORE]);
	}

	if(n->data->opt_freq_uncore != UNEXPLORED){
	// if optimum frequency found
		global_current_freq_uncore = n->data->opt_freq_uncore;
		return;
    	}

	int left_bound = n->data->LU;
	int right_bound = n->data->RU;
	if(right_bound - left_bound < 2){ //optimization 2 of uncore
	// if exploration space is left less than 2 then set optimum frequency based on memory intense tipi
		if(n->data->RU < MID_FREQ_UNCORE){
			n->data->opt_freq_uncore = n->data->LU; // always set to left value for compute intensive TIPI
		}
		else{
			n->data->opt_freq_uncore = n->data->RU; // always set to right value for memory intensive TIPI 
		}
		n->data->exec_time_uncore = counter;
		global_current_freq_uncore = n->data->opt_freq_uncore;
		return;
	}

	int next_explore_frequency = right_bound -2;
	double right_edp = n->data->EDPU[right_bound - MIN_FREQ_UNCORE]; // Read the JPI value at RIGHT bound of TIPI
	int right_count = n->data->countU[right_bound - MIN_FREQ_UNCORE]; // Read the count that tells how many time JPI has come on same frequency for same TIPI slab

	double next_freq_edp = n->data->EDPU[next_explore_frequency - MIN_FREQ_UNCORE];//JPI value at RIGHT bound - 2 of current TIPI
	int next_freq_count = n->data->countU[next_explore_frequency - MIN_FREQ_UNCORE];// Read the count that tells how many time JPI has come on same frequency for same TIPI slab

	if(right_edp!=0.0 && next_freq_edp!=0.0 && right_count>=NUM_AVG_JPI && next_freq_count>=NUM_AVG_JPI){
	// if both frequency has been explored
		int iter=0;
		while(right_edp!=0.0 && next_freq_edp!=0.0 && right_count>=NUM_AVG_JPI && next_freq_count>=NUM_AVG_JPI && right_bound!=next_explore_frequency){ // optimization 1
		// explore till JPI found at both frequency right bound and right bound - 2
			iter++;
			if(right_edp < next_freq_edp){
				// right edp is better
				// so fix left bound with next_freq
				n->data->LU = next_explore_frequency + 1;
				Node *ptr_iter = n;
				while(ptr_iter->next!=NULL){ // optimization 4
					// set the Left bound of all TIPI slab in right neighbours
					ptr_iter->next->data->LU = next_explore_frequency + 1;
					if(ptr_iter->data->LU > ptr_iter->data->RU){
						ptr_iter->data->LU = ptr_iter->data->RU;
						ptr_iter->data->opt_freq_uncore=ptr_iter->data->LU;
					}
					ptr_iter = ptr_iter->next;
				}

				global_current_freq_uncore = n->data->LU; // predicted uncore frequency for next slab
			}
			else{
				// next smaller edp is better
				// so fix right bound with next_freq
				n->data->RU = next_explore_frequency;
				Node *ptr_iter = n;
				while(ptr_iter->prev!=NULL){//optimization 4
					// set the Right bound of all TIPI slab in left neighbours
					ptr_iter->prev->data->RU = next_explore_frequency;
					if(ptr_iter->data->LU > ptr_iter->data->RU){
						ptr_iter->data->RU = ptr_iter->data->LU; 
						ptr_iter->data->opt_freq_uncore=ptr_iter->data->LU;
					}
					ptr_iter = ptr_iter->prev; // check assertion for each node
				}

				global_current_freq_uncore = ((n->data->RU - 2) >= n->data->LU) ? n->data->RU - 2 : n->data->LU; // predicted uncore frequency for next slab
				right_bound = next_explore_frequency;

			}

			next_explore_frequency = global_current_freq_uncore; 
			// set opt_freq_core to right_bound because of diffrence <1 set next_explore_frequency = right_bound
			if(n->data->RU - n->data->LU < 2){
				if(n->data->RU < MID_FREQ_UNCORE){
					next_explore_frequency = n->data->LU; // always set to left value for compute intensive TIPI
				}
				else{
					next_explore_frequency = n->data->RU; // always set to right value for memory intensive TIPI 
				}
			}

			right_edp = n->data->EDPU[right_bound - MIN_FREQ_UNCORE];
			right_count = n->data->countU[right_bound - MIN_FREQ_UNCORE];
			next_freq_edp = n->data->EDPU[next_explore_frequency - MIN_FREQ_UNCORE];
			next_freq_count = n->data->countU[next_explore_frequency - MIN_FREQ_UNCORE];
		}
		if(right_bound==next_explore_frequency){
		// if right bound is unexplored
			n->data->opt_freq_uncore = right_bound;
			n->data->exec_time_uncore = counter;
			global_current_freq_uncore = right_bound; // set the frequency for upcoming slab
			return;
		}
		else{
		// if next lower frequency at step of 2 is unexplored
			// check assertion for both edp
			global_current_freq_uncore = next_explore_frequency; // set the frequency for upcoming slab
			return;
		}
	}
	else if(right_count >=NUM_AVG_JPI){
	// enters until right bound-2 is not explored NUM_AVG_JPI times
		global_current_freq_uncore = next_explore_frequency;
		return;
	}
	else{
	// enters untill right bound of current TIPI slab is not explored NUM_AVG_JPI times
		global_current_freq_uncore = right_bound;
		return;
	}


}

// Searching core optimum frequency based search policy
void search_core_optimum(Node *n, int freq , double edp){
#if defined(UNCORE)
	// for only uncore policy set optimum core frequency with Maximum value of core frequency
	n->data->opt_freq_core = MAX_FREQ_CORE;
#else


	// calculation of avg EDP
	if(global_current_freq_uncore==MAX_FREQ_UNCORE && last_tipi_slab==n->data->min){ //TIPI transition slab can have anamolus JPI value hence we discard the transition slab
		// enter if TIPI will same in previous and current execution slab otherwise discard the current JPI value
		n->data->G_EDP[freq-MIN_FREQ_CORE] += edp;
		n->data->count[freq-MIN_FREQ_CORE] += 1;
		n->data->EDP[freq-MIN_FREQ_CORE] = (n->data->G_EDP[freq-MIN_FREQ_CORE]) / (n->data->count[freq-MIN_FREQ_CORE]);
	}

#endif
	// if TIPI already explored it's optimum frequency
	if(n->data->opt_freq_core != UNEXPLORED){
#if defined(UNCORE)
	    search_uncore_optimum(n, global_current_freq_uncore, edp);
#endif

#if defined(CORE)
	n->data->opt_freq_uncore = MAX_FREQ_UNCORE;
#endif

#if defined(COMBINED)
		// in combined policy uncore exploration starts once core optimum will set
		search_uncore_optimum(n, global_current_freq_uncore, edp);
#endif
		global_current_freq_core=n->data->opt_freq_core;
		return;
	}
#if defined(COMBINED)
	// in combined policy, always set uncore frequency with maximum value of uncore frequency untill core optimum will set
	global_current_freq_uncore = n->data->RU;
#endif
	int left_bound = n->data->L;
	int right_bound = n->data->R;

	if(right_bound - left_bound < 2){ //Optimization 2
		// if exploration space is left less than 2 then set optimum frequency based on memory intense tipi
		if(n->data->R < MID_FREQ_CORE)
			n->data->opt_freq_core = n->data->L; // always set to left value for memory intensive TIPI
		else
			n->data->opt_freq_core = n->data->R; // always set to right value for compute intensive TIPI 
		n->data->exec_time = counter;
		global_current_freq_core=n->data->opt_freq_core;
		return;
	}

	// if optimum frequency is unexplored
	int next_explore_frequency = right_bound - 2; // select the lower frequency at step of 2 from right bound
	double right_edp = n->data->EDP[right_bound - MIN_FREQ_CORE]; // Read the JPI value at RIGHT bound of TIPI
	int right_count = n->data->count[right_bound - MIN_FREQ_CORE];// Read the count that tells how many time JPI has come on same frequency for same TIPI slab

	double next_freq_edp = n->data->EDP[next_explore_frequency - MIN_FREQ_CORE]; // Read the JPI value at RIGHT bound-2 of current TIPI
	int next_freq_count = n->data->count[next_explore_frequency - MIN_FREQ_CORE];// Read the count that tells how many time JPI has come on same frequency for same TIPI slab

	if(right_edp != 0.0 && next_freq_edp != 0.0 && right_count >= NUM_AVG_JPI && next_freq_count >= NUM_AVG_JPI){
		// if both frequency has been explored
		int iter=0;
		while(right_edp != 0.0 && next_freq_edp != 0.0 && right_count >= NUM_AVG_JPI && next_freq_count >= NUM_AVG_JPI && right_bound != next_explore_frequency){ // optimization 1
			// explore till JPI found at both frequency right bound and next explore frequency(right bound - 2)
			iter++;
			if(right_edp < next_freq_edp){
				// right edp is better
				// so fix left bound with next_freq
				n->data->L = next_explore_frequency + 1;
				Node *ptr_iter = n;
				while(ptr_iter->prev!=NULL){ // optimization 4
					// set the Left bound of all TIPI slab in left  neighbours
					ptr_iter->prev->data->L = next_explore_frequency + 1;
					if(ptr_iter->data->L > ptr_iter->data->R){
						ptr_iter->data->R = ptr_iter->data->L; 
						ptr_iter->data->opt_freq_core=ptr_iter->data->L;
					}
					ptr_iter = ptr_iter->prev;
				}
				global_current_freq_core = n->data->L;// predicted core frequency for next slab
			}
			else{
				// next smaller edp is better
				// so fix right bound with next_freq
				n->data->R = next_explore_frequency;
				Node *ptr_iter = n;
				while(ptr_iter->next!=NULL){ // optimization 4
					// set the Right bound of all TIPI slab in right neighbours
					ptr_iter->next->data->R = next_explore_frequency;
					if(ptr_iter->data->L > ptr_iter->data->R){
						ptr_iter->data->L = ptr_iter->data->R; 
						ptr_iter->data->opt_freq_core=ptr_iter->data->L;
					}
					ptr_iter = ptr_iter->next; // check assertion for each node
				}
				global_current_freq_core = ((n->data->R - 2) >= n->data->L) ? n->data->R - 2 : n->data->L; // predicted core frequency for the next slab
				right_bound = next_explore_frequency;
			}

			next_explore_frequency = global_current_freq_core;
			// set opt_freq_core to right_bound because of diffrence <1 set next_explore_frequency = right_bound
			if(n->data->R - n->data->L < 2){
				if(n->data->R < MID_FREQ_CORE){ 
					next_explore_frequency = n->data->L; // always set to left value for memory intensive TIPI
				}
				else{
					next_explore_frequency = n->data->R; // always set to right value for compute intensive TIPI 
				}
			}

			right_edp = n->data->EDP[right_bound-MIN_FREQ_CORE];
			right_count = n->data->count[right_bound-MIN_FREQ_CORE];
			next_freq_edp = n->data->EDP[next_explore_frequency-MIN_FREQ_CORE];
			next_freq_count = n->data->count[next_explore_frequency-MIN_FREQ_CORE];
		}
		if(right_bound==next_explore_frequency){
			// if right bound is unexplored
			n->data->opt_freq_core = right_bound;
			n->data->exec_time = counter;
			global_current_freq_core = right_bound; // set the core frequency for next upcoming slab
			return;
		}
		else{
			// if next lower frequency at step of 2 is unexplored
			global_current_freq_core = next_explore_frequency; // set the core frequency for next upcoming slab
			return;
		}
	}
	else if(right_count >=NUM_AVG_JPI){
		// enters until right bound-2 is not explored NUM_AVG_JPI times
		global_current_freq_core = next_explore_frequency;
		return;
	}
	else{
		//enters untill right bound is not explored NUM_AVG_JPI times
		global_current_freq_core = right_bound;
		return;
	}

}


int sleep_dur=20;
int discard_duration=2000;
// main policy function
void lb_policy(double tipi , double jpi){
	if(counter==0){
		if(getenv("CUTTLEFISH_INTERVAL") != NULL) {
		        sleep_dur = atoi(getenv("CUTTLEFISH_INTERVAL"));
    			assert(sleep_dur>0 && "CUTTLEFISH_INTERVAL should be greater than zero");
  		}
	}
        counter +=1;
	// Discarded TOTAL_SLABS_TO_DISCARD_ATSTART execution slab because of unstability in JPI values
	if(counter < discard_duration/sleep_dur){
		return;
	}

        Node *n = Search(tipi); // find appropriate tipi node in which tipi value lies
	n->data->slab_count+=1;
#if defined(CORE) || defined(COMBINED)
	if(n->data->exec_time==UNEXPLORED){
		n->data->finish++; //count in how many slabs optimal core was found for this TIPI
	}
#endif
#if defined(UNCORE) || defined(COMBINED)
	if(n->data->exec_time_uncore==UNEXPLORED){
		n->data->finish_uncore++; //count in how many slabs optimal uncore was found for this TIPI
	}
#endif
        int freq = global_current_freq_core; //current core frequency of machine
	//start exploration from core frequency
	search_core_optimum(n , freq , jpi); // start exploration from core side first
	last_tipi_slab = n->data->min;
}

}
