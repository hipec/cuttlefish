mkdir -p output
id=-1
if [ $# -eq 1 ]; then
        id=$1
fi

cd scripts
# create policy graphs
if [ $# -eq 1 ]; then 
	#Generate T_inv Table
	python T_inv_choose.py ../logs/T_inv ../output Table-3.csv $id
else
	python T_inv_choose.py ../logs/T_inv ../output Table-3.csv
fi
