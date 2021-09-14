import matplotlib
matplotlib.use('Agg')
import numpy as np
import matplotlib.axes as axes
import matplotlib.pyplot as plt
import statistics 
import math
import scipy.stats as st
import sys
#//https://www.geeksforgeeks.org/how-to-print-superscript-and-subscript-in-python/
def get_super(x):
    normal = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-=()"
    super_s = "ᴬᴮᶜᴰᴱᶠᴳᴴᴵᴶᴷᴸᴹᴺᴼᴾQᴿˢᵀᵁⱽᵂˣʸᶻᵃᵇᶜᵈᵉᶠᵍʰᶦʲᵏˡᵐⁿᵒᵖ۹ʳˢᵗᵘᵛʷˣʸᶻ⁰¹²³⁴⁵⁶⁷⁸⁹⁺⁻⁼⁽⁾"
    res = x.maketrans(''.join(normal), ''.join(super_s))
    return x.translate(res)

def read_core(file_flag):
    file="";
    if(file_flag==0):
        file="mapping.txt"
    else:
        file="timeline.txt"
    f = open(file, "r")
    data=[];
    for x in f:
        x=x.split(" ")
        x=x[0:5]
        for i in range(1,3):
            x[i]=float(x[i])
        x[-1]=x[-1].rstrip("\n")
        if(x[-1]=="U30"):
            del x[-1];
            data.append(x);

    m={}
    for i in data:
        m[i[0]]={};

    for i in data:
        k=(i[3]);
        if(k in m[i[0]] ):
            m[i[0]][k].append(i[1:3]);
        else:
            m[i[0]][k]=[i[1:3]];
    return m;

def read_uncore(file_flag):
    file="";
    if(file_flag==0):
        file="mapping.txt"
    else:
        file="timeline.txt"
    f = open(file, "r")
    data=[];
    for x in f:
        x=x.split(" ")
        x=x[0:5]
        for i in range(1,3):
            x[i]=float(x[i])
        x[-1]=x[-1].rstrip("\n")
        if(x[3]=="C23"):
            del x[3];
            data.append(x);

    m={}
    for i in data:
        m[i[0]]={};

    for i in data:
        k=(i[3]);
        if(k in m[i[0]] ):
            m[i[0]][k].append(i[1:3]);
        else:
            m[i[0]][k]=[i[1:3]];
    return m;


execution_slab=1000
slabs_to_discard=0;
TIPI_slab=0.004

def get_TIPI_index(tipi):
    return int(tipi/TIPI_slab)

def plot2_average_core_uncore(benc,freq_arr,scaling_flag):
    if(scaling_flag==0):
        m=read_uncore(0);
    else:
        m=read_core(0);
    dat=m[benc];
    m1={i:[] for i in freq_arr};
    m2={i:[] for i in freq_arr};
    for i in dat:
        if i in freq_arr:
            for j in dat[i]:
                m1[i].append(j[0]); 
                m2[i].append(j[1]);
    ret_jpi=[]
    ret_tipi=[]
    
    ret_deviation=[]
    
    for freq in range(len(freq_arr)):
        x=(m1[freq_arr[freq]]);
        y=(m2[freq_arr[freq]]);
        tp_map={}

        deviation_map={}
        
        count_map={}
        tipi_slab=[]
        average_jpi=[]
        total_slabs=0
        for i in range(len(x)):
            ind=get_TIPI_index(x[i]);
            if ind in tp_map:
                tp_map[ind]+=y[i];
                count_map[ind]+=1;
                deviation_map[ind].append(y[i]);
            
            else:
                tp_map[ind]=y[i];
                count_map[ind]=1;
                
                deviation_map[ind]=[y[i]];
            total_slabs+=1;

        tipi_list=[]
        jpi_list=[]
        deviation_list=[]

        for ind in sorted(tp_map.keys()):
            if(count_map[ind]/total_slabs>=0.1):
                tipi_list.append(ind);
                jpi_list.append(round(((tp_map[ind]/count_map[ind])*1000000000),3));
                deviation_list.append((st.t.interval(alpha=0.95, df=len(deviation_map[ind])-1, loc=np.mean(deviation_map[ind]), scale=st.sem(deviation_map[ind]))[1]-(tp_map[ind]/count_map[ind]))*1000000000);
                # print(benc,freq_arr[freq],"percentage:",round((deviation_list[-1]/jpi_list[-1])*100,2));
        
        ret_jpi.append(jpi_list);
        ret_tipi.append(tipi_list);
        ret_deviation.append(deviation_list);
    return ret_jpi,ret_tipi,ret_deviation;


