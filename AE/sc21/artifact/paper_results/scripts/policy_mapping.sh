#!/bin/bash

run(){
	LOG="$1";
	echo "$LOG";
	flag=0;
	kernel=0;
	power=0;
	time=0;
	count=0
	#e=0;
	IFS='.' read -ra log <<< "$LOG"
	IFS='.' read -ra freq <<< "${log[1]}"
	while read LINE; 
	do  
		# echo "$LINE"
		if [[ "$LINE" == "============================ Unprocessed Statistics ============================" ]]; then
  			flag=1;
  		elif [[ $flag -eq 1 ]]; then
  			flag=2;
  		elif [[ $flag -eq 2 ]]; then
  			flag=3;
  		elif [[ "$LINE" == "=============================================================================" ]]; then
  			flag=4;
		fi

		if [[ $flag -eq 3 ]]; then
			if [[ $count -eq 100 ]]; then
				value=($LINE)
				tipi="${value[0]}"
				jpi="${value[1]}"
				echo "${log[0]} $tipi $jpi ${log[5]} ${log[6]}" >> mapping.txt
			else
				count=`expr $count + 1`
			fi

		fi

		if [[ $flag -eq 4 ]]; then
			break;
		fi
		
	
	done < $LOG
}

run $1
