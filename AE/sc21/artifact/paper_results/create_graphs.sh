mkdir -p output
cd logs/mapping/
for i in *; do ../../scripts/policy_mapping.sh $i; done
mv mapping.txt ../../scripts
cd ../../

cd logs/timeline/
for i in *; do ../../scripts/policy_timeline.sh $i; done
mv timeline.txt ../../scripts
cd ../../
cd scripts
python figure2-a.py
python figure2-b.py
python figure3-a.py
python figure3-b.py

mv *.png ../output
cd ../
cd scripts
rm *.txt

# create policy graphs
python policy_script.py ../logs/policy/Hclib ../output figure11-a figure11-b figure11-c
python policy_script.py ../logs/policy/OpenMP ../output figure10-a figure10-b figure10-c

# Generate raw data for Table2
python FQ_opt.py ../logs/policy/OpenMP/*.combined >> ../output/Table-2.txt

#Generate T_inv Table
python T_inv_choose.py ../logs/T_inv ../output Table-3.csv
