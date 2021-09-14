# define MSR_PMON_CTL0_VALUE 0x408a35 //TOR_INSERT:MISS_REMOTE// Enable C counting (1 in bit 22), pick event (36 in 0-7 = TOR_OCCUPANCY), one in bit 11 to count ALL event in TOR (TOR_OCCUPANCY ALL = 0x400836); RegisterRestricti$
# define MSR_PMON_CTL1_VALUE 0x402a35 //TOR_INSERT:MISS_LOCAL Enable C counting (1 in bit 22), pick event (36 in 0-7 = TOR_INSERT), 2A in bit {8-15} to count LOCAL MISS ALL event in TOR (TOR_INSERT:MISSALL = 0x402A35); Regis$
# define MSR_PMON_CTL2_VALUE 0x400037 // NOT USING in CUTTLEFISH       // Enable C counting (1 in bit 22), pick event (37 in 0-7 = LLC_VICTIMS), b00000000 in bits 15-8 to count all events tracked (LLC_VI$
# define MSR_PMON_CTL3_VALUE 0x400000 // NOT USING in CUTTLEFISH       // Enable C counting (1 in bit 22), pick event (00 in 0-7 = UNCORE CLOCKTICKS); RegisterRestriction : 0-3 

#define  MSR_PMON_BOX_FILTER0_VALUE 0x0  // default value of filter 0
#define  MSR_PMON_BOX_FILTER1_VALUE 0x0  // default value of filter 1

#define MSR_PMON_BOX_CTL_FREEZE_VALUE 0x0000000000010100 //freeze C-BOX
#define MSR_PMON_BOX_CTL_UNFREEZE_VALUE 0x0000000000010000 //unfreeze C-BOX

/************************************************************************/
// Machine configuration
#define CORESperSOCKET 10
#define SOCKETSperNODE 2
#define NNODES 1
#define AVAIL_FREQ_CORE 12
#define MAX_FREQ_CORE 23
#define MIN_FREQ_CORE 12
#define MID_FREQ_CORE 17
#define AVAIL_FREQ_UNCORE 19
#define MIN_FREQ_UNCORE 12
#define MAX_FREQ_UNCORE 30
#define MID_FREQ_UNCORE 21
#define NUM_WORKERS 20
#if defined(MOTIVATION)
	#define TIPI_INTERVAL 1
#else
	#define TIPI_INTERVAL 0.004
#endif
#define TOTAL_SLABS_TO_DISCARD_ATSTART 100
/************************************************************************/
