#include "../src/registration.hpp"
#include "../src/volume.hpp"
#include <cassert>
#include <iostream>
int wmain(int argc,wchar_t** argv){
 if(argc!=3)return 2;auto a=readModel(argv[1]),b=readModel(argv[2]);const auto na=a.points.size(),nb=b.points.size();int last=-1;
 auto r=registration::align(a,b,{},[&](int p){assert(p>=last&&p<=100);last=p;});
 // Real reversed-bed regression: the former pose matched only a displaced
 // longitudinal wall. The independent structural rescue must recover both ends.
 assert(r.structureRefined&&r.overlap>.49&&r.rms<115);
 assert(std::abs(std::remainder(r.options.angle-180.,360.))<2);
 assert(r.options.dx>-600&&r.options.dx<-300);
 assert(!r.reliable&&last==100&&a.points.size()==na&&b.points.size()==nb);
 auto v=calculateVolume(a,b,{},r.options,50,true,true);
 assert(v.cargo.volume>28e9&&v.cargo.volume<30e9&&v.comparison.options.provisional);
 std::cout<<"PASS reversed 12942: wide structural consensus, both ends recovered, uncertainty retained, immutable inputs\n";
}
