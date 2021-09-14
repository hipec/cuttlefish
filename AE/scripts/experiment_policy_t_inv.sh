#!/bin/bash

#######################################                 CHANGE ALL PARAMETERS BASED ON ARCHITECTURE                   #############################################
SOCKET=2
CORE_PER_SOCKET=10
PHYSICAL_CORE=20
LOGICAL_CORE=40
####################################################################################################################################################################


BASE_DIRECTORY="`pwd`/.."

LOG_DIRECTORY="$BASE_DIRECTORY/sc21/artifact/reproducibility/logs/T_inv"

echo "BASE_DIRECORY:$BASE_DIRECTORY NUMBER OF SOCKET:$SOCKET NUMBER OF PHYSICAL CORE:$PHYSICAL_CORE NUMBER OF LOGICAL CORE:$LOGICAL_CORE"


BENCHMARKS=( "heat" "heatIR" "heatRR" "sor" "sorIR" "sorRR" "HPCCG" "miniFE" "amg" "uts" )



##################################################             CHANGE ITERATION                    #################################################################################
TIMED_ITERATIONS=10
MACHINE_WARMPUP_ITERATIONS=0
#####################################################################################################################################################################################



export CUTTLEFISH_INTERVAL=20

THREADS=( $PHYSICAL_CORE )

HIPPO=0
HASWELL=1


################################################################           CHANGE FREQUENCY PARAMETER               ################################################################

#DVFS_LEVELS_HASWELL=( 2300000 2200000 2100000 2000000 1900000 1800000 1700000 1600000 1500000 1400000 1300000 1200000 )
DVFS_LEVELS_HASWELL=( 2300000 1800000 1200000)

UNCORE_FREQ_REGISTER_VALUES=(0xc0c 0xd0d 0xe0e 0xf0f 0x1010 0x1111 0x1212 0x1313 0x1414 0x1515 0x1616 0x1717 0x1818 0x1919 0x1a1a 0x1b1b 0x1c1c 0x1d1d 0x1e1e)
UNCORE_FREQ_LEVELS=(1.2 1.3 1.4 1.5 1.6 1.7 1.8 1.9 2.0 2.1 2.2 2.3 2.4 2.5 2.6 2.7 2.8 2.9 3.0)
UNCORE_LOG_LEVELS=(12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30)
CORES_PER_NODE_HASWELL=$CORE_PER_SOCKET
###################################################################################################################################################################################

#HCLIB_INSTALL_DIRECTORY="$BASE_DIRECTORY/hclib/hclib-install/bin/hclib_setup_env.sh"
#CUTTLEFISH_INSTALL_DIRECTORY="$BASE_DIRECTORY/cuttlefish/cuttlefish-install/bin/cuttlefish_setup_env.sh"
#source $HCLIB_INSTALL_DIRECTORY
#source $CUTTLEFISH_INSTALL_DIRECTORY

insmod $BASE_DIRECTORY/msr-safe/msr-safe.ko

#export BASE=$BASE_DIRECTORY
#export CUTTLEFISH_ROOT=$BASE/cuttlefish/cuttlefish-install
#export HCLIB_ROOT=$BASE/hclib/hclib-install
#export LD_LIBRARY_PATH=$CUTTLEFISH_ROOT/lib:$HCLIB_ROOT/lib:$LD_LIBRARY_PATH

OLD_LD_PATH=$LD_LIBRARY_PATH

#export HCLIB_STATS=1
export HCLIB_BIND_THREADS=1

export OMP_NUM_THREADS=$PHYSICAL_CORE
export OMP_PLACES="{0:$CORE_PER_SOCKET:1},{$CORE_PER_SOCKET:$CORE_PER_SOCKET:1}"
export OMP_PROC_BIND=true


