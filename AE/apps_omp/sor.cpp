#include <unistd.h>
#include "common.h"
#include "cuttlefish.h"

#define ELEMENT_t float
#define OMEGA 1.25
#define THRESHOLD 2
ELEMENT_t* G;
 
void compute(int i, int size) {
    ELEMENT_t omega_over_four = OMEGA * 0.25;
    ELEMENT_t one_minus_omega = 1.0 - OMEGA;
    int Mm1 = size-1;
    int Nm1 = size-1;
    ELEMENT_t* Gi = &G[i*size];
    const int im1 = i-1;
    ELEMENT_t* Gim1 = &G[im1*size];
    const int ip1 = i+1;
    ELEMENT_t* Gip1 = &G[ip1*size];
    for (int j=1; j<Nm1; j++) {
        Gi[j] = omega_over_four * (Gim1[j] + Gip1[j] + Gi[j-1] + Gi[j+1]) + one_minus_omega * Gi[j];
    }
}

void parallel_kernel(int size) {
    hclib::loop_domain_t loop = {1, size-1, 1, THRESHOLD};
    auto start = [=](int i) { return (i-1)*size; };
    auto end = [=](int i) { return i*size; };
    hclib::parallel_for(&loop, [=] (int i) {
	compute(i, size);
    });
}

int main(int argc, char **argv) {
    cuttlefish::log_header();
    hclib::launch([=]() {
       int size = argc>1?atoi(argv[1]): 32768; //use multiples of PAGE_SIZE
       int iterations = argc>2?atoi(argv[2]): 200; 
       printf("SOR: size=%d, num_iterations=%d\n",size,iterations);
       G = hclib::my_malloc<ELEMENT_t>(size * size);
       hclib::firsttouch_random_initialization(0, size, G, true);
       printf("SOR started..\n");
       cuttlefish::start();
       hclib::kernel([&]() {
           for (int p=0; p<iterations; p++) {
	       parallel_kernel(size);
	   }
       }); 
       cuttlefish::stop();
       printf("SOR ended..\n");
       hclib::my_free(G);
    });
    cuttlefish::log_footer();
    return 0;
}
