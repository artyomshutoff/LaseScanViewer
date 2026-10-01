#include "../src/registration.hpp"
#include <cassert>
#include <iostream>
int wmain(int argc,wchar_t** argv){
    if(argc!=2)return 2;
    auto root=std::filesystem::path(argv[1]);
    auto a=readModel(root/L"0000006066_Full.bin"),b=readModel(root/L"0000006066_Empty.bin");
    int last=-1,calls=0;
    auto r=registration::align(a,b,{},[&](int percent){assert(percent>=last&&percent<=100);last=percent;++calls;});
    // The asymmetric raised end must be on the same side. This fixture checks
    // the reversed orientation, not an independently calibrated cargo volume.
    assert(r.options.aligned);
    assert(std::abs(std::remainder(r.options.angle-180,360.))<3);
    assert(r.rms<110&&r.overlap>.48);
    assert(last==100&&calls>100);
    std::cout<<"PASS Learn 6066: yaw="<<r.options.angle<<" rms="<<r.rms<<" overlap="<<r.overlap<<" progress="<<last<<"\n";
}
