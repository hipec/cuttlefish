#include <cuttlefish-policy.h>

namespace cuttlefish {

Node *head = NULL; // front node in linked list of TIPI slabs
Node *rear = NULL; // back/rear node in linked list of TIPI slabs

// deallocate the memory associated with TIPI linked list
void free_tipi_node(){
	Node *p = head;
        while(p!=NULL){
		Node *pnext = p->next;
	        free(p);
        	p=pnext;
	}
}

// print attributes of each TIPI node
void tipi_value(){
	Node *p = head;
	int total_slabs=0;
	if(p!=NULL){
	int distinct_slabs=0;
	int opt_core_slabs=0;
	int opt_uncore_slabs=0;
	double range_l=p->data->min;
	double range_r=-1;
	while(p!=NULL){
		if(p->next==NULL)
			range_r=p->data->max;
		total_slabs+=p->data->slab_count;
		distinct_slabs+=1;
		if(p->data->opt_freq_core!=UNEXPLORED){
			opt_core_slabs+=1;
		}
		if(p->data->opt_freq_uncore!=UNEXPLORED && p->data->opt_freq_uncore!=0){
                        opt_uncore_slabs+=1;
                }
		p=p->next;
	}
	fprintf(stdout , "%f\t%f\t%d\t%d\t%d\t" , range_l, range_r,  distinct_slabs , opt_core_slabs , opt_uncore_slabs);
        p=head;
	}
	while(p!=NULL){
		/* optimum core and optimum uncore frequency
		   of each TIPI slabs in linked list
		*/
		if(((double)p->data->slab_count/(double)total_slabs)>0.05){
                	fprintf(stdout , "%f\t%d\t%d\t%d\t%d\t" , ((double)p->data->slab_count/(double)total_slabs), p->data->opt_freq_core , p->data->opt_freq_uncore , p->data->finish , p->data->finish_uncore);
                }
		p = p->next;
        }
        fflush(stdout);

}

void tipi_stats(){
	Node *p = head;
	int total_slabs=0;
	if(p!=NULL){
        while(p!=NULL){
                total_slabs+=p->data->slab_count;
		p=p->next;
        }
        p=head;
	fprintf(stdout , "%s\t%s\t%s\t%s\t%s\t" , "RANGE_L", "RANGE_R"  ,"DISTINCT_SLABS" , "OPT_CORE_SLABS" , "OPT_UNCORE_SLABS");
	}
	while(p!=NULL){
		/* print some variable names in TIPI's info table
		*/
		if(((double)p->data->slab_count/(double)total_slabs)>0.05){
		char c_str[50];
        	char u_str[50];
		char ct_str[50];
		char ut_str[50];
		char perc[50];
        	sprintf(c_str, "%.5f", p->data->min);
        	strcat(c_str , "_");
        	sprintf(c_str+8 , "%.5f" , p->data->max);
        	strcpy(u_str , c_str);
		strcpy(ct_str , c_str);
		strcpy(ut_str , c_str);
		strcpy(perc , c_str);
        	strcat(c_str , "_CF");
        	strcat(u_str , "_UF");
		strcat(ct_str , "_CT");
		strcat(ut_str , "_UT");
		strcat(perc , "_PERC");
        	fprintf(stdout , "%s\t%s\t%s\t%s\t%s\t" ,perc, c_str , u_str , ct_str , ut_str);
		}
		p = p->next;
	}
	fflush(stdout);
}

void debug_stats(){

	fprintf(stdout,"\n============================ TIPI SLAB Statistics ============================\n");
        Node *p = head;
        while(p!=NULL){
		if(1){
		fprintf(stdout,"\n================================= TIPI =======================================\n");
                fprintf(stdout, "TIPI SLAB\t %f - %f",p->data->min , p->data->max);
		fprintf(stdout,"\n============================ CORE Statistics ============================\n");
        	fprintf(stdout, "L\t %d\n" , p->data->L);
        	fprintf(stdout, "R\t %d\n" , p->data->R);
        	fprintf(stdout, "OPTIMUM CORE FREQUENCY\t %d\n" , p->data->opt_freq_core);
        	for(int j=0;j<AVAIL_FREQ_CORE;j++){
                	fprintf(stdout, "JPI[%d]\t" , j);
        	}
		fprintf(stdout,"\n");
		for(int j=0;j<AVAIL_FREQ_CORE;j++){
                        fprintf(stdout, "%.10f\t" , p->data->EDP[j]);
                }
		fprintf(stdout,"\n============================ UNCORE Statistics ============================\n");
		fprintf(stdout, "LU\t %d\n" , p->data->LU);
                fprintf(stdout, "RU\t %d\n" , p->data->RU);
		fprintf(stdout, "LG\t %d\n" , p->data->LG);
                fprintf(stdout, "RG\t %d\n" , p->data->RG);
                fprintf(stdout, "OPTIMUM UNCORE FREQUENCY\t %d\n" , p->data->opt_freq_uncore);
                for(int j=0;j<AVAIL_FREQ_UNCORE;j++){
                        fprintf(stdout, "JPIU[%d]\t" , j);
                }
                fprintf(stdout,"\n");
                for(int j=0;j<AVAIL_FREQ_UNCORE;j++){
                        fprintf(stdout, "%.10f\t" , p->data->EDPU[j]);
                }
		fprintf(stdout,"\n==============================================================================\n");
		}
        	p = p->next;
    	}
	fprintf(stdout,"\n==============================================================================\n");
	fflush(stdout);

}


// Allocate memory to new TIPI node which does not exist in linked list
Node * make_node(double TIPI){
	const double tipi_inv= TIPI_INTERVAL;
	//printf("TIPI_INTERVAL: %f\n" , tipi_inv);
	double Min = (int)(TIPI/tipi_inv)*(tipi_inv); // find low bound of TIPI slab
	double Max = Min + tipi_inv; // find right bound of TIPI slab
	//fprintf(stdout, "inv1 %f inv2 %f TIPI %f INV %f Max %f Min %f\n" , TIPI_INTERVAL , TIPI_SUB_INTERVAL , TIPI , tipi_inv , Min , Max);
	Node *ptr = (Node *) malloc(sizeof(Node)); // alocate memory to hold pointers of neighbour TIPI slab
	ptr->data = (table*) malloc(sizeof(table)); // alocate memory to hold information about TIPI slab
	ptr->data->L = MIN_FREQ_CORE; //set left bound of core exploration with minimum core frequency
	ptr->data->R = MAX_FREQ_CORE; //set right bound of core exploration with maximum core frequency
	memset(ptr->data->EDP , 0.0 , sizeof(ptr->data->EDP)); // initialize with 0
	memset(ptr->data->count, 0 , sizeof(ptr->data->count)); // initialize with 0
	memset(ptr->data->G_EDP , 0.0 , sizeof(ptr->data->G_EDP)); // initialize with 0
	ptr->data->opt_freq_core = UNEXPLORED;
	ptr->data->exec_time = UNEXPLORED;
	ptr->data->finish = 0;
	ptr->data->LU = MIN_FREQ_UNCORE; //set left bound of uncore exploration with minimum uncore frequency
	ptr->data->RU = MAX_FREQ_UNCORE; //set right bound of uncore exploration with maximum uncore frequency
	ptr->data->opt_freq_uncore = 0;  //set to zero untill it is not discovered
	ptr->data->exec_time_uncore = 0; //set to zero untill it is not discovered
	ptr->data->finish_uncore=0;
	memset(ptr->data->EDPU , 0.0 , sizeof(ptr->data->EDPU));
	memset(ptr->data->countU, 0 , sizeof(ptr->data->countU));
	memset(ptr->data->G_EDPU , 0.0 , sizeof(ptr->data->G_EDPU));
	ptr->data->min = Min; // set to low bound
	ptr->data->max = Max; // set to high bound
	ptr->prev = NULL;
	ptr->next = NULL;
	ptr->data->slab_count=0;
    	return ptr;
}

// find wheather TIPI slab exist in linked list or not
// set the exploration space of new Node based on adjacent node's exploration space
Node *Search(double TIPI){
	if(head == NULL){
		/* initially linked lis empty
		  it will generated node for new tipi value and
		  insert into linked list as a front node
		*/
		Node *ptr = make_node(TIPI);
		head = ptr;
		rear = ptr;
		return ptr;
	}
	else{
		if(TIPI < head->data->min){ /*this tipi is compute bound relative to the head tipi in list
					     *i.e. there is no TIPI slab exist for this tipi value
					     */
			Node *ptr = make_node(TIPI);
			ptr->next = head;
			head->prev = ptr;
			ptr->data->L = head->data->L; // bound L part
			if(head->data->opt_freq_core!=UNEXPLORED){
                                /* As this tipi is compute bound relative to current tipi,
                                 * it will definitely have faster optimal core frequency
                                 * than the optimal core frequency of current tipi.
                                 */
				ptr->data->L = head->data->opt_freq_core; // bound L part
			}
			head = ptr;
			return head;
		}
		else if(TIPI > rear->data->max){/*this is a memory bound tipi than what we have so far*/
			Node *ptr = make_node(TIPI);
			ptr->prev = rear;
			rear->next = ptr;
			ptr->data->R = rear->data->R; // bound R part
			if(rear->data->opt_freq_core!=UNEXPLORED){
                                /*
                                 * As this tipi is memory bound relative to current tipi,
                                 * it will definitely have slower optimal core frequency
                                 * than the optimal core frequency of current tipi.
                                 */
				ptr->data->R = rear->data->opt_freq_core; // bound L part
			}
			rear = ptr;
			return rear;
		}
		else{/*insert this tipi in list*/
			Node *p = head;
			while(p!=NULL){
				if(TIPI <= p->data->max){
					/* already TIPI slab exit for current TIPI value
					*/
					break;
				}
				else{
					if(TIPI < p->next->data->min){
						Node *ptr = make_node(TIPI);
						ptr->next = p->next;
						ptr->prev = p;
						p->next->prev = ptr;
						p->next = ptr;
                                                /* 
                                                 * This tipi is compute bound relative to next tipi, and memory
                                                 * bound relative to the prev tipi. Hence, its optimal core frequency
                                                 * would be in between the optimal core of next tipi (L) and
                                                 * optimal core of prev tipi (R)
                                                 */
						if(p->data->opt_freq_core!=UNEXPLORED && ptr->next->data->opt_freq_core!=UNEXPLORED){
							/*
							 *set R value to optimum frequency of left TIPI slab which have low tipi range relative to current tipi value
							 *set L value to optimum frequeny of right TIPI slab which have high tipi range relative to current tipi value
							 */
							ptr->data->R = p->data->opt_freq_core;
							ptr->data->L = ptr->next->data->opt_freq_core;
						}
						else if(p->data->opt_freq_core!=UNEXPLORED){
							/*
                                                         *set R value to optimum frequency of left TIPI slab which have low tipi range relative to current tipi value
                                                         *set L value to left bound of right TIPI slab which have high tipi range relative to current tipi value
                                                         */

							ptr->data->R = p->data->opt_freq_core;
							ptr->data->L = ptr->next->data->L;
						}
						else if(ptr->next->data->opt_freq_core!=UNEXPLORED){
							/*
                                                         *set R value to right bound of left TIPI slab which have low tipi range relative to current tipi value
                                                         *set L value to optimum frequeny of right TIPI slab which have high tipi range relative to current tipi value
                                                         */

							ptr->data->R = p->data->R;
							ptr->data->L = ptr->next->data->opt_freq_core;
						}
						else{
							/*
                                                         *set R value to right bound of left TIPI slab which have low tipi range relative to current tipi value
                                                         *set L value to left bound of right TIPI slab which have high tipi range relative to current tipi value
                                                         */

							ptr->data->R = p->data->R;
							ptr->data->L = ptr->next->data->L;
						}
						p = ptr;
						break;
					}
					else{
						p = p->next;
					}
				}
			}
			return p;
		}
	}
}
}
