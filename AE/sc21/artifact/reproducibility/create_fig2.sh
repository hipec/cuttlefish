mkdir -p output

id=-1
if [ $# -eq 1 ]; then
        id=$1
fi

cd logs/timeline/
for i in *; do ../../scripts/policy_timeline.sh $i; done
mv timeline.txt ../../scripts
cd ../../
cd scripts

if [ $# -eq 1 ]; then 
	python figure2-a.py "$id"
	python figure2-b.py "$id"

else
	python figure2-a.py 
	python figure2-b.py

fi

mv *.png ../output
cd ../
cd scripts
rm *.txt
