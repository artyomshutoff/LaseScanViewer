#include "../src/simd_distance.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <chrono>
#include <iostream>
__attribute__((noinline)) double scalar(const double* a,const double* b){double sum=0;for(int i=0;i<190;i++){double d=a[i]-b[i];sum+=d*d;}return sum;}
__attribute__((noinline)) double accelerated(const double* a,const double* b){return descriptorDistance190(a,b);}
int main(){
 std::array<double,190> a{},b{};uint64_t state=37;
 for(int test=0;test<10000;test++){for(int i=0;i<190;i++){state=state*6364136223846793005ULL+1;a[i]=double(int32_t(state>>32))*.0000001;state=state*6364136223846793005ULL+1;b[i]=double(int32_t(state>>32))*.0000001;}double expected=scalar(a.data(),b.data());assert(std::abs(expected-accelerated(a.data(),b.data()))<=expected*1e-14);}
 auto time=[&](auto fn){auto start=std::chrono::steady_clock::now();volatile double total=0;for(int n=0;n<1000000;n++){a[0]=n*.000001;total=total+fn(a.data(),b.data());}return std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();};
 double normal=time(scalar),fast=time(accelerated);std::cout<<"PASS 10000 distances within 1e-14 relative; scalar "<<normal<<" s; SSE2 "<<fast<<" s; speedup "<<normal/fast<<"x\n";
}
