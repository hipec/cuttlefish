
all: clean common hpccg minife amg tree

common:
	make -f makefile.omp
hpccg:
	cd HPCCG && make clean &&  make
minife:
	cd miniFE/openmp/src && make
amg:
	cd AMG && make
tree:
	cd uts && make
clean:
	make clean -f makefile.omp
	cd HPCCG && make clean
	cd miniFE/openmp/src && make clean && rm -rf miniFE
	cd AMG && make veryclean
	cd uts && make clean
