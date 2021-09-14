
import pandas as pd
from operator import add
import numpy
import numpy as np
import sys
import glob
from statistics import stdev
from statistics import mean
import math
import numpy as np
import scipy.stats as st
from scipy.stats.mstats import gmean 

Bench_Name={'uts':'UTS','sorIR':'SOR-irt','sorRR':'SOR-rt','sor':'SOR-ws','heatIR':'Heat-irt','heatRR':'Heat-rt','heat':'Heat-ws','miniFE':'MiniFE','HPCCG':'HPCCG','amg':'AMG'}
Policy_Name={'combined':'Cuttlefish','core':'Cuttlefish-Core','uncore':'Cuttlefish-Uncore'}



def parse_log(file , bench , Dict):
	counter_names=0
	counter_values=0
	Energy_list=[]
	Time_list=[]
	with open(file,'r',encoding='ISO-8859-1') as file_ptr:
		for line in file_ptr:
			if(line.find("============================ Tabulate Statistics ============================")!=-1):
				counter_names=1
				continue;
			if(counter_names==1):
				counter_names=0
				counter_values=1
				continue;
			if(counter_values==1):
				values = line.rstrip("\n").split("\t")[:8];
				Energy_list.append(float(values[0]))
				Time_list.append(float(values[7]))
				counter_values=0
				continue;
			else:
				continue;
	Dict[bench] = [Energy_list , Time_list]
	return Dict



def normalise(policy , normaliser):
	normalised_value={}
	normalise_policy={}
	for bench in policy:
		normalise_policy[bench]=[[],[]];
		for i in range(len(policy[bench][0])):
			normalise_policy[bench][0].append(policy[bench][0][i]/np.mean(normaliser[bench][0]))
			normalise_policy[bench][1].append(policy[bench][1][i]/np.mean(normaliser[bench][1]))

		# Mean of Energy values on multiple invocations
		me = np.mean(normalise_policy[bench][0])
		# Mean of Time values on multiple invocations
		mt = np.mean(normalise_policy[bench][1])
		# Compute confidence interval for energy and time
		lower_e , upper_e = st.t.interval(alpha=0.95,  df=len(normalise_policy[bench][0])-1, loc=me, scale=st.sem(normalise_policy[bench][0]))
		lower_t , upper_t = st.t.interval(alpha=0.95,  df=len(normalise_policy[bench][1])-1, loc=mt, scale=st.sem(normalise_policy[bench][1]))

		# append mean and std_dev in dictionary
		normalised_value[bench]=[]
		normalised_value[bench].append(me)
		normalised_value[bench].append(mt)
		normalised_value[bench].append(upper_e - me)
		normalised_value[bench].append(upper_t - mt)
	return normalised_value;

def Execution(file_list):
	Policy_dict = { }
	Policy_default = { }

	# iterate in default log
	for file in file_list:
		log = file.split('/')[-1]
		name = log.split('.')[0]
		policy = log.split('.')[3]
		if policy not in Policy_dict:
			Policy_dict[policy]={}
		Policy_dict[policy] = parse_log(file , name , Policy_dict[policy]);

	Normalise_Dict={}
	for i in Policy_dict:
		if i != 'default':
			Normalise_Dict[i] = normalise(Policy_dict[i] , Policy_dict['default'])

	Geomean_Energy={}
	Geomean_Time={}
	for i in Normalise_Dict:
		Energy_List=[]
		Time_List=[]
		for bench in Normalise_Dict[i]:
			Energy_List.append(Normalise_Dict[i][bench][0])
			Time_List.append(Normalise_Dict[i][bench][1])
		Geomean_Energy[i] = gmean(Energy_List)
		Geomean_Time[i] = gmean(Time_List)
	return (Geomean_Energy , Geomean_Time)


#Enter directory path of logs as an input from user
inp_dir=sys.argv[1]
out_dir=sys.argv[2]
csv_name=sys.argv[3]

T_inv=[]
Energy=[]
Time=[]

dir_name = glob.glob(str(inp_dir) + "/*")
dir_name.sort()
#print(dir_name)
for sub_dir_path in dir_name:
	file_list=glob.glob(str(sub_dir_path) + "/*.log")
	file_list.sort()
	#print(file_list)
	Geo_Energy , Geo_Time = Execution(file_list);
	T_inv.append(str(sub_dir_path.split('/')[-1][2:]) + "ms")
	Energy.append(str(round(((1 - float(Geo_Energy['combined']))*100),1))+"%")
	Time.append(str(round(((float(Geo_Time['combined'])-1)*100),1))+"%")

#print(T_inv)

#create dataframe for generate table
df = pd.DataFrame(columns=['T_inv','Energy Savings','Slowdown'])
df['T_inv'] = T_inv
df['Energy Savings'] = Energy
df['Slowdown'] = Time

df.to_csv(out_dir +"/"+ csv_name, sep='\t', index=False,header=True)
