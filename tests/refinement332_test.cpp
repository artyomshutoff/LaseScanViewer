#include "../src/registration.hpp"
#include "../src/volume.hpp"
#include <cassert>
#include <iostream>
int wmain(int argc,wchar_t**argv){
 assert(rejectFragmentedRefinement(100,34,120));
 assert(!rejectFragmentedRefinement(100,98,120));
 assert(!rejectFragmentedRefinement(100,74,130));
 assert(!rejectFragmentedRefinement(0,0,0));
 assert(!rejectFragmentedRefinement(100,76,100));
 assert(!rejectFragmentedRefinement(100,120,100));
 assert(!rejectFragmentedRefinement(100,-1,100));
 assert(!rejectFragmentedRefinement(100,0,std::numeric_limits<double>::quiet_NaN()));
 if(argc>=3){auto a=readModel(argv[1]),b=readModel(argv[2]);auto alignment=registration::align(a,b,{});auto o=alignment.options;
  auto c=compareClouds(a,b,{},o);auto requested=buildCargo(c,50,true,&a,true,&b);int progress=-1;
  auto v=calculateVolume(a,b,{},o,50,true,true,[&](int n){assert(n>=progress&&n<=100);progress=n;});
  assert(v.comparison.refinementRejected&&v.comparison.options.provisional&&!v.cargo.calibrated);
  assert(v.cargo.volume==requested.volume&&v.comparison.options.step==o.step);
  assert(v.comparison.options.angle==o.angle&&v.comparison.options.dx==o.dx&&v.comparison.options.dy==o.dy&&v.comparison.options.dz==o.dz);
  assert(v.comparison.warning.find("раздробила")!=std::string::npos&&progress==100);
  assert(v.comparison.sensitivityMin<v.cargo.volume&&v.comparison.sensitivityMax>v.cargo.volume);
  o.adaptiveGrid=false;auto fixed=calculateVolume(a,b,{},o,50,true,true);assert(!fixed.comparison.refinementRejected&&fixed.cargo.volume==requested.volume);
  std::cout<<"PASS real fragmented scan, requested-grid geometry/integral, no model estimate, unchanged pose, provisional warning, sensitivity, monotonic progress and fixed-grid setting; volume "<<v.cargo.volume*1e-9<<" m3\n";
 }else std::cout<<"PASS supported/refined grid guards\n";
}
