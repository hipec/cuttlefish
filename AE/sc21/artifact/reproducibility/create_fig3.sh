mkdir -p output

id=-1
if [ $# -eq 1 ]; then
        id=$1
fi
cd logs/mapping/
for i in *; do ../../scripts/policy_mapping.sh $i; done
mv mapping.txt ../../scripts
cd ../../
cd scripts

if [ $# -eq 1 ]; then 
	python figure3-a.py "$id"
	python figure3-b.py "$id"

else
	python figure3-a.py
	python figure3-b.py

fi

mv *.png ../output 2>/dev/null
cd ../
cd scripts
rm -rf *.txt

