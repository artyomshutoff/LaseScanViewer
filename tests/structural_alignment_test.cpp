#include "../src/registration.hpp"
#include <cassert>
#include <iostream>
int wmain(int argc,wchar_t** argv){if(argc!=3)return 2;auto a=readModel(argv[1]),b=readModel(argv[2]);auto na=a.points.size(),nb=b.points.size();int last=-1;auto r=registration::align(a,b,{},[&](int n){assert(n>=last&&n<=100);last=n;});assert(r.structureRefined&&r.overlap>.39&&r.rms<85);assert(r.options.dx>150&&r.options.dx<300);assert(!r.reliable&&last==100);assert(a.points.size()==na&&b.points.size()==nb);std::cout<<"PASS 13785 structural rescue, lower RMS, better coverage, retained uncertainty, progress and immutable scans\n";}
