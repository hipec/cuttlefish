#CUTTLEFISH_BASE_FLAGS := -I$(CUTTLEFISH_ROOT)/include 
#CUTTLEFISH_CFLAGS     := $(CUTTLEFISH_BASE_FLAGS)
#CUTTLEFISH_CXXFLAGS   := $(CUTTLEFISH_BASE_FLAGS)
#CUTTLEFISH_LDFLAGS    := -L$(CUTTLEFISH_ROOT)/lib
#CUTTLEFISH_LDLIBS     := -lcuttlefish -Wl,-rpath,$(CUTTLEFISH_ROOT)/lib 

CUTTLEFISH_CXXFLAGS=-O3 -I$(CUTTLEFISH_ROOT)/include
CUTTLEFISH_LDFLAGS=-L$(CUTTLEFISH_ROOT)/lib
CUTTLEFISH_LDLIBS=-lcuttlefish