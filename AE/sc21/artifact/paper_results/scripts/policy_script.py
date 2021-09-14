
import matplotlib
matplotlib.use('Agg')
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

Policy_dict = { }
Policy_default = { }

def parse_default(file , bench , policy):
    counter_names=0
    counter_values=0
    Energy_list=[]
    Time_list=[]
    EDP_list=[]
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
                EDP_list.append(float(values[0])*float(values[7]))
                counter_values=0
                continue;
            else:
                continue;
    Policy_default[bench] = [np.mean(Energy_list) , np.mean(Time_list) , np.mean(EDP_list)]


def parse_log(file , name , policy):
    counter_names=0
    counter_values=0
    pkg_energy = 0.0
    time = 0.0
    pkg_count = 0
    time_count = 0
    Energy_list=[]
    Time_list=[]
    EDP_list=[]
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
                pkg_energy += float(values[0])
                time += float(values[7])
                Energy_list.append(float(values[0])/Policy_default[name][0])
                Time_list.append(float(values[7])/Policy_default[name][1])
                EDP_list.append(float(values[0])*float(values[7])/Policy_default[name][2])
                pkg_count += 1
                time_count += 1
                counter_values=0
                continue;
            else:
                continue;
    # Mean of Energy values on multiple invocations
    me = np.mean(Energy_list)
    # Mean of Time values on multiple invocations
    mt = np.mean(Time_list)
    # Mean of EDP values on multiple invocations
    mp = np.mean(EDP_list)

    # Computer confidence interval for energy and time
    lower_e , upper_e = st.t.interval(alpha=0.95,  df=len(Energy_list)-1, loc=me, scale=st.sem(Energy_list))
    lower_t , upper_t = st.t.interval(alpha=0.95,  df=len(Time_list)-1, loc=mt, scale=st.sem(Time_list))
    lower_p , upper_p = st.t.interval(alpha=0.95,  df=len(EDP_list)-1, loc=mp, scale=st.sem(EDP_list))

    # append mean and std_dev in dictionary
    Policy_dict[name][policy].append(me)
    Policy_dict[name][policy].append(mt)
    Policy_dict[name][policy].append(mp)
    Policy_dict[name][policy].append(upper_e - me)
    Policy_dict[name][policy].append(upper_t - mt)
    Policy_dict[name][policy].append(upper_p - mp)
    # print(me)
    # print(Energy_list)
    # print(upper_e - me)



#Enter directory path of logs as an input from user
inp_dir=sys.argv[1]
out_dir=sys.argv[2]
energy_fig_name=sys.argv[3]
time_fig_name=sys.argv[4]
edp_fig_name=sys.argv[5]
file_list=glob.glob(str(inp_dir) + "/*.log")
file_list.sort()





# iterate in default log
for file in file_list:
    log = file.split('/')[-1]
    name = log.split('.')[0]
    policy = log.split('.')[3]
    if str(policy) == 'default':
        parse_default(file , name , policy);



# iterate in policy log
for file in file_list:
    log = file.split('/')[-1]
    name = log.split('.')[0]
    policy = log.split('.')[3]
    if str(policy) != 'default':
        if name not in Policy_dict:
            Policy_dict[name] = { }
        if policy not in Policy_dict[name]:
            Policy_dict[name][policy] = []
        parse_log(file , name , policy);


#Sort Policy dictionary based on benchmarks
Policy_dict = {k: v for k, v in sorted(Policy_dict.items(), key=lambda item: item[0])}


x_list=[]
x_real=[]
for i in list(Bench_Name.keys()):
    if i in list(Policy_dict.keys()):
        x_list.append(Bench_Name[i])
        x_real.append(i)
# list that contains all benchmark's name
#x_list = list(Policy_dict.keys())


core_policy=[[],[],[],[],[],[]]
uncore_policy=[[],[],[],[],[],[]]
combined_policy=[[],[],[],[],[],[]]


for i in x_real:
    for j in Policy_dict[i]:
        if str(j)=='core':
            core_policy[0].append(Policy_dict[i][j][0])
            core_policy[1].append(Policy_dict[i][j][1])
            core_policy[2].append(Policy_dict[i][j][2])
            core_policy[3].append(Policy_dict[i][j][3])
            core_policy[4].append(Policy_dict[i][j][4])
            core_policy[5].append(Policy_dict[i][j][5])
        elif str(j)=='uncore':
            uncore_policy[0].append(Policy_dict[i][j][0])
            uncore_policy[1].append(Policy_dict[i][j][1])
            uncore_policy[2].append(Policy_dict[i][j][2])
            uncore_policy[3].append(Policy_dict[i][j][3])
            uncore_policy[4].append(Policy_dict[i][j][4])
            uncore_policy[5].append(Policy_dict[i][j][5])
        elif str(j)=='combined':
            combined_policy[0].append(Policy_dict[i][j][0])
            combined_policy[1].append(Policy_dict[i][j][1])
            combined_policy[2].append(Policy_dict[i][j][2])
            combined_policy[3].append(Policy_dict[i][j][3])
            combined_policy[4].append(Policy_dict[i][j][4])
            combined_policy[5].append(Policy_dict[i][j][5])




