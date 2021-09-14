/*This file contains functions to set freruency at core level as allowed by the archtiecture
*
*Please check your architecture specification for supported frequency values (/sys/devices/system/cpu/cpu_/cpufreq/scaling_available_frequencies)
*
* Produced at Renaissance Computing Institute
* Written by  Sridutt Bhalachandra, sriduttb@renci.org
*             Allan Porterfield,    akp@renci.org
*/

#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <err.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>  // for itoa

#include <cuttlefish-internal.h>

namespace cuttlefish {


double core_freq=MAX_FREQ_CORE;
double uncore_freq=MAX_FREQ_UNCORE;

/************************
** Writes the required frequency value into the required file provided by the CPUfreq interface
************************/
int freq_write (int cpu, const char* data)
{ 
  int fd;
  char setspeed_file_name[128];
  
  sprintf(setspeed_file_name, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_setspeed", cpu);
  
  fd = open(setspeed_file_name, O_WRONLY);
  if (fd < 0)
  { 
    if (errno == ENXIO) 
    {
      fprintf(stderr, "wrfreq: No CPU %d\n", cpu);
      return -1;
    } 
    else if (errno == EIO) 
    {
      fprintf(stderr, "wrfreq: CPU %d doesn't support MSRs\n", cpu);
      return -1;
    } 
    else 
    {
      perror("wrfreq: ");
      return -1;
    }
  }
  
  if (pwrite(fd, data, sizeof data, fd) != sizeof data) 
  {
    if (errno == EIO) 
    {
      fprintf(stderr, "wrfreq: CPU %d cannot set Freq to  %s\n", cpu, data);
      return(4);
    } 
    else 
    {
      perror("wrfreq: pwrite");
      return(127);
    }
  }

  close(fd);
  
  return(0);
}

/************************
** Sets the required frequency value for a core
************************/
void set_coreFrequency (int core, int value) 
{
  char setFrequencyValue[8];
  int baseClockFrequency = 100000; // in kHz (100 MHz)
  int totalCores = SOCKETSperNODE * CORESperSOCKET;
  value *= baseClockFrequency;

  sprintf(setFrequencyValue, "%d", value);
  freq_write (core % totalCores, setFrequencyValue);
  freq_write ((core % totalCores) + totalCores, setFrequencyValue); // Hyperthread
  //printf("\ncore=%d hyperthread=%d value=%s", core % totalCores, (core % totalCores) + totalCores, setFrequencyValue);
}

/************************
** Resets the frequency of a core to default
************************/
void reset_coreFrequency (int core)
{
  int totalCores = SOCKETSperNODE * CORESperSOCKET;
  char tmp[10];
  sprintf(tmp, "%d*100000", MAX_FREQ_CORE);
  freq_write (core % totalCores, tmp); // Max Non-Turbo frequency for Cheddar in kHz
  freq_write ((core % totalCores) + totalCores, tmp); // Hyperthread
}


//possible values of turbo freq are from MIN_FREQ_UNCORE and MAX_FREQ_UNCORE on haswell

void set_uncoreFrequency(int socket, int value)
{
  if(value<MIN_FREQ_UNCORE || value>MAX_FREQ_UNCORE){
    fprintf(stdout, "wrong freq\n");
    fflush(stdout);
    return;
  }
  int reg_value=value|(value<<8);
  MSRWrite(socket, MSR_UNCORE_FREQ, reg_value);  
}

// Reset duty cycle of a core to Max/default
void reset_uncoreFrequency(int socket)
{
  int value = MAX_FREQ_UNCORE;
  int reg_value=value|(value<<8);
  MSRWrite(socket, MSR_UNCORE_FREQ, reg_value);
  //MSRWrite(socket, MSR_UNCORE_FREQ, 0xc1e);
}

// Check current dutycycle value
uint64_t check_uncoreFrequency(int socket)
{
  return (MSRRead(socket, MSR_UNCORE_READ));
}

void resetFrequency(){
#if defined(CORE) || defined(COMBINED)
	for(int core=0; core<NUM_WORKERS; core++)
        	reset_coreFrequency(core);
#endif
#if defined(UNCORE) || defined(COMBINED)
        for(int sock=0; sock<SOCKETSperNODE; sock++)
        	reset_uncoreFrequency(sock);
#endif
        return;
}

void setFrequency(){
#if defined(CORE) || defined(COMBINED)
        if(core_freq!=get_core_Freq()){
                core_freq = get_core_Freq();
                for(int core=0; core<NUM_WORKERS; core++)
                        set_coreFrequency(core , core_freq);
        }
#endif
#if defined(UNCORE) || defined(COMBINED)
        if(uncore_freq!=get_uncore_Freq()){
                uncore_freq = get_uncore_Freq();
                for(int sock=0; sock<SOCKETSperNODE; sock++)
                        set_uncoreFrequency(sock , uncore_freq);
        }
#endif
	return;
}

}
