#ifdef PERF_BASELINE
#include "../build/perf331/baseline/src/registration.hpp"
#include "../build/perf331/baseline/src/volume.hpp"
#else
#include "../src/registration.hpp"
#include "../src/volume.hpp"
#endif
#include <chrono>
#include <iostream>
#include <iomanip>
#include <cstring>
struct Digest{
 uint64_t h=14695981039346656037ULL;
 template<class T>void add(T v){unsigned char bytes[sizeof(T)];std::memcpy(bytes,&v,sizeof(v));for(auto b:bytes){h^=b;h*=1099511628211ULL;}}
 void model(const Model& m){add(m.points.size());add(m.faces.size());for(auto p:m.points){add(p.x);add(p.y);add(p.z);}for(auto f:m.faces){add(f[0]);add(f[1]);add(f[2]);}}
};
int wmain(int argc,wchar_t** argv){if(argc!=4)return 2;try{
 auto now=[](){return std::chrono::steady_clock::now();};auto seconds=[](auto a,auto b){return std::chrono::duration<double>(b-a).count();};
 auto t0=now();auto a=readModel(argv[1]),b=readModel(argv[2]);triangulate(a);triangulate(b);auto t1=now();
 int ap=-1,vp=-1;auto alignment=registration::align(a,b,{},[&](int n){if(n<ap||n>100)throw std::runtime_error("Alignment progress");ap=n;});auto t2=now();
 auto v=calculateVolume(a,b,{},alignment.options,50,true,true,[&](int n){if(n<vp||n>100)throw std::runtime_error("Volume progress");vp=n;});auto t3=now();if(ap!=100||vp!=100)throw std::runtime_error("Incomplete progress");
 Digest digest;digest.model(v.cargo.geometry);digest.model(v.cargo.cloud);
 for(auto&[k,c]:v.comparison.cells){digest.add(k.first);digest.add(k.second);digest.add(c.active);digest.add(c.base);digest.add(c.delta);digest.add(c.area);digest.add(c.reconstructed);}
 auto&o=alignment.options;
 std::ofstream out{std::filesystem::path(argv[3])};out<<std::setprecision(17)<<"load,align,volume,total,angle,dx,dy,dz,overlap,rms,accepted,reliable,structure,bed_shift,bed_spread,bed_variants,cargo,calibrated,model_used,step,min,max,datum_min,datum_max,reconstructed,cells,removed,body_excluded,digest\n"<<seconds(t0,t1)<<','<<seconds(t1,t2)<<','<<seconds(t2,t3)<<','<<seconds(t0,t3)<<','<<o.angle<<','<<o.dx<<','<<o.dy<<','<<o.dz<<','<<alignment.overlap<<','<<alignment.rms<<','<<o.aligned<<','<<alignment.reliable<<','<<alignment.structureRefined<<','<<o.bedHeightAdjustment<<','<<o.bedHeightSpread<<','<<o.bedHeightVariants<<','<<v.cargo.volume<<','<<v.cargo.calibratedVolume<<','<<v.cargo.calibrated<<','<<v.comparison.options.step<<','<<v.comparison.sensitivityMin<<','<<v.comparison.sensitivityMax<<','<<v.comparison.datumSensitivityMin<<','<<v.comparison.datumSensitivityMax<<','<<v.cargo.reconstructedVolume<<','<<v.cargo.cells<<','<<v.cargo.removedCells<<','<<v.cargo.bodyPointsExcluded<<','<<digest.h<<'\n';
 std::cout<<"PASS "<<seconds(t0,t3)<<" seconds\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
