#include <cuttlefish-internal.h>

// how much average value should be taken of JPI
#define NUM_AVG_JPI 10
// if optimum frequency not found of any TIPI slab called as UNEXPLORED
#define UNEXPLORED -1

namespace cuttlefish {
// characteristics of each node
typedef struct _table{
        int L; // left bound exploration in core
        int R; // right bound exploration in core
	int slab_count;
        double EDP[AVAIL_FREQ_CORE]; // it holds average JPI value that calculate from multiple slabs for core
	int count[AVAIL_FREQ_CORE];  // it holds number of slabs that find for particular TIPI slab
	double G_EDP[AVAIL_FREQ_CORE]; // it holds the summation of JPI values comes in different slab for core
        int opt_freq_core; // optimum frequency of core
        double exec_time; // discovered time of TIPI slab
        double min; // minimum value of current TIPI slab
        double max; // maximum value of current TIPI slab
	int finish; // number of execution slabs consume in profile optimum core frequency of this tipi

	/************************************ UNCORE******************************/
	int LG; // Guess left bound for exploration in uncore
	int RG; // Guess right bound for exploration in uncore
	int opt_freq_uncore; // optimum frequency of uncore
	int LU; // left bound of exploration in uncore
	int RU;	// right bound of exploration in uncore
	double EDPU[AVAIL_FREQ_UNCORE]; // it holds average JPI value that calculate from multiple slabs for core
	int countU[AVAIL_FREQ_UNCORE];  // it holds number of slabs that find for particular TIPI slab
	double G_EDPU[AVAIL_FREQ_UNCORE]; // it holds the summation of JPI values comes in different slab for uncore
	double exec_time_uncore; // finishing time of TIPI
	int finish_uncore; // number of execution slabs consume in profile optimum uncore frequency of this tipi
}table;

// doubly linked list
typedef struct _Node{
        table *data; // hold information about tipi slab
        struct _Node *prev; // value of previous TIPI should be less than current Node
        struct _Node *next; // value of next TIPI should be more than current Node
}Node;

// find appropriate tipi slab correspond to give TIPI value in parameter
Node *Search(double TIPI);
// generate new node in linked list for given TIPI value
Node * make_node(double TIPI);
// deallocate memory of each tipi slabs inserted in linked list
void free_tipi_node();
// print values of tipi's info
void tipi_value();
// print variable name of tipi's info
void tipi_stats();
}
