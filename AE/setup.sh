#!/bin/bash

################################ INSTALL MSR-SAFE #############################################
rm -rf msr-safe
BASE_DIRECTORY="`pwd`"
echo $BASE_DIRECTORY
git clone https://github.com/LLNL/msr-safe
cd msr-safe
git checkout e7f9423
make clean
make
cd ..
###############################################################################################

############################## BUILD RUNTIME ##################################################

rm -rf builds
mkdir builds
cd builds

################
#### HCLIB #####
################

rm -rf hclib 2>/dev/null
git clone https://github.com/habanero-rice/hclib.git
cd hclib
git checkout ab310a0
patch -p1 <../../hclib.patch
./install.sh
cd ..
###############################################################################################

##################
### CUTTLEFISH ###
##################

INSTALL_DIRECTORY="`pwd`/cuttlefish"
echo $INSTALL_DIRECTORY
verify_installation() {
  DIRNAME=$1
  if [ `cat out | grep "Libraries have been installed in" | wc -l` -eq 0 ]; then
    echo "INSTALLATION ERROR: $DIRNAME"
    exit
  fi
  rm -rf $INSTALL_DIRECTORY/$DIRNAME
  cp -rf cuttlefish-install $INSTALL_DIRECTORY/$DIRNAME
  echo "export CUTTLEFISH_ROOT=$INSTALL_DIRECTORY/$DIRNAME" > $INSTALL_DIRECTORY/$DIRNAME/bin/cuttlefish_setup_env.sh
  chmod +X $INSTALL_DIRECTORY/$DIRNAME/bin/cuttlefish_setup_env.sh
}

rm -rf $INSTALL_DIRECTORY 2>/dev/null
mkdir -p $INSTALL_DIRECTORY 2>/dev/null

cd $BASE_DIRECTORY/cuttlefish
echo $pwd

#Cuttlefish implementation without cuttlefish policy
./clean.sh
./install.sh  2>&1 | tee out
verify_installation "default"

#Cuttlefish implementation with core policy
./clean.sh
CUTTLEFISH_FLAGS=" --enable-core" ./install.sh  2>&1 | tee out
verify_installation "core"

#Cuttlefish implementation with uncore policy
./clean.sh
CUTTLEFISH_FLAGS=" --enable-uncore" ./install.sh  2>&1 | tee out
verify_installation "uncore"

#Cuttlefish implementation with combined (core + uncore) policy
./clean.sh
CUTTLEFISH_FLAGS=" --enable-combined" ./install.sh  2>&1 | tee out
verify_installation "combined"

#Cuttlefish implementation with motivation policy
./clean.sh
CUTTLEFISH_FLAGS=" --enable-motivation" ./install.sh  2>&1 | tee out
verify_installation "motivation"

##############################################################################################

################################ BUILD BENCHMARKS ############################################

cd $BASE_DIRECTORY/builds
mkdir benchmarks
cd benchmarks
rm -rf openmp
mkdir openmp
rm -rf hclib
mkdir hclib

build_benchmark(){
	DIRNAME=$1
	RUNTIME=$2

  	rm -rf $BASE_DIRECTORY/builds/benchmarks/$RUNTIME/$DIRNAME
	mkdir -p $BASE_DIRECTORY/builds/benchmarks/$RUNTIME/$DIRNAME
	source $INSTALL_DIRECTORY/$DIRNAME/bin/cuttlefish_setup_env.sh
	make
	cp -rf heat $BASE_DIRECTORY/builds/benchmarks/$RUNTIME/$DIRNAME
	cp -rf heatIR $BASE_DIRECTORY/builds/benchmarks/$RUNTIME/$DIRNAME
	cp -rf heatRR $BASE_DIRECTORY/builds/benchmarks/$RUNTIME/$DIRNAME
	cp -rf sor $BASE_DIRECTORY/builds/benchmarks/$RUNTIME/$DIRNAME
	cp -rf sorIR $BASE_DIRECTORY/builds/benchmarks/$RUNTIME/$DIRNAME
	cp -rf sorRR $BASE_DIRECTORY/builds/benchmarks/$RUNTIME/$DIRNAME
	if [ "$RUNTIME" == "openmp" ]; then
		cp -rf HPCCG/HPCCG $BASE_DIRECTORY/builds/benchmarks/$RUNTIME/$DIRNAME
		cp -rf miniFE/openmp/src/miniFE $BASE_DIRECTORY/builds/benchmarks/$RUNTIME/$DIRNAME
		cp -rf AMG/test/amg $BASE_DIRECTORY/builds/benchmarks/$RUNTIME/$DIRNAME
		cp -rf uts/uts $BASE_DIRECTORY/builds/benchmarks/$RUNTIME/$DIRNAME
		source uts/sample_trees.sh
	fi
	echo $CUTTLEFISH_ROOT
}

################
#### OpenMP ####
################

cd $BASE_DIRECTORY/apps_omp

# Build benchmarks without cuttlefish policy
build_benchmark "default" "openmp"

# Build benchmarks with cuttlefish-core policy
build_benchmark "core" "openmp"

# Build benchmarks with cuttlefish-uncore policy
build_benchmark "uncore" "openmp"

# Build benchmarks with cuttlefish-combined policy
build_benchmark "combined" "openmp"

# Build benchmarks with cuttlefish-combined policy
build_benchmark "motivation" "openmp"

################
#### HCLIB #####
################

cd $BASE_DIRECTORY/apps_hclib

source $BASE_DIRECTORY/builds/hclib/hclib-install/bin/hclib_setup_env.sh

# Build benchmarks without cuttlefish policy
build_benchmark "default" "hclib"

# Build benchmarks with cuttlefish-core policy
build_benchmark "core" "hclib"

# Build benchmarks with cuttlefish-uncore policy
build_benchmark "uncore" "hclib"

# Build benchmarks with cuttlefish-combined policy
build_benchmark "combined" "hclib"

##############################################################################################