def plot3_average(benchmarks,uncore_freq):
    uname=[]
    uname_jpi=[]
    uname_dev=[]
    umin_jpi=9999;
    umax_jpi=-9999;
    color =['red','blue','black','green','grey','yellow','purple','green']
    width=0.25
    for benc in benchmarks:

        jpi_list_uncore,tipi_list_uncore,deviation_uncore=plot2_average_core_uncore(benc,uncore_freq,0);

        if(benc=="heatIR"):
            benc="Heat-irt"
        if(benc=="uts"):
            benc="UTS"
        if(benc=="sorIR"):
            benc="SOR-irt"
        if(benc=="miniFE"):
            benc="MiniFE"
        if(benc=="HPCCG"):
            benc="HPCCG"
        if(benc=="amg"):
            benc="AMG"
        

        for num_tipi in range(len(jpi_list_uncore[0])):
            uname.append( benc+" ("+   str(round(tipi_list_uncore[0][num_tipi]*TIPI_slab,3))+"-"+str(round((tipi_list_uncore[0][num_tipi]+1)*TIPI_slab,3))   +")"  )
            jpi_freq=[]
            jpi_freq_Dev=[]

            #added by akshat
            if (len(jpi_list_uncore[0])==len(jpi_list_uncore[1])) and (len(jpi_list_uncore[1])==len(jpi_list_uncore[2])):
                pass
            else:
                print("There was some problem with the logs generated, please delete the log files of", benc, "benchmark from reproducibility/logs/mapping directory and relaunch the experiment for this benchmark")
                exit()

            for freq_ind in range(len(uncore_freq)):
                jpi_freq.append(jpi_list_uncore[freq_ind][num_tipi]);
                jpi_freq_Dev.append(deviation_uncore[freq_ind][num_tipi]);
                umin_jpi=min(umin_jpi,jpi_list_uncore[freq_ind][num_tipi]);
                umax_jpi=max(umax_jpi,jpi_list_uncore[freq_ind][num_tipi]);
            uname_jpi.append(jpi_freq);
            uname_dev.append(jpi_freq_Dev);

    

    figure = plt.gcf()
    figure.set_size_inches(15, 5)
    for freq_ind in range(3):
        arr_jpi=[]
        arr_dev=[]
        for nam in range(len(uname)):
            arr_jpi.append(uname_jpi[nam][freq_ind]);
            arr_dev.append(uname_dev[nam][freq_ind]);
            label_freq=""
            if(freq_ind==0):
                label_freq="UFmin"
            elif(freq_ind==1):
                label_freq="UFmid"
            elif(freq_ind==2):
                label_freq="UFmax"
        plt.bar(np.arange(len(uname))+width*freq_ind,arr_jpi, yerr=arr_dev,color = color[freq_ind],width = width,label=label_freq);

    plt.xlabel("Tor Inserts per Instruction (TIPI)")
    plt.ylabel('Joules Per 10'+get_super('9') +' Instructions (JPI)')
    plt.ylim(0,int(umax_jpi+1))
    plt.grid(axis = 'y')
    plt.xticks(np.arange(len(uname)) + width,uname,rotation=15,horizontalalignment='center');
    plt.legend(loc=1,fontsize=8)
    plt.savefig("figure3-b.png",dpi=300, bbox_inches='tight')
    plt.close()






uncore_freq=["U12","U21","U30"];
#benchmarks=['uts','sorIR','heatIR','miniFE','HPCCG','amg'];
benchmarks=['uts','heatIR','miniFE','HPCCG','sorIR','amg'];

# total arguments
n = len(sys.argv)
if (n==1):
    data_map=read_uncore(0);
    benchmarks1=[];
    for k in data_map:
        benchmarks1.append(k);
    if(len(benchmarks1)!=len(benchmarks)):
        benchmarks=benchmarks1;
    plot3_average(benchmarks,uncore_freq);
elif (n==2):
    benc=benchmarks[int(sys.argv[1])]
    plot3_average([benc],uncore_freq);

else :
    print("Invalid Arguments")
