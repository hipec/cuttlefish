mkdir -p output
cd scripts
# Generate raw data for Table2
python FQ_opt.py ../logs/policy/OpenMP/*.combined >> ../output/Table-2.txt

