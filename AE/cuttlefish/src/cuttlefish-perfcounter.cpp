#include <cuttlefish-internal.h>

namespace cuttlefish {

int PREV_TIPI_SLAB=-1;
int cf_read_counter=0;

int maxCounters=16;
int counterNum = 0;  // initialize number of counter
int flag = 0; // intialize all counters
double RCRPowerPU = 0;    // Increment size (in Watts) for power Unit
double RCRPowerEU = 0;    // energy for each Energy Status Unit
double RCRPowerTime = 0;  // time between samples

int64_t numOfNodes = -1;
int64_t numOfSockets = -1;
int64_t numOfCores = -1;

double THRESHOLD=0.0;

uint64_t *energyWrap;
uint64_t *energySave;

uint64_t *memoryWrap;
uint64_t *memorySave;

static int isBlockTopology = 0; //Assuming non-block (cyclic etc.) topology for the system by default

typedef enum _RCRCounter {
    RCR_CNT_UNDEF = 0,
    RCR_CNT_SOCKET,
    RCR_CNT_CORE
} RCRCounter;

struct MSRCounter{
  uint64_t name;
  RCRCounter type; 
  uint32_t socket; 
  uint32_t core; 
  bool constant; 
  bool writable; 
  bool open;
  int fd;
  uint64_t value;
};

extern struct MSRCounter *msrArray;
// read counter for whole socket
uint64_t readMSRSocket(uint32_t socket, uint32_t name);
// read counter for core
uint64_t readMSRCore(uint32_t core, off_t name);
// setup the counter type
struct MSRCounter* RCRSetup(uint32_t numSockets, uint32_t numCores, uint32_t maxCounters);
// write the counter value of whole socket
void writeMSRSocket(uint32_t socket, uint32_t name, uint64_t value);
// write the counter value of whole core
void writeMSRCore(uint32_t core, uint32_t name, uint64_t value);



/* generic function to set up MSR Array */

struct MSRCounter *msrArray = NULL;

static void setupMsrArrayEntry (uint64_t name, uint64_t entry, uint64_t sock,uint64_t core)
{
  msrArray[entry].name = name;
  msrArray[entry].type = RCR_CNT_SOCKET; 
  msrArray[entry].socket = sock; 
  msrArray[entry].core = core;  //default
  msrArray[entry].constant = false;  //default
  msrArray[entry].writable = false;  //default
  msrArray[entry].open = false; //default
  msrArray[entry].fd = -1; //default
  msrArray[entry].value = 0; //default
}


uint64_t TOTAL_TOR[SOCKETSperNODE];
uint64_t TOTAL_RRO[SOCKETSperNODE];
uint64_t TOTAL_INST_RETIRED[SOCKETSperNODE];
uint64_t TOTAL_PWR_PKG_ENERGY[SOCKETSperNODE];
uint64_t TOTAL_L3_MISS[SOCKETSperNODE];
uint64_t TOTAL_L2_MISS[SOCKETSperNODE];

uint64_t LAST_TOR[SOCKETSperNODE];
uint64_t LAST_RRO[SOCKETSperNODE];
uint64_t LAST_INST_RETIRED[SOCKETSperNODE];
uint64_t LAST_PWR_PKG_ENERGY[SOCKETSperNODE];
uint64_t LAST_INST_RETIRED_CORE[SOCKETSperNODE][CORESperSOCKET];
uint64_t LAST_L3_MISS_CORE[SOCKETSperNODE][CORESperSOCKET];
uint64_t LAST_L2_MISS_CORE[SOCKETSperNODE][CORESperSOCKET];

uint64_t TOR_Core[SOCKETSperNODE][CORESperSOCKET];
uint64_t RRO_Core[SOCKETSperNODE][CORESperSOCKET];
uint64_t INST_RETIRED_CORE[SOCKETSperNODE][CORESperSOCKET]; ///////
uint64_t INST_RETIRED_Core[SOCKETSperNODE];
uint64_t PWR_PKG_ENERGY_Core[SOCKETSperNODE];
uint64_t L3_MISS_CORE[SOCKETSperNODE][CORESperSOCKET];
uint64_t L2_MISS_CORE[SOCKETSperNODE][CORESperSOCKET];

uint64_t POWER_UNIT = 0;
double JOULE_UNIT = 0.0;


/*********************************RCRMSRGenric***********************************/

static uint32_t numberOfMSRS = 0;

static uint64_t read_msr(int fd, int which) {

  uint64_t data;

  if (pread(fd, &data, sizeof(data), which) != sizeof(data)) {
    perror("rdmsr:pread");
    exit(127);
  }
  return data;

}

static int open_fd(struct MSRCounter *msr, bool write) {

  int fd = -1;
  char filename[BUFSIZ];

  if (msr->open == false) {
    sprintf(filename, "/dev/cpu/%d/msr_safe", msr->core);
    if (!write) fd = open(filename, O_RDONLY);
    else fd = open(filename, O_WRONLY);
    if (fd >= 0) {
      msr->fd = fd;
      msr->open = true;
    }
  }
  else fd = msr->fd;
  //  printf("\t\t\tfd %d open %d\n", msr->fd,msr->open);
  return fd;
}

uint64_t readMSRSocket(uint32_t socket, uint32_t name) {

  uint32_t i;
  for (i = 0; i < numberOfMSRS; i++) {
    // if (i <20) printf("entry %d name %d looking for %d\n", i,msrArray[i].name, name);
    if (msrArray[i].name != name) continue;  // not this counter
    if (msrArray[i].type != RCR_CNT_SOCKET) continue;  // not right type
    if (msrArray[i].socket != socket) continue;  // right name wrong socket
    if (msrArray[i].open == false) {
      msrArray[i].fd = open_fd(&msrArray[i], 0);        // not open so open
      if (msrArray[i].fd < 0) printf("MSR open failed %d %s\n", msrArray[i].fd, strerror(errno)); // open failed
    }
    if (msrArray[i].constant == false) {        // counter constant -- value already valid
      msrArray[i].value = read_msr(msrArray[i].fd, msrArray[i].name);
    }
    return msrArray[i].value;
  }
  return 0;
}

uint64_t readMSRCore(uint32_t core, off_t name) {
  char msr_file_name[64];
  ssize_t retval;
  uint64_t data;
  int fd_msr;

  sprintf(msr_file_name, "/dev/cpu/%d/msr_safe", core);
  fd_msr = open(msr_file_name, O_RDONLY);
  if (fd_msr < 0) {
    if (errno == ENXIO) {
      fprintf(stderr, "RCRTool: No CPU %d\n", core);
      return -1;
    } 
    else if (errno == EIO) {
      fprintf(stderr, "RCRTool: CPU %d doesn't support MSRs\n",
            core);
      return -1;
    } 
    else {
      perror("RCRTool: ");
      return -1;
    }
  }


  retval = pread(fd_msr, &data, sizeof data, name);
  if (retval != sizeof data) {
    fprintf(stderr, "pread cpu%d 0x%zu = %lu\n", core, name, retval);
    exit (-2);
  }
  close(fd_msr);
  return data;
}

/******************************write****************************/
static bool RCRSetupCalled = false;

struct MSRCounter* RCRSetup(uint32_t numSockets, uint32_t numCores, uint32_t maxCounters) {

