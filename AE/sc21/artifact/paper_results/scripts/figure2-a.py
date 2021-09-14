
import matplotlib
matplotlib.use('Agg')
import numpy as np
import matplotlib.axes as axes
import matplotlib.pyplot as plt
import statistics 
import math
import scipy.stats as st

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

#JPI vs TIPI plot for all the benchmarks
#key1 is the list of benchmarks
#key2 is the list of frequency ie [230000]
#flag tells whether you want to plot jpi or tipi 
def plot1(key1,key2,flag):
    max_exec_Slab=0;
    color =['black','green','red','blue','orange','grey','purple','yellow']
    fig, arr = plt.subplots();
    m=read_core(1);
    benc_count=0;


    for benc in key1:
        dat=m[benc];
        m1={i:[] for i in key2};
        m2={i:[] for i in key2};
        for i in dat:
            if i in key2:
                iter_count=0;
                for j in dat[i]:
                    iter_count+=1;
                    if (iter_count<=slabs_to_discard):
                        continue;
                    if(j[0]>0.0 and j[0]<1):
                        m1[i].append((j[0]//0.004)*0.004); 
                        m2[i].append(j[1]*1000000000);
        x=(m1[key2[0]]);
        y=(m2[key2[0]]);
        execution_slab=len(x);
        max_exec_Slab=max(max_exec_Slab,execution_slab);
        x_axis=[i/50 for i in range(slabs_to_discard+1,execution_slab+1+slabs_to_discard)];
        tot=(len(x));
        slab=tot//execution_slab;
        slab_mod=tot%execution_slab;
        print(tot,slab)
        jpi_list=[]
        tipi_list=[]
        count=0;
        temp_jpi=0;
        temp_tipi=0;
        slab_num=0;
        for i in range(tot):
            if(slab_num<slab_mod-1):
                if(count!=0 and count%(slab+1)==0):
                    slab_num+=1;
                    jpi_list.append((temp_jpi/count));
                    tipi_list.append(temp_tipi/count);
                    count=1;
                    temp_tipi=x[i];
                    temp_jpi=y[i];
                else:
                    count+=1;
                    temp_tipi+=x[i];
                    temp_jpi+=y[i];
            else:
                if(count!=0 and count%(slab)==0):
                    slab_num+=1;
                    jpi_list.append((temp_jpi/count));
                    tipi_list.append(temp_tipi/count);
                    count=1;
                    temp_tipi=x[i];
                    temp_jpi=y[i];
                else:
                    count+=1;
                    temp_tipi+=x[i];
                    temp_jpi+=y[i];
        if(slab_mod==0):
            jpi_list.append(temp_jpi/count);
            tipi_list.append(temp_tipi/count);
        print(len(tipi_list),len(jpi_list),len(x_axis));
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
        if(flag==0):
            plt.plot(x_axis,jpi_list,c=color[benc_count],label=benc)
        else:
            plt.plot(x_axis,tipi_list,c=color[benc_count],label=benc)
        benc_count+=1;
    arr.legend()
    arr.grid(True)
    figure = plt.gcf()
    figure.set_size_inches(10, 5)
    f_size=16;
    f_size1=13;
    matplotlib.rc('font', family='sans-serif') 
    matplotlib.rc('font', serif='Helvetica') 
    matplotlib.rc('text', usetex='false') 
    matplotlib.rcParams.update({'font.size': f_size1})
    if(flag==0):
        plt.xlabel('Execution Timeline (sec)',fontsize=f_size)
        plt.ylabel('Joules Per 10'+get_super('9') +' Instructions (JPI)',fontsize=f_size)
        handles,labels = arr.get_legend_handles_labels()
        new_handles=[handles[0],handles[4],handles[1], handles[2],handles[3],handles[5]]
        new_labels=[labels[0],labels[4],labels[1],labels[2],labels[3],labels[5]]
        arr.legend(new_handles,new_labels, loc=1,prop={'size':f_size1})
        plt.yticks(fontsize=f_size)
        plt.xticks([0,10,20,30,40,50,60,70,80],rotation=0,fontsize=f_size)
        plt.savefig('figure2-a.png', dpi=300, bbox_inches='tight')
    else:
        plt.xlabel('Execution Timeline (sec)',fontsize=f_size)
        plt.ylabel('TOR Inserts per Instructions (TIPI)',fontsize=f_size)
        handles,labels = arr.get_legend_handles_labels()
        new_handles=[handles[0],handles[4],handles[1], handles[2],handles[3],handles[5]]
        new_labels=[labels[0],labels[4],labels[1],labels[2],labels[3],labels[5]]
        arr.legend(new_handles,new_labels, loc=1,prop={'size':f_size1})
        plt.xticks([0,10,20,30,40,50,60,70,80],rotation=0,fontsize=f_size)
        plt.yticks(fontsize=f_size)
        plt.savefig('figure2-b.png', dpi=300, bbox_inches='tight')
    plt.close()


key2=["C23"]
benchmarks=['uts','heatIR','miniFE','HPCCG','sorIR','amg'];

plot1(benchmarks,key2,0);
