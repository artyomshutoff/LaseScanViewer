#include "../src/registration.hpp"
#include "../src/volume.hpp"
#include <iostream>
#include <iomanip>
int wmain(int argc,wchar_t** argv){if(argc<3)return 2;auto a=readModel(argv[1]),b=readModel(argv[2]);auto alignment=registration::align(a,b,{});auto o=alignment.options;auto original=compareClouds(a,b,{},o);std::cout<<std::setprecision(15)<<"pose,"<<o.angle<<','<<o.dx<<','<<o.dy<<','<<o.dz<<",overlap,"<<alignment.overlap<<",positive,"<<original.positive*1e-9<<",negative,"<<original.negative*1e-9<<'\n';
 for(bool clean:{true,false})for(bool largest:{true,false}){auto v=calculateVolume(a,b,{},o,50,largest,clean);std::cout<<clean<<','<<largest<<",geometric,"<<v.cargo.volume*1e-9<<",final,"<<(v.cargo.calibrated?v.cargo.calibratedVolume:v.cargo.volume)*1e-9<<",cells,"<<v.cargo.cells<<",removed,"<<v.cargo.removedCells<<",baseZ,"<<v.cargo.geometry.lo.z<<",topZ,"<<v.cargo.geometry.hi.z<<'\n';}
}
