#!/usr/bin/env python
# coding: utf-8

# In[29]:


#####
import matplotlib
matplotlib.use('Agg')
import pandas as pd
from operator import add
import numpy
import numpy as np
import sys
import glob


# In[30]:


TIPI={ }


# In[31]:
APP={}

def parse_log(file , bench):
    APP[bench]=[0.0,0.0]
    counter_names=0
    counter_values=0
    flag=0
    with open(file,'r',encoding='ISO-8859-1') as file_ptr:
        for line in file_ptr:
            if(line.find("============================ Tabulate Statistics ============================")!=-1):
                counter_names=1
                continue;
            if(counter_names==1):
                event = line.rstrip("\n").split("\t")[13:];
                event = event[:len(event)-1]
                for evt in range(0,len(event),5):
                    if event[evt] not in TIPI:
                        TIPI[str(event[evt])]={ 'uts':[float(0.0) , float(0.0)],'sorIR':[float(0.0) , float(0.0)],'sorRR':[float(0.0) , float(0.0)],'sor':[float(0.0) , float(0.0)],'heatIR':[float(0.0) , float(0.0)],'heatRR':[float(0.0) , float(0.0)],'heat':[float(0.0) , float(0.0)],'miniFE':[float(0.0) , float(0.0)],'HPCCG':[float(0.0) , float(0.0)], 'amg':[float(0.0) , float(0.0)]}
                        TIPI[str(event[evt+1])]={ 'uts':[int(0) , int(0)],'sorIR':[int(0) , int(0)],'sorRR':[int(0) , int(0)],'sor':[int(0) , int(0)],'heatIR':[int(0) , int(0)],'heatRR':[int(0) , int(0)],'heat':[int(0) , int(0)],'miniFE':[int(0) , int(0)],'HPCCG':[int(0) , int(0)], 'amg':[int(0) , int(0)]}
                        TIPI[str(event[evt+2])]={ 'uts':[int(0) , int(0)],'sorIR':[int(0) , int(0)],'sorRR':[int(0) , int(0)],'sor':[int(0) , int(0)],'heatIR':[int(0) , int(0)],'heatRR':[int(0) , int(0)],'heat':[int(0) , int(0)],'miniFE':[int(0) , int(0)],'HPCCG':[int(0) , int(0)], 'amg':[int(0) , int(0)]}
                counter_names=0
                counter_values=1
                continue;
            if(counter_values==1):
                new_val=line.rstrip("\n").split("\t")[10:13];
                APP[bench][0] += float(new_val[1])/float(new_val[0])*100
                APP[bench][1] += float(new_val[2])/float(new_val[0])*100
                flag+=1;
                values = line.rstrip("\n").split("\t")[13:];
                values = values[:len(values)-1]
                for val in range(0 ,len(values),5):
                    TIPI[str(event[val])][bench][0] += float(values[val])
                    TIPI[str(event[val])][bench][1] += 1
                    TIPI[str(event[val+1])][bench][0] += int(values[val+1])
                    TIPI[str(event[val+1])][bench][1] += 1
                    TIPI[str(event[val+2])][bench][0] += int(values[val+2])
                    TIPI[str(event[val+2])][bench][1] += 1
                counter_values=0
                continue;
            else:
                continue;
    APP[bench][0] = round(APP[bench][0]/flag)
    APP[bench][1] = round(APP[bench][1]/flag)


# In[32]:


dir=sys.argv[1]
file_list=glob.glob(str(dir) + "*.threads-20.C23.U30.log")


# In[33]:
#print(file_list)
ben_tipi=[]
for file in file_list:
    name = file.split('/')[-1]
    name = name.split('.')[0]
    ben_tipi.append(name)
    parse_log(file , name)

tipi_list = list(TIPI.keys())
#print(APP)

table = {}
bench=['uts','sorIR','sorRR','sor','heatIR','heatRR','heat','miniFE','HPCCG','amg']
bench_name={'uts':'UTS','sorIR':'SOR-irt','sorRR':'SOR-rt','sor':'SOR-ws','heatIR':'Heat-irt','heatRR':'Heat-rt','heat':'Heat-ws','miniFE':'MiniFE','HPCCG':'HPCCG','amg':'AMG'}

# In[36]:


for key in tipi_list:
    table[key]=[]
    #print(list(TIPI[key].keys()))
    for k in bench: #TIPI[key]:
        if str(key).find("PERC")!= -1:
            val = round(TIPI[key][k][0]/TIPI[key][k][1]*100) if TIPI[key][k][1]!=0 else -1
            table[key].append(val)
        else:
            val = round(TIPI[key][k][0]/TIPI[key][k][1]) if TIPI[key][k][1]!=0 else -1
            table[key].append(val)



#print(table)
# In[37]:
data={}
for b in range(len(bench)):
    data[bench[b]]={}
    for key in tipi_list:
        if table[key][b] != -1:
            new_k=str(key)[:15]
            if new_k not in data[bench[b]]:
                data[bench[b]][new_k]=[0.0, 0, 0]
            if str(key).find("PERC")!= -1:
                data[bench[b]][new_k][0]=table[key][b]
            elif str(key).find("CF")!= -1:
                data[bench[b]][new_k][1]=table[key][b]
            else:
                data[bench[b]][new_k][2]=table[key][b]


#print(data)
print("Benchmarks\tTIPI-Ranges(%)-CFopt\tTIPI-Ranges(%)-UFopt\tFrequent-TIPI-Ranges\tCuttlefish-CFopt\tCuttlefish-UFopt")
for ben in data:
	if ben in ben_tipi:
	    print(str(bench_name[ben])+"\t\t\t"+str(APP[ben][0])+"%\t\t\t"+str(APP[ben][1])+"%\t\t",end="")
	    for tipi in data[ben]:
		    if data[ben][tipi][0] >= 10:
			    print(str(tipi)+"("+str(data[ben][tipi][0])+"%)"+"\t\t"+str(data[ben][tipi][1])+"\t\t"+str(data[ben][tipi][2]))
			    print("\t\t\t\t\t\t\t\t",end="")
	    print("")










