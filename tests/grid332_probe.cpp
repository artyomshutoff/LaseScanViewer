#include "../src/registration.hpp"
#include "../src/volume.hpp"
#include <iostream>
#include <iomanip>
int wmain(int argc,wchar_t**argv){if(argc<3)return 2;auto a=readModel(argv[1]),b=readModel(argv[2]);auto o=registration::align(a,b,{}).options;
std::cout<<std::setprecision(15);for(double step:{50.,75.,90.,100.,110.,125.,150.}){auto v=o;v.step=step;auto c=compareClouds(a,b,{},v);auto g=buildCargo(c,50,true,nullptr,true,nullptr,false);auto all=buildCargo(c,50,false,nullptr,true,nullptr,false);std::cout<<step<<",cargo,"<<g.volume*1e-9<<",all,"<<all.volume*1e-9<<",positive,"<<c.positive*1e-9<<",repaired,"<<g.reconstructedVolume*1e-9<<",cells,"<<g.cells<<std::endl;}}
