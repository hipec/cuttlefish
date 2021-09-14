mkdir -p output

id=-1
if [ $# -eq 1 ]; then
        id=$1
fi

cd scripts
# create policy graphs
if [ $# -eq 1 ]; then 
	python policy_script.py ../logs/policy/OpenMP ../output figure10-a figure10-b figure10-c $id
else
	python policy_script.py ../logs/policy/OpenMP ../output figure10-a figure10-b figure10-c
fi

