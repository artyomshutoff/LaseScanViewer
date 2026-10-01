#include "../src/registration.hpp"
#include "../src/volume.hpp"
#include <iostream>
#include <cassert>
#include <chrono>
int wmain(int argc,wchar_t** argv){if(argc!=3)return 2;auto& device=compute::runtime();if(!device.ready){std::cerr<<device.reason;return 3;}
 auto a=readModel(argv[1]),b=readModel(argv[2]);compute::choose(compute::Backend::CPU);auto start=std::chrono::steady_clock::now();auto cpu=registration::align(a,b,{});auto cv=calculateVolume(a,b,{},cpu.options,50,true,true);auto mid=std::chrono::steady_clock::now();std::cout<<"CPU completed\n"<<std::flush;
 compute::choose(compute::Backend::GPU);auto gpu=registration::align(a,b,{},[](int n){static int shown=-1;if(n/20>shown){shown=n/20;std::cout<<"GPU "<<n<<"%\n"<<std::flush;}});auto gv=calculateVolume(a,b,{},gpu.options,50,true,true);auto end=std::chrono::steady_clock::now();
 assert(compute::gpuBatches.load()>0&&compute::cpuFallbacks.load()==0);
 assert(cpu.options.angle==gpu.options.angle&&cpu.options.dx==gpu.options.dx&&cpu.options.dy==gpu.options.dy&&cpu.options.dz==gpu.options.dz&&cpu.overlap==gpu.overlap);
 assert(cv.cargo.volume==gv.cargo.volume&&cv.cargo.calibratedVolume==gv.cargo.calibratedVolume&&cv.comparison.sensitivityMin==gv.comparison.sensitivityMin&&cv.comparison.sensitivityMax==gv.comparison.sensitivityMax);
 std::cout<<"PASS exact CPU/GPU alignment and volume; batches "<<compute::gpuBatches.load()<<"; CPU "<<std::chrono::duration<double>(mid-start).count()<<" s; GPU "<<std::chrono::duration<double>(end-mid).count()<<" s\n";
}
