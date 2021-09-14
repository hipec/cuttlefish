mkdir -p output

id=-1
if [ $# -eq 1 ]; then
        id=$1
fi
cd scripts
# create policy graphs
if [ $# -eq 1 ]; then 
	python policy_script.py ../logs/policy/Hclib ../output figure11-a figure11-b figure11-c $id
else
	python policy_script.py ../logs/policy/Hclib ../output figure11-a figure11-b figure11-c
fi
