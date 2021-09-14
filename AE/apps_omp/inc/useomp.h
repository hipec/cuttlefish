#include <omp.h>
namespace hclib {

template<typename T>
void async(T &&lambda) {
    lambda();
}

#define HCLIB_FINISH
#define FORASYNC_MODE_RECURSIVE 1

typedef struct {
    int low;
    int high;
    int stride;
    int tile;
} loop_domain_t;

template<typename T>
void __divide2_1D(int low, int high, int threshold, T && lambda);

template<typename T>
void __divide4_1D(int low, int high, int threshold, T && lambda){
    if(high-low > threshold && high-low>=4) {
        int chunk = (high - low) / 4;
	#pragma omp task untied
        __divide2_1D(low, low+chunk, threshold, lambda);
	#pragma omp task untied
        __divide2_1D(low+chunk, low + 2 * chunk, threshold, lambda);
	#pragma omp task untied
        __divide4_1D(low + 2 * chunk, low + 3 * chunk, threshold, lambda);
	//#pragma omp task untied
        __divide2_1D(low + 3 * chunk, high, threshold, lambda);
	#pragma omp taskwait
    } else {
        for(int i=low; i<high; i++) {
            lambda(i);
        }
    }
}

template<typename T>
void __divide2_1D(int low, int high, int threshold, T && lambda){
    if(high-low > threshold) {
	#pragma omp task untied
        __divide4_1D(low, (low+high)/2, threshold, lambda);
	//#pragma omp task untied
        __divide2_1D((low+high)/2, high, threshold, lambda);
	#pragma omp taskwait
    } else {
        for(int i=low; i<high; i++) {
            lambda(i);
        }
    }
}

template<typename T>
void _r_divide2_1D(int low, int high, int threshold, T && lambda){
    if(high-low > threshold) {
	#pragma omp task untied
        _r_divide2_1D(low, (low+high)/2, threshold, lambda);
	//#pragma omp task untied
        _r_divide2_1D((low+high)/2, high, threshold, lambda);
	#pragma omp taskwait
    } else {
        for(int i=low; i<high; i++) {
            lambda(i);
        }
    }
}

template<typename T>
void parallel_for_irregular_tasking(loop_domain_t* loop, T && lambda) {
    assert(loop->stride == 1);
    #pragma omp parallel
    #pragma omp master
    #pragma omp task untied
    __divide2_1D(loop->low, loop->high, loop->tile, lambda);
}

template<typename T>
void parallel_for_tasking(loop_domain_t* loop, T && lambda) {
    assert(loop->stride == 1);
    #pragma omp parallel
    #pragma omp master
    #pragma omp task untied
    _r_divide2_1D(loop->low, loop->high, loop->tile, lambda);
}

template<typename T1>
void parallel_for(loop_domain_t* loop, T1 && lambda) {
    int i;
    #pragma omp parallel for schedule(static) private(i)
    for(i=loop->low; i<loop->high; i++) {
        lambda(i);
    }
}

int num_workers() {
    char* wn = getenv("OMP_NUM_THREADS"); 
    assert(wn!=NULL);
    return atoi(wn);
}

int current_worker() { return omp_get_thread_num(); }

template<typename T>
void finish(T &&lambda) {
    lambda();
}

template<typename T>
void firsttouch_random_initialization(int low, int high, T* array1d, bool decimal) {
    int numWorkers= num_workers();
    int batchSize = high / numWorkers;
    int wid;
    #pragma omp parallel for schedule(static) private(wid)
    for(wid=0; wid<numWorkers; wid++) {
        int start = wid * batchSize;
        int end = start + batchSize;
        unsigned int seed = wid+1;
        for(int j=start; j<end; j++) {
            int num = rand_r(&seed);
            array1d[j] = decimal? T(num/RAND_MAX) : T(num);
        }
    }
    for(int end = batchSize*numWorkers; end<high; end++) {
        unsigned int seed = numWorkers;
        int num = rand_r(&seed);
        array1d[end] = decimal? T(num/RAND_MAX) : T(num);
    }
}

template<typename T>
void launch(T && lambda) {
    lambda();
}

}
