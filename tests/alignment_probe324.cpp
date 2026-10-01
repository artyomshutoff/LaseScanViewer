#ifdef ALIGN_BASELINE
#include "../build/alignment324/baseline/src/registration.hpp"
#include "../build/alignment324/baseline/src/volume.hpp"
#else
#include "../src/registration.hpp"
#include "../src/volume.hpp"
#endif
#include <iostream>
#include <iomanip>
int wmain(int argc,wchar_t** argv){if(argc<3)return 2;auto a=readModel(argv[1]),b=readModel(argv[2]);auto r=registration::align(a,b,{});if(argc==7){r.options.angle=std::stod(argv[3]);r.options.dx=std::stod(argv[4]);r.options.dy=std::stod(argv[5]);r.options.dz=std::stod(argv[6]);}auto v=calculateVolume(a,b,{},r.options,50,true,true);auto o=r.options;std::cout<<std::setprecision(16)<<a.scan<<','<<o.angle<<','<<o.dx<<','<<o.dy<<','<<o.dz<<','<<r.overlap<<','<<r.rms<<','<<o.aligned<<','<<v.cargo.volume*1e-9<<','<<(v.cargo.calibrated?v.cargo.calibratedVolume:v.cargo.volume)*1e-9<<'\n';if(argc==4){auto folder=std::filesystem::path(argv[3]);std::filesystem::create_directories(folder);for(auto pair:{std::pair<const char*,const Model*>{"full.csv",&a},{"empty.csv",&b}}){std::ofstream f(folder/pair.first);f<<"x,y,z\n";for(auto p:registration::sample(*pair.second,40,30000))f<<p.x<<','<<p.y<<','<<p.z<<'\n';}}
}
