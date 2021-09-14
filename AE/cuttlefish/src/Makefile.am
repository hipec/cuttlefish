SUBDIRS =

# Includes folders
INCLUDES_DIR = -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc

# cflags: important to define that otherwise we inherit default values too
CXXFLAGS = -Wall -O3
LDFLAGS = -lpthread

if MOTIVATION
MOTIVATION_FLAGS = -DMOTIVATION
else
MOTIVATION_FLAGS =
endif

if CORE
CORE_FLAGS = -DCORE
else
CORE_FLAGS =
endif

if UNCORE
UNCORE_FLAGS = -DUNCORE
else
UNCORE_FLAGS =
endif

if COMBINED
COMBINED_FLAGS = -DCOMBINED
else
COMBINED_FLAGS =
endif

# build the cuttlefish library
lib_LTLIBRARIES = libcuttlefish.la

# These are populated in sub-folder's Makefile.inc
noinst_LTLIBRARIES =
libcuttlefish_la_LIBADD =

AM_CXXFLAGS = $(CORE_FLAGS) $(UNCORE_FLAGS) $(COMBINED_FLAGS) $(MOTIVATION_FLAGS)
libcuttlefish_la_SOURCES = cuttlefish-daemon.cpp cuttlefish-perfcounter.cpp cuttlefish-lbPolicy.cpp cuttlefish-tipiList.cpp cuttlefish-setFrequency.cpp

# build cuttlefish
libcuttlefish_la_CXXFLAGS= $(CXXFLAGS) $(INCLUDES_DIR) $(AM_CXXFLAGS)

MAINTAINERCLEANFILES = Makefile.in