  uint32_t i;
  if (RCRSetupCalled) return msrArray; // already setup
  if (maxCounters == 0) maxCounters = 8;
  RCRSetupCalled = true;
  numberOfMSRS = numSockets * numCores * maxCounters; // for starts allow 8 per core -- AKP Need to compute correct amount
  msrArray = (struct MSRCounter*) malloc(numberOfMSRS * sizeof(struct MSRCounter));

  for (i = 0; i < numberOfMSRS; i++) {
    msrArray[i].name = -1;
    msrArray[i].type = RCR_CNT_UNDEF;
    msrArray[i].socket = -1;
    msrArray[i].core = -1;
    msrArray[i].constant = false;
    msrArray[i].writable = false;
    msrArray[i].open = false;
    msrArray[i].fd = -1;
    msrArray[i].value = 0;
  }
  return msrArray;
}

/***********************************************************************************************/

int MSRWrite(int cpu, uint32_t reg, uint64_t data)
{
  int fd;
  char msr_file_name[64];

  sprintf(msr_file_name, "/dev/cpu/%d/msr_safe", cpu);
  fd = open(msr_file_name, O_WRONLY);
  if (fd < 0) {
    if (errno == ENXIO) {
      fprintf(stderr, "wrmsr: No CPU %d\n", cpu);
      exit(2);
    } else if (errno == EIO) {
      fprintf(stderr, "wrmsr: CPU %d doesn't support MSRs\n",
        cpu);
      exit(3);
    } else {
      perror("wrmsr@: open");
      exit(127);
    }
  }

    if (pwrite(fd, &data, sizeof data, reg) != sizeof data) {
        if (errno == EIO) {
            fprintf(stderr,
                "wrmsr: CPU %d cannot set MSR "
                "0x%08" PRIx32 " to 0x%016" PRIx64 "\n",
                cpu, reg, data);
            return(4);
        } else {
            perror("wrmsr: pwrite");
            return(127);
        }
    }

  close(fd);

  return(0);
}

uint64_t MSRRead(int cpu, uint32_t reg)
{
	uint64_t data;
	int fd;
	char msr_file_name[64];

	sprintf(msr_file_name, "/dev/cpu/%d/msr_safe", cpu);
	fd = open(msr_file_name, O_RDONLY);
	if (fd < 0) {
		if (errno == ENXIO) {
			fprintf(stderr, "rdmsr: No CPU %d\n", cpu);
			return(2);
		} else if (errno == EIO) {
			fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n",
				cpu);
			return(3);
		} else {
			perror("rdmsr@: open");
			return(127);
		}
	}

	if (pread(fd, &data, sizeof data, reg) != sizeof data) {
		if (errno == EIO) {
			fprintf(stderr, "rdmsr: CPU %d cannot read "
				"MSR 0x%08" PRIx32 "\n",
				cpu, reg);
			close(fd);
			return(4);
		} else {
			perror("rdmsr: pread");
			close(fd);
			return(127);
		}
	}

	close(fd);

	return(data);
}

/* Function returns the physical package id (socket number) given a cpu */
int get_physical_package_id (int cpu)
{
  char path[256];
  FILE *fileP;
  int physicalPackageId;

  sprintf (path, "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", cpu);

  fileP = fopen (path, "r");
  if (!fileP)
  {
    fprintf (stderr, "\n%s : open failed", path);
    return -1;
  }

  if (fscanf (fileP, "%d", &physicalPackageId) != 1)
  {
    fprintf (stderr, "\n%s: failed to parse from file", path);
    return -1;
  }

  fclose(fileP);
  return physicalPackageId;
}


