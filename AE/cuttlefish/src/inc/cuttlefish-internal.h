#include <cuttlefish.h>
#include <sys/time.h>
#include <stdint.h> // for int64_t
#include <stdio.h>  // for printf
#include <time.h>   // for time functions
#include <unistd.h> // for ftruncate
#include <stdlib.h>  // for malloc - temp until share memory region allocated
#include <inttypes.h>
#include <assert.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#include <sched.h>
#include <cuttlefish-archConstant.h>

namespace cuttlefish {
void init_internal();
void finalize_internal();
void cf_read();
void show_stats();
void add_log(double tipi , int core_freq , int uncore_freq , double edp , double dur , uint64_t instruct , double pkg_energy, double L3_miss , double TOR_insert , int optimum_core , int optimum_uncore);
double get_exec_time();
void get_timer(double *timer);
/************POLICY FUNCTIONS********************/
int get_core_Freq();
int get_uncore_Freq();
void free_tipi_node();
void tipi_value();
void tipi_stats();
void debug_stats();
void lb_policy(double tipi , double jpi);
/*************MSR ACCESS***************/
int MSRWrite(int cpu, uint32_t reg, uint64_t data);
uint64_t MSRRead(int cpu, uint32_t reg);
/***************FREQUENCY**************/
void set_coreFrequency (int core, int value);
void reset_coreFrequency (int core);
void set_uncoreFrequency(int socket, int value);
void reset_uncoreFrequency(int socket);
uint64_t check_uncoreFrequency(int socket);
void resetFrequency();
void setFrequency();
/*************************************/
}