logs = ['' , 'min' , 'max' , 'mean' , 'geomean']

x_list.extend(logs)

for policy in ([core_policy , uncore_policy , combined_policy]):
    for i in range(len(policy)):
        if i<3:
            Min = min(policy[i])
            Max = max(policy[i])
            Mean = np.mean(policy[i])
            Geo = gmean(policy[i])
            policy[i].append(0)
            policy[i].append(Min)
            policy[i].append(Max)
            policy[i].append(Mean)
            policy[i].append(Geo)
        else:
            policy[i].extend([0,0,0,0,0])



import matplotlib.pyplot as plt

# set width of bar
barWidth = 0.25
my_dpi = 100

# Set position of bar on X axis
br1 = np.arange(len(x_list))
br2 = [x + barWidth for x in br1]
br3 = [x + barWidth for x in br2]


# Make the plot for energy
plt.figure(figsize=(1500/my_dpi, 600/my_dpi), dpi=my_dpi)
plt.bar(br1, combined_policy[0], yerr=combined_policy[3], color ='yellowgreen', width = barWidth,
        edgecolor ='grey', label =Policy_Name['combined'],zorder=3)
plt.bar(br2, core_policy[0], yerr=core_policy[3], color ='blue', width = barWidth,
        edgecolor ='grey', label =Policy_Name['core'],zorder=3)
plt.bar(br3, uncore_policy[0], yerr=uncore_policy[3], color ='orange', width = barWidth,
        edgecolor ='grey', label =Policy_Name['uncore'],zorder=3)


plt.title("Energy savings relative to Default", fontweight ='bold', fontsize = 15)
#plt.xlabel('Benchmarks', fontweight ='bold', fontsize = 15)
plt.ylabel('Energy relative to Default(Lower the better)', fontweight ='bold', fontsize = 10)
plt.xticks([r + barWidth for r in range(len(x_list))],x_list, rotation=30, horizontalalignment='right')
plt.ylim((0.7,1.25))
plt.yticks(np.arange(0.7, 1.25, 0.05))
plt.grid(axis="y")
plt.legend(loc='upper right')
#plt.show()
plt.savefig(str(out_dir) + '/' + energy_fig_name + '.png',dpi=my_dpi)
plt.close()

# Make the plot for performance
plt.figure(figsize=(1500/my_dpi, 600/my_dpi), dpi=my_dpi)
plt.bar(br1, combined_policy[1], yerr=combined_policy[4], color ='yellowgreen', width = barWidth,
        edgecolor ='grey', label =Policy_Name['combined'], zorder=3)
plt.bar(br2, core_policy[1], yerr=core_policy[4], color ='blue', width = barWidth,
        edgecolor ='grey', label =Policy_Name['core'], zorder=3)
plt.bar(br3, uncore_policy[1], yerr=uncore_policy[4], color ='orange', width = barWidth,
        edgecolor ='grey', label =Policy_Name['uncore'], zorder=3)


plt.title("Execution time relative to Default", fontweight ='bold', fontsize = 15)
#plt.xlabel('Benchmarks', fontweight ='bold', fontsize = 15)
plt.ylabel('Time relative to Default(Lower the better)', fontweight ='bold', fontsize = 10)
plt.xticks([r + barWidth for r in range(len(x_list))],x_list, rotation=30, horizontalalignment='right')
plt.ylim((0.99,1.1))
plt.yticks(np.arange(0.99, 1.1, 0.01))
plt.grid(axis="y")
plt.legend(loc='upper right')
#plt.show()
plt.savefig(str(out_dir) + '/' + time_fig_name + '.png',dpi=my_dpi)
plt.close()

# Make the plot for Energy-Delay Product
plt.figure(figsize=(1500/my_dpi, 600/my_dpi), dpi=my_dpi)
plt.bar(br1, combined_policy[2], yerr=combined_policy[5], color ='yellowgreen', width = barWidth,
        edgecolor ='grey', label =Policy_Name['combined'], zorder=3)
plt.bar(br2, core_policy[2], yerr=core_policy[5], color ='blue', width = barWidth,
        edgecolor ='grey', label =Policy_Name['core'], zorder=3)
plt.bar(br3, uncore_policy[2], yerr=uncore_policy[5], color ='orange', width = barWidth,
        edgecolor ='grey', label =Policy_Name['uncore'], zorder=3)


plt.title("EDP relative to Default", fontweight ='bold', fontsize = 15)
#plt.xlabel('Benchmarks', fontweight ='bold', fontsize = 15)
plt.ylabel('EDP relative to Default(Lower the better)', fontweight ='bold', fontsize = 10)
plt.xticks([r + barWidth for r in range(len(x_list))],x_list, rotation=30, horizontalalignment='right')
plt.ylim((0.7,1.25))
plt.yticks(np.arange(0.7, 1.25, 0.05))
plt.grid(axis="y")
plt.legend(loc='upper right')
#plt.show()
plt.savefig(str(out_dir) + '/' + edp_fig_name + '.png',dpi=my_dpi)
plt.close()