modprobe msr
chmod +rw /dev/cpu/*/msr_safe

setfrequency() {
    mode=$1
    core=0
    while [ $core -lt $LOGICAL_CORE ]; do
        cpufreq-set --cpu $core --freq $mode
        core=`expr $core + 1`
    done
    sleep 2
}

setuncorefrequency() {
    mode=$1
    socket=0
    while [ $socket -lt $SOCKET ]; do
        wrmsr -p$socket 0x620 "${UNCORE_FREQ_REGISTER_VALUES[$mode]}"
        socket=`expr $socket + 1`
    done
    sleep 2
}

launch() {
    exe=$1
    current_iteration=$2
    prefix=$3
    policy=$4
    cfreq=$5
    ufreq=$6
    echo "=============Launching experiment: $exe $current_iteration==============="
    for thread in "${THREADS[@]}"; do
        COMMAND="numactl -i all"
        unset HCLIB_WORKERS
        export HCLIB_WORKERS=$thread
	FILE="$LOG_DIR/`basename $exe`.3096.1024.$policy.threads-$thread.$prefix.log"
        date >> $BASE_DIRECTORY/scripts/akshat_profile_log.out 
        who >> $BASE_DIRECTORY/scripts/akshat_profile_log.out 
        echo "Currently Running: $FILE as follows: $COMMAND $exe"
	touch $FILE
	RHS=`expr $current_iteration + 1`
        while [ `cat $FILE | grep "Test PASSED" | wc -l` -lt $RHS ]; do
	if [ `echo $exe | grep uts | wc -l` -eq 1 ]; then
                $COMMAND $exe $T1XXL 2>&1 | tee out
        else
                $COMMAND $exe 2>&1 | tee out
        fi

        if [ "$current_iteration" -ge "$MACHINE_WARMPUP_ITERATIONS" ]; then
                cat out >> $FILE
                echo "Test Success: $FILE"

        fi
	if [ "$policy" == "default" ]; then
                likwid-setFrequencies --umin 1.2 --umax 3.0
        else
                setfrequency $cfreq
                likwid-setFrequencies --umin "${UNCORE_FREQ_LEVELS[$ufreq]}"  --umax "${UNCORE_FREQ_LEVELS[$ufreq]}"
        fi
        sleep 2
	done
    done
}





setgoverner() {
    mode=$1
    core=0
    while [ $core -lt $LOGICAL_CORE ]; do
        cpufreq-set --cpu $core --governor $mode
    echo 1200000 > "/sys/devices/system/cpu/cpufreq/policy$core/scaling_min_freq"
        core=`expr $core + 1`
    done
    sleep 2 # wait for 30 seconds
}


CUTTLEFISH_POLICY=( default core uncore combined )

export CUTTLEFISH_SHUTDOWN_DAEMON=0

##########################################################################################
BUILD_DIRECTORY="$BASE_DIRECTORY/builds"
CUTTLEFISH_DIRECTORY="$BUILD_DIRECTORY/cuttlefish"
HCLIB_DIRECTORY="$BUILD_DIRECTORY/hclib"
BENCHMARK_DIRECTORY="$BUILD_DIRECTORY/benchmarks/openmp"

TIME_INTERVAL=( 10 20 40 60 )

run_experiment() {
    NET_ITERATIONS=`expr $MACHINE_WARMPUP_ITERATIONS + $TIMED_ITERATIONS`

	for cuttlefish in 3 0 ; do
		export CUTTLEFISH_SHUTDOWN_DAEMON=0
		CUTTLEFISH_SRC="$CUTTLEFISH_DIRECTORY/${CUTTLEFISH_POLICY[$cuttlefish]}/bin/cuttlefish_setup_env.sh"
                source $CUTTLEFISH_SRC
                export LD_LIBRARY_PATH=$CUTTLEFISH_ROOT/lib:$OLD_LD_PATH
    		#cd $BASE_DIRECTORY/cuttlefish
    		#./clean.sh
    		if [ $cuttlefish -eq 0 ]; then
			    setgoverner performance
			    #sleep 30
			    export CUTTLEFISH_SHUTDOWN_DAEMON=1
			    #./install.sh
    		elif [ $cuttlefish -eq 1 ]; then
			setgoverner userspace
                        #sleep 30
        		#CUTTLEFISH_FLAGS="--enable-core" ./install.sh
    		elif [ $cuttlefish -eq 2 ]; then
			setgoverner userspace
                        #sleep 30
        		#CUTTLEFISH_FLAGS="--enable-uncore" ./install.sh
    		elif [ $cuttlefish -eq 3 ]; then
			setgoverner userspace
                        #sleep 30
        		#CUTTLEFISH_FLAGS="--enable-combined" ./install.sh
		else
			echo "INVALID BUILD FLAG"
			exit
    		fi
    		#cd -

		#cd $BASE_DIRECTORY/apps_omp
	        #make
    		source $BASE_DIRECTORY/apps_omp/uts/sample_trees.sh
    		#cd -

		if [ $cuttlefish -eq 3 ]; then
		for inv in 0 1 2 3 ; do
		export CUTTLEFISH_INTERVAL=${TIME_INTERVAL[$inv]}

		LOG_DIR="$LOG_DIRECTORY/T_${TIME_INTERVAL[$inv]}"
    		mkdir -p $LOG_DIR

    		for exe in "${BENCHMARKS[@]}"; do
			EXEC="$BENCHMARK_DIRECTORY/${CUTTLEFISH_POLICY[$cuttlefish]}/$exe"
			current_iteration=0
        		while [ $current_iteration -lt $NET_ITERATIONS ]; do

                		for freq in "${DVFS_LEVELS_HASWELL[0]}"; do
                        		for ind in 18 ; do
						core_freq=$((freq / 100000))
						uncore_freq="${UNCORE_LOG_LEVELS[$ind]}"
						prefix="C$core_freq.U$uncore_freq"
						policy="${CUTTLEFISH_POLICY[$cuttlefish]}"
						if [ $cuttlefish -eq 0 ]; then
							likwid-setFrequencies --umin 1.2 --umax 3.0
						else
                            				setfrequency $freq
                            				likwid-setFrequencies --umin "${UNCORE_FREQ_LEVELS[$ind]}"  --umax "${UNCORE_FREQ_LEVELS[$ind]}"
                            			fi
						sleep 2
                            			launch $EXEC $current_iteration $prefix $policy $freq $ind
                            			#cd -
                        		done
                		done

            			current_iteration=`expr $current_iteration + 1`
        		done
    		done
		done
		else
		LOG_DIR="$LOG_DIRECTORY/default"
                mkdir -p $LOG_DIR

                for exe in "${BENCHMARKS[@]}"; do
			EXEC="$BENCHMARK_DIRECTORY/${CUTTLEFISH_POLICY[$cuttlefish]}/$exe"
                        current_iteration=0
                        while [ $current_iteration -lt $NET_ITERATIONS ]; do

                                for freq in "${DVFS_LEVELS_HASWELL[0]}"; do
                                        for ind in 18 ; do
                                                core_freq=$((freq / 100000))
                                                uncore_freq="${UNCORE_LOG_LEVELS[$ind]}"
                                                prefix="C$core_freq.U$uncore_freq"
                                                policy="${CUTTLEFISH_POLICY[$cuttlefish]}"
                                                if [ $cuttlefish -eq 0 ]; then
                                                        likwid-setFrequencies --umin 1.2 --umax 3.0
                                                else
                                                        setfrequency $freq
                                                        likwid-setFrequencies --umin "${UNCORE_FREQ_LEVELS[$ind]}"  --umax "${UNCORE_FREQ_LEVELS[$ind]}"
                                                fi
                                                sleep 2
                                                launch $EXEC $current_iteration $prefix $policy $freq $ind
                                                #cd -
                                        done
                                done

                                current_iteration=`expr $current_iteration + 1`
                        done
                done

		fi

	done

	for inv in 0 1 2 3 ; do
		cp $LOG_DIRECTORY/default/* $LOG_DIRECTORY/T_${TIME_INTERVAL[$inv]}
	done

	rm -r $LOG_DIRECTORY/default

}


if [ $# -eq 2 ]; then
        TIMED_ITERATIONS=$2
        id=$1
        if [ $id -eq 0 ]; then
                BENCHMARKS=( "uts" )
        elif [ $id -eq 1 ]; then
                BENCHMARKS=( "heatIR" )
        elif [ $id -eq 2 ]; then
                BENCHMARKS=( "miniFE" )
        elif [ $id -eq 3 ]; then
                BENCHMARKS=( "HPCCG" )
        elif [ $id -eq 4 ]; then
                BENCHMARKS=( "sorIR" )
        elif [ $id -eq 5 ]; then
                BENCHMARKS=( "amg" )
        elif [ $id -eq 6 ]; then
                BENCHMARKS=( "heat" )
        elif [ $id -eq 7 ]; then
                BENCHMARKS=( "heatRR" )
        elif [ $id -eq 8 ]; then
                BENCHMARKS=( "sor" )
        elif [ $id -eq 9 ]; then
                BENCHMARKS=( "sorRR" )
        else
                echo "INVALID BENCHMARKS ID"
                exit
        fi
fi



run_experiment


setgoverner userspace
sleep 2
setfrequency ${DVFS_LEVELS_HASWELL[0]}
likwid-setFrequencies --umin 3.0  --umax 3.0

