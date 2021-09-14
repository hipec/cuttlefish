#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>

namespace hclib {

static long get_usecs (void)
{
   struct timeval t;
   gettimeofday(&t,NULL);
   return t.tv_sec*1000000+t.tv_usec;
}

static long ___start_all, ___end_all;
static long ___start, ___end;
static double ___dur=0;

#define REPORT_TIME	{double _dur_ = ((double)(___end-___start))/1000000; printf("Kernel Execution Time = %fsec\n",_dur_); ___dur+=_dur_;}
#define PRINT_HARNESS_FOOTER { 		\
	___end_all = get_usecs();		\
	double ___dur_all = ((double)(___end_all-___start_all))/1000;		\
	printf("Harness Ended...\n");	\
	printf("============================ MMTk Statistics Totals ============================\n"); \
	printf("time.kernel\n");		\
	printf("%.3f\n",___dur*1000);		\
	printf("Total time: %.3f ms\n", ___dur_all);	\
	printf("------------------------------ End MMTk Statistics -----------------------------\n");	\
	printf("===== TEST PASSED in 0 msec =====\n");	\
}

#define PRINT_HARNESS_HEADER {                                            \
  printf("\n-----\nmkdir timedrun fake\n\n");                                 \
  ___start_all = get_usecs();						\
}
#define START_TIMER	{___start = get_usecs();}
#define END_TIMER	{___end = get_usecs();REPORT_TIME;}

/*template<typename T>
void launch(T &&lambda) {
//    PRINT_HARNESS_HEADER
    lambda();
//    PRINT_HARNESS_FOOTER
}*/

template<typename T>
void kernel(T &&lambda) {
//    START_TIMER
    lambda();
//    END_TIMER
}

}
