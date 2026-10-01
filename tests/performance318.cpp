#ifdef PERF_BASELINE
#include "../build/perf318/baseline/src/registration.hpp"
#include "../build/perf318/baseline/src/volume.hpp"
#else
#include "../src/registration.hpp"
#include "../src/volume.hpp"
#endif
#include <chrono>
#include <iostream>
#include <iomanip>
int wmain(int argc,wchar_t** argv){
 if(argc!=4)return 2;
 try{auto clock=[](){return std::chrono::steady_clock::now();};auto seconds=[](auto a,auto b){return std::chrono::duration<double>(b-a).count();};
 auto t0=clock();auto a=readModel(argv[1]),b=readModel(argv[2]);triangulate(a);triangulate(b);auto t1=clock();
 auto alignment=registration::align(a,b,{});auto t2=clock();auto v=calculateVolume(a,b,{},alignment.options,50,true,true);auto t3=clock();
 std::ofstream out{std::filesystem::path(argv[3])};out<<std::setprecision(17)<<"load,align,volume,total,angle,dx,dy,dz,overlap,cargo,calibrated,step,min,max\n"<<seconds(t0,t1)<<','<<seconds(t1,t2)<<','<<seconds(t2,t3)<<','<<seconds(t0,t3)<<','<<alignment.options.angle<<','<<alignment.options.dx<<','<<alignment.options.dy<<','<<alignment.options.dz<<','<<alignment.overlap<<','<<v.cargo.volume<<','<<v.cargo.calibratedVolume<<','<<v.comparison.options.step<<','<<v.comparison.sensitivityMin<<','<<v.comparison.sensitivityMax<<'\n';
 std::cout<<"PASS benchmark "<<seconds(t0,t3)<<" seconds\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