void init_internal(){

  int sock;
  int correctedCoreNumber;
  //Store local copies of the socket and core counts -- check for previous intialization
  if (numOfNodes == -1) numOfNodes = NNODES;
  if (numOfSockets == -1) numOfSockets = SOCKETSperNODE;
  if (numOfCores == -1) numOfCores = CORESperSOCKET; 

  energyWrap = (uint64_t *) malloc (sizeof (uint64_t) * numOfSockets);
  energySave = (uint64_t *) malloc (sizeof (uint64_t) * numOfSockets);

  memoryWrap = (uint64_t *) malloc (sizeof (uint64_t) * numOfSockets);
  memorySave = (uint64_t *) malloc (sizeof (uint64_t) * numOfSockets);

  memset(TOR_Core , 0 , sizeof(TOR_Core));
  memset(RRO_Core , 0 , sizeof(RRO_Core));
  memset(PWR_PKG_ENERGY_Core , 0 , sizeof(PWR_PKG_ENERGY_Core));
  memset(INST_RETIRED_Core , 0 , sizeof(INST_RETIRED_Core));
  memset(INST_RETIRED_CORE,  0 , sizeof(INST_RETIRED_CORE));//////////
  memset(L3_MISS_CORE, 0 , sizeof(L3_MISS_CORE));
  memset(L2_MISS_CORE, 0 , sizeof(L2_MISS_CORE));

  memset(LAST_TOR , 0 , sizeof(LAST_TOR));
  memset(LAST_RRO , 0 , sizeof(LAST_RRO));
  memset(LAST_INST_RETIRED , 0 , sizeof(LAST_INST_RETIRED));
  memset(LAST_PWR_PKG_ENERGY , 0 , sizeof(LAST_PWR_PKG_ENERGY));
  memset(LAST_INST_RETIRED_CORE , 0 , sizeof(LAST_INST_RETIRED_CORE));/////////////
  memset(LAST_L3_MISS_CORE , 0 , sizeof(LAST_L3_MISS_CORE));
  memset(LAST_L2_MISS_CORE , 0 , sizeof(LAST_L2_MISS_CORE));

  memset(TOTAL_TOR , 0 , sizeof(TOTAL_TOR));
  memset(TOTAL_RRO , 0 , sizeof(TOTAL_RRO));
  memset(TOTAL_INST_RETIRED , 0 , sizeof(TOTAL_INST_RETIRED));
  memset(TOTAL_PWR_PKG_ENERGY , 0 , sizeof(TOTAL_PWR_PKG_ENERGY));
  memset(TOTAL_L3_MISS , 0 , sizeof(TOTAL_L3_MISS));
  memset(TOTAL_L2_MISS , 0 , sizeof(TOTAL_L2_MISS));

  for (sock = 0; sock < numOfSockets; sock++)
  {
    energyWrap[sock] = 0;
    energySave[sock] = 0;

    memoryWrap[sock] = 0;
    memorySave[sock] = 0;

  }

  msrArray = RCRSetup (numOfSockets, numOfCores, maxCounters);

  // set up generic MSR array
  // setup POWER_UNIT to read to compute unit values 
  setupMsrArrayEntry (MSR_RAPL_POWER_UNIT, counterNum++, 0, 0);  //counter 1

  // set up counters to read dynamic power during execution
  for (sock = 0; sock < numOfSockets; sock++)
  { 
    //Discover the topology of the system using the physical id and assign correct cores to sockets 
    if (sock == get_physical_package_id(sock))
    {
      correctedCoreNumber = sock;
    }
    else
    {
      correctedCoreNumber = sock * numOfCores;
      isBlockTopology = 1;
    }

    // setup Power MSRs
    setupMsrArrayEntry (MSR_PKG_ENERGY_STATUS, counterNum++, sock, correctedCoreNumber);
  }

  // set up counters to read dynamic power during execution
  for (sock = 0; sock < numOfSockets; sock++)
  {
    // assign counters needed to view C0-Box Counters
    // init array sttructure for all of the counters to be used 
    int c = 0;

    if (c < numOfCores)
    {
      // Cbox 0
      setupMsrArrayEntry (C0_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C0_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C0_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C0_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C0_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C0_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C0_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C0_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C0_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C0_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C0_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }
    if (c < numOfCores)
    { 
      // Cbox 1
      setupMsrArrayEntry (C1_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C1_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C1_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C1_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C1_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C1_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C1_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C1_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C1_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C1_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C1_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 2
      setupMsrArrayEntry (C2_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C2_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C2_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C2_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C2_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C2_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C2_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C2_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C2_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C2_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C2_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 3
      setupMsrArrayEntry (C3_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C3_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C3_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C3_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C3_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C3_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C3_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C3_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C3_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C3_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C3_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 4
      setupMsrArrayEntry (C4_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C4_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C4_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C4_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C4_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C4_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C4_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C4_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C4_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C4_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C4_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 5
      setupMsrArrayEntry (C5_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C5_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C5_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C5_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C5_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C5_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C5_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C5_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C5_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C5_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C5_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 6
      setupMsrArrayEntry (C6_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C6_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C6_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C6_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C6_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C6_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C6_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C6_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C6_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C6_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C6_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 7
      setupMsrArrayEntry (C7_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C7_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C7_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C7_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C7_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C7_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C7_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C7_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C7_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C7_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C7_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 8
      setupMsrArrayEntry (C8_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C8_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C8_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C8_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C8_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C8_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C8_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C8_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C8_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C8_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C8_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 9
      setupMsrArrayEntry (C9_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C9_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C9_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C9_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C9_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C9_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C9_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C9_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C9_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C9_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C9_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }   
    if (c < numOfCores)
    {
      // Cbox 10
      setupMsrArrayEntry (C10_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C10_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C10_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C10_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C10_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C10_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C10_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C10_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C10_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C10_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C10_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }   
    if (c < numOfCores)
    {
      // Cbox 11
      setupMsrArrayEntry (C11_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C11_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C11_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C11_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C11_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C11_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C11_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C11_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C11_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C11_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C11_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }   
    if (c < numOfCores)
    {
      // Cbox 12
      setupMsrArrayEntry (C12_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C12_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C12_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C12_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C12_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C12_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C12_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C12_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C12_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C12_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C12_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }   
    if (c < numOfCores)
    {
      // Cbox 13
      setupMsrArrayEntry (C13_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C13_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C13_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C13_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C13_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C13_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C13_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C13_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C13_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C13_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C13_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }   
    if (c < numOfCores)
    {
      // Cbox 14
      setupMsrArrayEntry (C14_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C14_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C14_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C14_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C14_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C14_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C14_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C14_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C14_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C14_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C14_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }   
    if (c < numOfCores)
    {
      // Cbox 15
      setupMsrArrayEntry (C15_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C15_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C15_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C15_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C15_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C15_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C15_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C15_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C15_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C15_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C15_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }   
    if (c < numOfCores)
    {
      // Cbox 16
      setupMsrArrayEntry (C16_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C16_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C16_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C16_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C16_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C16_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C16_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C16_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C16_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C16_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C16_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }   
    if (c < numOfCores)
    {
      // Cbox 17
      setupMsrArrayEntry (C17_MSR_PMON_BOX_CTL, counterNum++, sock, sock);
      setupMsrArrayEntry (C17_MSR_PMON_CTL0, counterNum++, sock, sock);
      setupMsrArrayEntry (C17_MSR_PMON_CTL1, counterNum++, sock, sock);
      setupMsrArrayEntry (C17_MSR_PMON_CTL2, counterNum++, sock, sock);
      setupMsrArrayEntry (C17_MSR_PMON_CTL3, counterNum++, sock, sock);
      setupMsrArrayEntry (C17_MSR_PMON_CTR0, counterNum++, sock, sock);
      setupMsrArrayEntry (C17_MSR_PMON_CTR1, counterNum++, sock, sock);
      setupMsrArrayEntry (C17_MSR_PMON_CTR2, counterNum++, sock, sock);
      setupMsrArrayEntry (C17_MSR_PMON_CTR3, counterNum++, sock, sock);
      setupMsrArrayEntry (C17_MSR_PMON_BOX_FILTER0, counterNum++, sock, sock);
      setupMsrArrayEntry (C17_MSR_PMON_BOX_FILTER1, counterNum++, sock, sock);
      c++;
    }   
  }
  
  for (int core = 0; core < numOfCores * numOfSockets; core++)
  {
    MSRWrite (core, IA32_PERF_GLOBAL_CTRL, IA32_PERF_GLOBAL_CTRL_VALUE);
    MSRWrite (core, IA32_PERFEVTSEL0, IA32_PERFEVTSEL0_VALUE); // L3 miss
    MSRWrite (core, IA32_PERFEVTSEL1, IA32_PERFEVTSEL1_VALUE); // L2 miss
    MSRWrite (core, IA32_FIXED_CTR_CTRL, IA32_FIXED_CTR_CTRL_VALUE);
  }
   // Setting the PERF CTRL MSRs to count instructions retired on all cores in user and operating system space

 // setup memory concurrency counters
  for (int sock = 0; sock < numOfSockets; sock++)
  {
    int c = 0;

    if (c < numOfCores)
    {
      // Cbox 0
      MSRWrite (sock, C0_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); // Freeze C-box
      // Enable C counting (1 in bit 22), pick event (36 in 0-7 = TOR_OCCUPANCY), one in bit 11 to count ALL event in TOR (TOR_OCCUPANCY ALL = 0x400836); RegisterRestriction : 0
      MSRWrite (sock, C0_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE);
      // Enable C counting (1 in bit 22), pick event (11 in 0-7 = RxR_OCCUPANCY), b00000001 in bits 15-8 to count all Ingress Request (IRQ) on AD ring (from cores) (RxR_OCCUPANCY IRQ = 0x400111); RegisterRestriction : 0, but works with 1 
      // change RR occupancy counter to TOR_INSERT
      MSRWrite (sock, C0_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      // Enable C counting (1 in bit 22), pick event (37 in 0-7 = LLC_VICTIMS), b00000000 in bits 15-8 to count all events tracked (LLC_VICTIMS ALL = 0x400037); RegisterRestriction : 0-3 
      MSRWrite (sock, C0_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      // Enable C counting (1 in bit 22), pick event (00 in 0-7 = UNCORE CLOCKTICKS); RegisterRestriction : 0-3 
      MSRWrite (sock, C0_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      
      MSRWrite (sock, C0_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C0_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);  
      
      MSRWrite (sock, C0_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); // Unfreeze C-box
      

      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 1
      MSRWrite (sock, C1_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C1_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE); 
      MSRWrite (sock, C1_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE); 
      MSRWrite (sock, C1_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C1_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C1_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C1_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);   
      MSRWrite (sock, C1_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    { 
      // Cbox 2
      MSRWrite (sock, C2_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C2_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE);
      MSRWrite (sock, C2_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      MSRWrite (sock, C2_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C2_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C2_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C2_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);  
      MSRWrite (sock, C2_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 3
      MSRWrite (sock, C3_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C3_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE); 
      MSRWrite (sock, C3_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE); 
      MSRWrite (sock, C3_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C3_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C3_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C3_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);   
      MSRWrite (sock, C3_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 4
      MSRWrite (sock, C4_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C4_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE);
      MSRWrite (sock, C4_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      MSRWrite (sock, C4_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C4_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C4_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C4_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);  
      MSRWrite (sock, C4_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    { 
      // Cbox 5
      MSRWrite (sock, C5_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C5_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE); 
      MSRWrite (sock, C5_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      MSRWrite (sock, C5_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C5_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C5_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C5_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);   
      MSRWrite (sock, C5_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    { 
      // Cbox 6
      MSRWrite (sock, C6_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C6_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE);
      MSRWrite (sock, C6_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      MSRWrite (sock, C6_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C6_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C6_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C6_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);  
      MSRWrite (sock, C6_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 7
      MSRWrite (sock, C7_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C7_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE);
      MSRWrite (sock, C7_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      MSRWrite (sock, C7_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C7_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C7_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C7_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);  
      MSRWrite (sock, C7_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    { 
      // Cbox 8
      MSRWrite (sock, C8_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C8_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE);
      MSRWrite (sock, C8_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      MSRWrite (sock, C8_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C8_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C8_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C8_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);  
      MSRWrite (sock, C8_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 9
      MSRWrite (sock, C9_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C9_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE);
      MSRWrite (sock, C9_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      MSRWrite (sock, C9_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C9_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C9_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C9_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);  
      MSRWrite (sock, C9_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 10
      MSRWrite (sock, C10_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C10_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE);
      MSRWrite (sock, C10_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      MSRWrite (sock, C10_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C10_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C10_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C10_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);  
      MSRWrite (sock, C10_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 11
      MSRWrite (sock, C11_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C11_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE);
      MSRWrite (sock, C11_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      MSRWrite (sock, C11_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C11_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C11_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C11_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);  
      MSRWrite (sock, C11_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 12
      MSRWrite (sock, C12_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C12_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE);
      MSRWrite (sock, C12_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      MSRWrite (sock, C12_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C12_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C12_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C12_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);  
      MSRWrite (sock, C12_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 13
      MSRWrite (sock, C13_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C13_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE);
      MSRWrite (sock, C13_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      MSRWrite (sock, C13_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C13_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C13_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C13_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);  
      MSRWrite (sock, C13_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 14
      MSRWrite (sock, C14_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C14_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE);
      MSRWrite (sock, C14_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      MSRWrite (sock, C14_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C14_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C14_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C14_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);
      MSRWrite (sock, C14_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 15
      MSRWrite (sock, C15_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C15_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE);
      MSRWrite (sock, C15_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      MSRWrite (sock, C15_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C15_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C15_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C15_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);  
      MSRWrite (sock, C15_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 16
      MSRWrite (sock, C16_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C16_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE);
      MSRWrite (sock, C16_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      MSRWrite (sock, C16_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C16_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C16_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C16_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);  
      MSRWrite (sock, C16_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
    if (c < numOfCores)
    {
      // Cbox 17
      MSRWrite (sock, C17_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_FREEZE_VALUE); 
      MSRWrite (sock, C17_MSR_PMON_CTL0, MSR_PMON_CTL0_VALUE);
      MSRWrite (sock, C17_MSR_PMON_CTL1, MSR_PMON_CTL1_VALUE);
      MSRWrite (sock, C17_MSR_PMON_CTL2, MSR_PMON_CTL2_VALUE);
      MSRWrite (sock, C17_MSR_PMON_CTL3, MSR_PMON_CTL3_VALUE);
      MSRWrite (sock, C17_MSR_PMON_BOX_FILTER0, MSR_PMON_BOX_FILTER0_VALUE); 
      MSRWrite (sock, C17_MSR_PMON_BOX_FILTER1, MSR_PMON_BOX_FILTER1_VALUE);  
      MSRWrite (sock, C17_MSR_PMON_BOX_CTL, MSR_PMON_BOX_CTL_UNFREEZE_VALUE); 
      c++;
    }
  }

  //compute power unit
  POWER_UNIT = readMSRSocket (0, MSR_RAPL_POWER_UNIT); // calculate once
  JOULE_UNIT = 1.0 / (1 << ((POWER_UNIT >> 8) & 0x1F)); // convert power unit into joule unit 
  
}


int get_TIPI_Slab(double tipi){
	//printf("TIPI_INTERVAL: %d\n" , (int)TIPI_INTERVAL);
	return (int)(tipi/TIPI_INTERVAL);
}
 

void cf_read(){
	int sock;
	int correctSocketIndex;

	uint64_t inst[numOfSockets];
	//Initialize the local socket counters to 0
	for (int i = 0; i < numOfSockets; i++)
	{
		inst[i] = 0;
	}
	int totalCores = numOfCores * numOfSockets; // Total cores in the system

	for (int core = 0; core < totalCores; core++)
	{
		//If not block topology, values from consecutive cores are assigned to alternate sockets in a cyclic manner
		if (isBlockTopology)
		{
			correctSocketIndex = core / numOfCores;
			uint64_t instruction = readMSRCore (core, IA32_FIXED_CTR0);
			uint64_t l3Miss_core = readMSRCore (core, IA32_PMC0);
			uint64_t l2Miss_core = readMSRCore (core, IA32_PMC1);

			if(flag!=0){
				LAST_INST_RETIRED_CORE[correctSocketIndex][core%numOfCores] = instruction - INST_RETIRED_CORE[correctSocketIndex][core%numOfCores];
				LAST_L3_MISS_CORE[correctSocketIndex][core%numOfCores] = l3Miss_core - L3_MISS_CORE[correctSocketIndex][core%numOfCores];
				LAST_L2_MISS_CORE[correctSocketIndex][core%numOfCores] = l2Miss_core - L2_MISS_CORE[correctSocketIndex][core%numOfCores];
				TOTAL_L3_MISS[correctSocketIndex] += LAST_L3_MISS_CORE[correctSocketIndex][core%numOfCores];
				TOTAL_L2_MISS[correctSocketIndex] += LAST_L2_MISS_CORE[correctSocketIndex][core%numOfCores];
			}
			INST_RETIRED_CORE[correctSocketIndex][core%numOfCores] = instruction;
			L3_MISS_CORE[correctSocketIndex][core%numOfCores] = l3Miss_core;
			L2_MISS_CORE[correctSocketIndex][core%numOfCores] = l2Miss_core;
			inst[correctSocketIndex] += instruction;

		}
		else
		{
		    correctSocketIndex = core % numOfSockets; /************************check this part from topology****************/
			uint64_t instruction = readMSRCore (core, IA32_FIXED_CTR0);
			uint64_t l3Miss_core = readMSRCore (core, IA32_PMC0);
			uint64_t l2Miss_core = readMSRCore (core, IA32_PMC1);
			if(flag!=0){
				LAST_INST_RETIRED_CORE[correctSocketIndex][core/numOfSockets] = instruction - INST_RETIRED_CORE[correctSocketIndex][core/numOfSockets];
				LAST_L3_MISS_CORE[correctSocketIndex][core/numOfSockets] = l3Miss_core - L3_MISS_CORE[correctSocketIndex][core/numOfSockets];
				LAST_L2_MISS_CORE[correctSocketIndex][core/numOfSockets] = l2Miss_core - L2_MISS_CORE[correctSocketIndex][core/numOfSockets];
				TOTAL_L3_MISS[correctSocketIndex] += LAST_L3_MISS_CORE[correctSocketIndex][core/numOfSockets];
				TOTAL_L2_MISS[correctSocketIndex] += LAST_L2_MISS_CORE[correctSocketIndex][core/numOfSockets];
			}
			INST_RETIRED_CORE[correctSocketIndex][core/numOfSockets] = instruction;
			L3_MISS_CORE[correctSocketIndex][core/numOfSockets] = l3Miss_core;
			L2_MISS_CORE[correctSocketIndex][core/numOfSockets] = l2Miss_core;

		    	inst[correctSocketIndex] += instruction;
		}


	}

	for (sock = 0; sock < numOfSockets; sock++)
	{
		if(flag!=0){
		    LAST_INST_RETIRED[sock] = inst[sock] - INST_RETIRED_Core[sock];
		    TOTAL_INST_RETIRED[sock] += LAST_INST_RETIRED[sock];
		}
		INST_RETIRED_Core[sock] = inst[sock];

		uint64_t energyStatus = readMSRSocket (sock, MSR_PKG_ENERGY_STATUS); // get energy MSR

		uint64_t energyCounter = energyStatus & 0xffffffff; // only 32 of 64 bits good
		if (energyCounter < energySave[sock])
		{
		  // did I just wrap the counter?
		  energyWrap[sock]++;
		}
		energySave[sock] = energyCounter;
		energyCounter = energyCounter + (energyWrap[sock]<<32);// number of wraps in upper 32 bits

		if(flag!=0){
		    LAST_PWR_PKG_ENERGY[sock] = energyCounter - PWR_PKG_ENERGY_Core[sock];
		    TOTAL_PWR_PKG_ENERGY[sock] += LAST_PWR_PKG_ENERGY[sock];
		}
		PWR_PKG_ENERGY_Core[sock] = energyCounter;

	}
	// for uncore counters
	for (sock = 0; sock < numOfSockets; sock++){
	uint64_t cnt0[numOfCores];
	uint64_t cnt1[numOfCores];
	uint64_t cnt2[numOfCores];
	uint64_t cnt3[numOfCores];
	double memTorOccupancy = 0.0;
	double memRROccupancy = 0.0;

	int c = 0;
	// read all Cboxes
	if (c < numOfCores)
	{
	  cnt0[0] = readMSRSocket (sock, C0_MSR_PMON_CTR0); // TOR INSERT : MISS_REMOTE
	  cnt1[0] = readMSRSocket (sock, C0_MSR_PMON_CTR1); // TOR INSERT : MISS LOCAL
	  cnt2[0] = readMSRSocket (sock, C0_MSR_PMON_CTR2); // LLC_VICTIMS ALL /* not used ******/
	  cnt3[0] = readMSRSocket (sock, C0_MSR_PMON_CTR3); // UNCORE CLOCK_TICKS /******not used*********/
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[1] = readMSRSocket (sock, C1_MSR_PMON_CTR0); 
	  cnt1[1] = readMSRSocket (sock, C1_MSR_PMON_CTR1); 
	  cnt2[1] = readMSRSocket (sock, C1_MSR_PMON_CTR2); 
	  cnt3[1] = readMSRSocket (sock, C1_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[2] = readMSRSocket (sock, C2_MSR_PMON_CTR0); 
	  cnt1[2] = readMSRSocket (sock, C2_MSR_PMON_CTR1); 
	  cnt2[2] = readMSRSocket (sock, C2_MSR_PMON_CTR2); 
	  cnt3[2] = readMSRSocket (sock, C2_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[3] = readMSRSocket (sock, C3_MSR_PMON_CTR0); 
	  cnt1[3] = readMSRSocket (sock, C3_MSR_PMON_CTR1); 
	  cnt2[3] = readMSRSocket (sock, C3_MSR_PMON_CTR2); 
	  cnt3[3] = readMSRSocket (sock, C3_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[4] = readMSRSocket (sock, C4_MSR_PMON_CTR0); 
	  cnt1[4] = readMSRSocket (sock, C4_MSR_PMON_CTR1); 
	  cnt2[4] = readMSRSocket (sock, C4_MSR_PMON_CTR2); 
	  cnt3[4] = readMSRSocket (sock, C4_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[5] = readMSRSocket (sock, C5_MSR_PMON_CTR0); 
	  cnt1[5] = readMSRSocket (sock, C5_MSR_PMON_CTR1); 
	  cnt2[5] = readMSRSocket (sock, C5_MSR_PMON_CTR2); 
	  cnt3[5] = readMSRSocket (sock, C5_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[6] = readMSRSocket (sock, C6_MSR_PMON_CTR0); 
	  cnt1[6] = readMSRSocket (sock, C6_MSR_PMON_CTR1); 
	  cnt2[6] = readMSRSocket (sock, C6_MSR_PMON_CTR2); 
	  cnt3[6] = readMSRSocket (sock, C6_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[7] = readMSRSocket (sock, C7_MSR_PMON_CTR0); 
	  cnt1[7] = readMSRSocket (sock, C7_MSR_PMON_CTR1); 
	  cnt2[7] = readMSRSocket (sock, C7_MSR_PMON_CTR2); 
	  cnt3[7] = readMSRSocket (sock, C7_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[8] = readMSRSocket (sock, C8_MSR_PMON_CTR0); 
	  cnt1[8] = readMSRSocket (sock, C8_MSR_PMON_CTR1); 
	  cnt2[8] = readMSRSocket (sock, C8_MSR_PMON_CTR2); 
	  cnt3[8] = readMSRSocket (sock, C8_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[9] = readMSRSocket (sock, C9_MSR_PMON_CTR0); 
	  cnt1[9] = readMSRSocket (sock, C9_MSR_PMON_CTR1); 
	  cnt2[9] = readMSRSocket (sock, C9_MSR_PMON_CTR2); 
	  cnt3[9] = readMSRSocket (sock, C9_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[10] = readMSRSocket (sock, C10_MSR_PMON_CTR0); 
	  cnt1[10] = readMSRSocket (sock, C10_MSR_PMON_CTR1); 
	  cnt2[10] = readMSRSocket (sock, C10_MSR_PMON_CTR2); 
	  cnt3[10] = readMSRSocket (sock, C10_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[11] = readMSRSocket (sock, C11_MSR_PMON_CTR0); 
	  cnt1[11] = readMSRSocket (sock, C11_MSR_PMON_CTR1); 
	  cnt2[11] = readMSRSocket (sock, C11_MSR_PMON_CTR2); 
	  cnt3[11] = readMSRSocket (sock, C11_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[12] = readMSRSocket (sock, C12_MSR_PMON_CTR0); 
	  cnt1[12] = readMSRSocket (sock, C12_MSR_PMON_CTR1); 
	  cnt2[12] = readMSRSocket (sock, C12_MSR_PMON_CTR2); 
	  cnt3[12] = readMSRSocket (sock, C12_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[13] = readMSRSocket (sock, C13_MSR_PMON_CTR0); 
	  cnt1[13] = readMSRSocket (sock, C13_MSR_PMON_CTR1); 
	  cnt2[13] = readMSRSocket (sock, C13_MSR_PMON_CTR2); 
	  cnt3[13] = readMSRSocket (sock, C13_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[14] = readMSRSocket (sock, C14_MSR_PMON_CTR0); 
	  cnt1[14] = readMSRSocket (sock, C14_MSR_PMON_CTR1); 
	  cnt2[14] = readMSRSocket (sock, C14_MSR_PMON_CTR2); 
	  cnt3[14] = readMSRSocket (sock, C14_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[15] = readMSRSocket (sock, C15_MSR_PMON_CTR0); 
	  cnt1[15] = readMSRSocket (sock, C15_MSR_PMON_CTR1); 
	  cnt2[15] = readMSRSocket (sock, C15_MSR_PMON_CTR2); 
	  cnt3[15] = readMSRSocket (sock, C15_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[16] = readMSRSocket (sock, C16_MSR_PMON_CTR0); 
	  cnt1[16] = readMSRSocket (sock, C16_MSR_PMON_CTR1); 
	  cnt2[16] = readMSRSocket (sock, C16_MSR_PMON_CTR2); 
	  cnt3[16] = readMSRSocket (sock, C16_MSR_PMON_CTR3); 
	  c++;
	}
	if (c < numOfCores)
	{
	  cnt0[17] = readMSRSocket (sock, C17_MSR_PMON_CTR0); 
	  cnt1[17] = readMSRSocket (sock, C17_MSR_PMON_CTR1); 
	  cnt2[17] = readMSRSocket (sock, C17_MSR_PMON_CTR2); 
	  cnt3[17] = readMSRSocket (sock, C17_MSR_PMON_CTR3); 
	  c++;
	}

	for (int cbox = 0; cbox < numOfCores; cbox++){
	  // socket 0 and 1 seem to agree -- only use 1 or bug?
	  uint64_t oldTorOccupancyValue, oldRROccupancyValue;
	  uint64_t torOccupancyValue, rrOccupancyValue;
	  double diff0, diff1;

	  // accumulate each Cbox
	  torOccupancyValue = cnt0[cbox] & 0xFFFFFFFFFFFFL;   //Chopping the higher order bits 62-48
	  rrOccupancyValue = cnt1[cbox] & 0xFFFFFFFFFFFFL; 

	  oldTorOccupancyValue = TOR_Core[sock][cbox];
	  oldRROccupancyValue = RRO_Core[sock][cbox];



	  //Check if the counters have wrapped
	  if (torOccupancyValue < oldTorOccupancyValue)
	  {
	    memoryWrap[sock]++;
	    torOccupancyValue = torOccupancyValue + (memoryWrap[sock]<<44);
	  }

	  diff0 = torOccupancyValue - oldTorOccupancyValue;   // TOR occupancy all since last update
	  diff1 = rrOccupancyValue - oldRROccupancyValue;     // RR Occupancy due to IRQ since last update

	  memTorOccupancy += (diff0>0) ? diff0 : 0.0;   // this cbox average Memorry access to LLC = TOR occupancy all/ticks
	  memRROccupancy  += (diff1>0) ? diff1 : 0.0;    // this cbox average currency = TOR occupancy/ticks

	  // write interdiatate values into blackboard
	  TOR_Core[sock][cbox] = torOccupancyValue;
	  RRO_Core[sock][cbox] = rrOccupancyValue;
	}
	if(flag!=0){
	    // write summed concurrency into blackboard
	    LAST_TOR[sock] = memTorOccupancy;
	    LAST_RRO[sock] = memRROccupancy;

	    TOTAL_TOR[sock] += memTorOccupancy;
	    TOTAL_RRO[sock] += memRROccupancy;
	}

  }
  if(flag==0){
  	flag=1;
  }
  else{
        double tor_insert=0;
        double inst_retired=0;
        double pkg_energy = 0;
        for (sock = 0; sock < numOfSockets; sock++){
                tor_insert += (double)(LAST_RRO[sock]+LAST_TOR[sock]);
                inst_retired += (double)(LAST_INST_RETIRED[sock]);
                pkg_energy += (double)(LAST_PWR_PKG_ENERGY[sock]);
        }
        pkg_energy = pkg_energy * JOULE_UNIT; // covert energy value in joule unit
        double tipi = tor_insert/inst_retired; // total tor insert per instruction
        double jpi = pkg_energy/inst_retired; // total energy per instruction



// Run cuttlefish policy
#if defined(CORE) || defined(UNCORE) || defined(COMBINED)
	lb_policy(tipi , jpi);
#else
// Run motivation policy
	int CURRENT_TIPI_SLAB=get_TIPI_Slab(tipi);
	// check previous tipi slab is same as current tipi slab
        if(CURRENT_TIPI_SLAB==PREV_TIPI_SLAB){
		add_log(tipi ,(int)check_uncoreFrequency(0) , (int)check_uncoreFrequency(1) , jpi , 0 , inst_retired , pkg_energy, 0 , tor_insert , 0 , 0);
  	}
	PREV_TIPI_SLAB=CURRENT_TIPI_SLAB;
#endif
	cf_read_counter +=1;

	if(cf_read_counter == TOTAL_SLABS_TO_DISCARD_ATSTART){
		if(getenv("CUTTLEFISH_MOTIVATION_CORE") != NULL){
			int cfs = atoi(getenv("CUTTLEFISH_MOTIVATION_CORE"));
			for(int core = 0; core < totalCores; core++)
				set_coreFrequency(core , cfs);
		}
		if(getenv("CUTTLEFISH_MOTIVATION_UNCORE") != NULL){
			int ufs = atoi(getenv("CUTTLEFISH_MOTIVATION_UNCORE"));
			for(sock = 0; sock <numOfSockets; sock++)
				set_uncoreFrequency(sock , ufs);
		}
	}

  }

}

void show_stats(){
  int i;
fprintf(stdout,"\n============================ Tabulate Statistics ============================\n"); 
    // print all events
    fprintf(stdout,"%s\t","PWR_PKG_ENERGY");
    fprintf(stdout,"%s\t","INST_RETIRED");
    fprintf(stdout,"%s\t","TOR_OCCUPANCY");
    fprintf(stdout,"%s\t","RR_OCCUPANCY");
    fprintf(stdout,"%s\t","TIPI");
    fprintf(stdout,"%s\t","L3_MISS");
    fprintf(stdout,"%s\t","L2_MISS");
    fprintf(stdout,"%s\t","TIME");
    tipi_stats();
    fprintf(stdout,"\n");
    uint64_t res=0;
    for(i=0; i<numOfSockets; i++) {
      res += TOTAL_PWR_PKG_ENERGY[i];
    }
    fprintf(stdout,"%f\t",((double)res)*JOULE_UNIT);
    res=0;
    for(i=0; i<numOfSockets; i++) {
      res += TOTAL_INST_RETIRED[i];
    }
    fprintf(stdout,"%zu\t",res);
    res=0;
    for(i=0; i<numOfSockets; i++) {
      res += TOTAL_TOR[i];
    }
    fprintf(stdout,"%zu\t",res);
    res=0;
    for(i=0; i<numOfSockets; i++) {
      res += TOTAL_RRO[i];
    }
    fprintf(stdout,"%zu\t",res);
    double tor_insert=0;
    double inst_retired=0;
    for(i=0; i<numOfSockets; i++) {
      tor_insert += (TOTAL_RRO[i] + TOTAL_TOR[i]);
      inst_retired += TOTAL_INST_RETIRED[i];
    }
    fprintf(stdout,"%f\t",tor_insert/inst_retired);
    res=0;
    for(i=0; i<numOfSockets; i++) {
      res += TOTAL_L3_MISS[i];
    }
    fprintf(stdout,"%zu\t",res);
    res=0;
    for(i=0; i<numOfSockets; i++) {
      res += TOTAL_L2_MISS[i];
    }
    fprintf(stdout,"%zu\t",res);
    fprintf(stdout,"%f\t",get_exec_time()); // return total time
    tipi_value();
    fprintf(stdout,"\n=============================================================================\n");
    fflush(stdout);
}

void finalize_internal(){
  free(energyWrap);
  free(energySave);
  free(memoryWrap);
  free(memorySave);
  free(msrArray);
}

}

