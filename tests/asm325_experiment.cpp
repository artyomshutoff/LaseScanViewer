#include <chrono>
#include <iostream>
#include <vector>
#include <random>
#include <cassert>
struct P {float x,y,z;};
inline double cpp(P a,P b){double x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;}
inline double assembly(const P& a,const P& b){double result;
 __asm__("movq (%[a]), %%xmm1\n\tmovq (%[b]), %%xmm2\n\tsubps %%xmm2, %%xmm1\n\tcvtps2pd %%xmm1, %%xmm1\n\tmulpd %%xmm1, %%xmm1\n\tmovapd %%xmm1, %%xmm2\n\tunpckhpd %%xmm2, %%xmm2\n\taddsd %%xmm2, %%xmm1\n\tmovss 8(%[a]), %%xmm2\n\tsubss 8(%[b]), %%xmm2\n\tcvtss2sd %%xmm2, %%xmm2\n\tmulsd %%xmm2, %%xmm2\n\taddsd %%xmm2, %%xmm1\n\tmovsd %%xmm1, %[result]" : [result] "=x"(result):[a]"r"(&a),[b]"r"(&b):"xmm1","xmm2","memory");return result;}
int main(){std::mt19937 rng(5);std::uniform_real_distribution<float> r(-10000,10000);std::vector<P> a,b;for(int i=0;i<100000;i++){a.push_back({r(rng),r(rng),r(rng)});b.push_back({r(rng),r(rng),r(rng)});assert(cpp(a.back(),b.back())==assembly(a.back(),b.back()));}volatile double sink=0;for(int pass=0;pass<4;pass++){auto start=std::chrono::steady_clock::now();double sum=0;for(int j=0;j<100;j++)for(size_t i=0;i<a.size();i++)sum+=pass%2?assembly(a[i],b[i]):cpp(a[i],b[i]);sink=sum;std::cout<<(pass%2?"ASM ":"CPP ")<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<" s\n";}return sink==0;}
