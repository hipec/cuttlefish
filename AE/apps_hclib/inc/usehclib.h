#include <hclib.hpp>
namespace hclib {

template<typename T>
void __divide2_1D(int low, int high, int threshold, T && lambda);

template<typename T>
void __divide4_1D(int low, int high, int threshold, T && lambda){
    if(high-low > threshold && high-low>=4) {
        int chunk = (high - low) / 4;
	HCLIB_FINISH {
        hclib::async([=]() {
        __divide2_1D(low, low+chunk, threshold, lambda);
        });
        hclib::async([=]() {
        __divide2_1D(low+chunk, low + 2 * chunk, threshold, lambda);
        });
        hclib::async([=]() {
        __divide4_1D(low + 2 * chunk, low + 3 * chunk, threshold, lambda);
        });
        __divide2_1D(low + 3 * chunk, high, threshold, lambda);
	}
    } else {
        for(int i=low; i<high; i++) {
            lambda(i);
        }
    }
}

template<typename T>
void __divide2_1D(int low, int high, int threshold, T && lambda){
    if(high-low > threshold) {
	HCLIB_FINISH {
        hclib::async([=]() {
        __divide4_1D(low, (low+high)/2, threshold, lambda);
        });
        __divide2_1D((low+high)/2, high, threshold, lambda);
	}
    } else {
        for(int i=low; i<high; i++) {
            lambda(i);
        }
    }
}

template<typename T>
void _r_divide2_1D(int low, int high, int threshold, T && lambda){
    if(high-low > threshold) {
	HCLIB_FINISH {
        hclib::async([=]() {
        _r_divide2_1D(low, (low+high)/2, threshold, lambda);
        });
        _r_divide2_1D((low+high)/2, high, threshold, lambda);
	}
    } else {
        for(int i=low; i<high; i++) {
            lambda(i);
        }
    }
}

template<typename T>
void parallel_for_tasking(loop_domain_t* loop, T && lambda) {
    assert(loop->stride == 1);
    _r_divide2_1D(loop->low, loop->high, loop->tile, lambda);
}

template<typename T>
void parallel_for_irregular_tasking(loop_domain_t* loop, T && lambda) {
    assert(loop->stride == 1);
    __divide2_1D(loop->low, loop->high, loop->tile, lambda);
}

template<typename T>
void parallel_for(loop_domain_t* loop, T && lambda) {
    assert(loop->stride == 1);
    int numWorkers= num_workers();
    int batchSize = (loop->high - loop->low) / numWorkers;
    HCLIB_FINISH {
    for(int wid=0; wid<numWorkers; wid++) {
      hclib::async([=]() {
        int start = loop->low + wid * batchSize;
        int end = start + batchSize;
        for(int j=start; j<end; j++) {
            lambda(j);
        }
      });
    }
    }
    for(int end = batchSize*numWorkers + loop->low; end < loop->high; end++) {
        lambda(end);
    }
}

template<typename T>
void firsttouch_random_initialization(int low, int high, T* array1d, bool decimal) {
    int numWorkers= num_workers();
    int batchSize = high / numWorkers;
    HCLIB_FINISH {
    for(int wid=0; wid<numWorkers; wid++) {
      hclib::async([=]() {
        int start = wid * batchSize;
        int end = start + batchSize;
        unsigned int seed = wid+1;
        for(int j=start; j<end; j++) {
            int num = rand_r(&seed);
            array1d[j] = decimal? T(num/RAND_MAX) : T(num);
        }
      });
    }
    }
    for(int end = batchSize*numWorkers; end<high; end++) {
        unsigned int seed = numWorkers;
        int num = rand_r(&seed);
        array1d[end] = decimal? T(num/RAND_MAX) : T(num);
    }
}
}
