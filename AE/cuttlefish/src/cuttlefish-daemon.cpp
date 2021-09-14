#include <cuttlefish-internal.h>
#include <pthread.h>

namespace cuttlefish {

int sleep_duration=20; //milliseconds
volatile int shutdown = 0;
volatile int start_logging=0;
int cuttlefish_log=0;
struct timespec adaptTimeStart; // used for measured time
struct timespec adaptTimeStop;  // used for measured time
struct timespec adaptTimeTEST;  // used for measured time

double start_timer;
double end_timer;

double start_init_timer;
double end_init_timer;

pthread_t deamon_tid;
// FWD declaration for pthread_create
void *deamon_loop(void *args);

// store logs at each interval
typedef struct _LOG{
  double tipi;
  int core_freq;
  int uncore_freq;
  double edp;
  double duration;
  uint64_t instruction;
  double pkg_energy;
  double L3_miss;
  double Tor_insert;
  int opt_core_freq;
  int opt_uncore_freq;
  struct _LOG *next;
}LOG;

LOG *log_start = NULL;
LOG *log_end = NULL;

void add_log(double tipi , int core_freq , int uncore_freq , double edp , double dur , uint64_t instruct , double pkg_energy, double L3_miss , double TOR_insert , int optimum_core , int optimum_uncore){
  if(!start_logging) return;

  LOG *l = (LOG*) malloc(sizeof(LOG));
  l->tipi = tipi;
  l->edp = edp;
  l->pkg_energy = pkg_energy;
  l->instruction = instruct;
  l->Tor_insert = TOR_insert;
  l->core_freq = core_freq;
  l->uncore_freq = uncore_freq;
  l->duration = dur;
  l->L3_miss = L3_miss;
  l->opt_core_freq = optimum_core;
  l->opt_uncore_freq = optimum_uncore;
  l->next = NULL;
  if(log_start==NULL){
    log_start = l;
    log_end = l;
  }
  else{
    log_end->next = l;
    log_end = l;
  }
}

void append_log(){
  fprintf(stdout,"\n============================ Unprocessed Statistics ============================\n"); 
  fprintf(stdout,"TIPI\tJPI\tUncore1\tUncore2\n");
  LOG *ptr = log_start;
  while(ptr!=NULL){
    //ptr->core_freq holds the uncore_frequency of socket 1, ptr->uncore_freq holds the uncore frequency for socket2
    fprintf(stdout,"%.5f\t%.14f\t%d\t%d\n", ptr->tipi , ptr->edp, ptr->core_freq, ptr->uncore_freq);
    ptr = ptr->next;
  }
  fprintf(stdout,"=============================================================================\n");
  fflush(stdout);
}

void free_log_node(){
  LOG *p = log_start;
  while(p!=NULL){
    LOG *pnext = p->next;
    free(p);
    p=pnext;
  }
}

void log_header(){
  get_timer(&start_init_timer);
  fprintf(stdout,"\n-----\nmkdir timedrun fake\n\n");
  cuttlefish_log=1;
}

void log_footer(){
#if !defined(CORE) && !defined(UNCORE) && !defined(COMBINED)
// we need log only for MOTIVATION ANALYSIS WHERE DAEMON SHOULD NOT BE RUNNING
  assert(getenv("CUTTLEFISH_SHUTDOWN_DAEMON"));
  if(atoi(getenv("CUTTLEFISH_SHUTDOWN_DAEMON")) == 0){
        append_log();
  }
#endif
  show_stats();
#if 1
  debug_stats();
#endif
  free_tipi_node();
  free_log_node();
  finalize_internal();
  get_timer(&end_init_timer);
  fprintf(stdout,"===== Total Time in %.f msec =====\n", (end_init_timer-start_init_timer)*1000);
  fprintf(stdout,"===== Test PASSED in 0.0 msec =====\n");
  fflush(stdout);
}

void init() {
  init_internal();
  if(getenv("CUTTLEFISH_INTERVAL") != NULL) {
    sleep_duration = atoi(getenv("CUTTLEFISH_INTERVAL"));
    assert(sleep_duration>0 && "CUTTLEFISH_INTERVAL should be greater than zero");
  }
  pthread_create(&deamon_tid, NULL, deamon_loop, NULL); 
}

void finalize() {
  shutdown=1;
  pthread_join(deamon_tid, NULL);
  resetFrequency();
  if (cuttlefish_log==0){
    free_tipi_node();
    free_log_node();
    finalize_internal();
  }
}

void start() {
  init();// call init to initialize cuttlefish
  assert(getenv("CUTTLEFISH_SHUTDOWN_DAEMON"));
  if(atoi(getenv("CUTTLEFISH_SHUTDOWN_DAEMON")) == 1){
	shutdown=1;
	cf_read();
  }
  else{
      start_logging = 1;
  }
  get_timer(&start_timer);
}

void stop() {
  get_timer(&end_timer);
  assert(getenv("CUTTLEFISH_SHUTDOWN_DAEMON"));
  if(atoi(getenv("CUTTLEFISH_SHUTDOWN_DAEMON")) == 1){
        cf_read();
  }
  else{
      start_logging = 0;
  }
  finalize();// call finalize to shutdown cuttlefish
}

void bind_deamon_to_core() {
    int nbCPU = sysconf(_SC_NPROCESSORS_ONLN);
    int bind_core = nbCPU-1; //bind to last core as core-0 is for master worker
    int mask = bind_core % nbCPU;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    /* Copy the mask from the int array to the cpuset */
    CPU_SET(mask, &cpuset);

    /* Set affinity */
    int res = sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
    if (res != 0) {
        fprintf(stdout,"ERROR: ");
        if (errno == ESRCH) {
            assert("THREADBINDING ERROR: ESRCH: Process not found!\n");
        }
        if (errno == EINVAL) {
            assert("THREADBINDING ERROR: EINVAL: CPU mask does not contain any actual physical processor\n");
        }
        if (errno == EFAULT) {
            assert("THREADBINDING ERROR: EFAULT: memory address was invalid\n");
        }
        if (errno == EPERM) {
            assert("THREADBINDING ERROR: EPERM: process does not have appropriate privileges\n");
        }
    }
}

void* deamon_loop(void* args) {
  struct timespec interval, remainder;
  interval.tv_sec  =  0;
  interval.tv_nsec = sleep_duration * 1000000;

  // bind to last core
  bind_deamon_to_core();
  while(!shutdown) {
    clock_gettime (CLOCK_MONOTONIC, &adaptTimeStart);

    //Update energy, temperature and instruction related counters
    if(start_logging) {
      cf_read();
      setFrequency(); //works only with policy
    }

    clock_gettime (CLOCK_MONOTONIC, &adaptTimeStop);

    if ((interval.tv_sec > 0) || (interval.tv_nsec > 0)) 
    {
      nanosleep (&interval, &remainder);
    }
  }

  return NULL;
}


void get_timer(double *timer){
  struct timespec currentTime;
  clock_gettime (CLOCK_MONOTONIC, &currentTime);
  *timer = (currentTime.tv_sec + (currentTime.tv_nsec * 10e-10));
}

// return time in millisecond
double get_exec_time(){
  return (end_timer-start_timer)*1000;
}
}
