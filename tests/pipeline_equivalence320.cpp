#ifdef PERF_331_BASELINE
#include "../build/perf331/baseline/src/volume.hpp"
#elif defined(PERF_BASELINE)
#include "../build/perf320/baseline/src/volume.hpp"
#else
#include "../src/volume.hpp"
#endif
#include <sstream>
#include <iostream>
#include <iomanip>
struct Digest{uint64_t value=1469598103934665603ULL;template<class T>void add(const T& x){auto p=(const unsigned char*)&x;for(size_t i=0;i<sizeof(T);i++){value^=p[i];value*=1099511628211ULL;}}};
int wmain(int argc,wchar_t** argv){if(argc!=5)return 2;try{
 auto a=readModel(argv[1]),b=readModel(argv[2]);std::ifstream poses{std::filesystem::path(argv[3])};std::string line;std::getline(poses,line);std::getline(poses,line);std::stringstream ss(line);std::vector<double> values;while(std::getline(ss,line,','))values.push_back(std::stod(line));
 CompareOptions o;o.angle=values.at(4);o.dx=values.at(5);o.dy=values.at(6);o.dz=values.at(7);o.aligned=true;o.alignmentOverlap=values.at(8);
 std::ofstream output{std::filesystem::path(argv[4])};output<<std::setprecision(17)<<"mode,volume,calibrated,min,max,positive,negative,cells,cloud,faces,digest,provisional,checked\n";
 for(int mode=0;mode<6;mode++){auto options=o;Region roi;bool largest=true,clean=true;
  if(mode==1){roi=bounds(a);roi.x0+=(roi.x1-roi.x0)*.1;roi.y1-=(roi.y1-roi.y0)*.1;}
  if(mode==2){options.adaptiveGrid=false;options.reconstructGaps=false;}
  if(mode==3){options.estimator=3;options.step=125;}
  if(mode==4){largest=false;clean=false;}
  if(mode==5){options={};options.aligned=true;}
  int progress=-1;auto v=calculateVolume(mode==5?b:a,b,roi,options,50,largest,clean,[&](int n){if(n<progress||n<0||n>100)throw std::runtime_error("Invalid progress");progress=n;});if(progress!=100)throw std::runtime_error("Incomplete calculation");
  Digest d;for(auto p:v.cargo.cloud.points){d.add(p.x);d.add(p.y);d.add(p.z);}for(auto p:v.cargo.geometry.points){d.add(p.x);d.add(p.y);d.add(p.z);}for(auto f:v.cargo.geometry.faces)for(auto i:f)d.add(i);
  for(auto& kv:v.comparison.cells){d.add(kv.first.first);d.add(kv.first.second);d.add(kv.second.base);d.add(kv.second.delta);}
  output<<mode<<','<<v.cargo.volume<<','<<v.cargo.calibratedVolume<<','<<v.comparison.sensitivityMin<<','<<v.comparison.sensitivityMax<<','<<v.comparison.positive<<','<<v.comparison.negative<<','<<v.comparison.cells.size()<<','<<v.cargo.cloud.points.size()<<','<<v.cargo.geometry.faces.size()<<','<<d.value<<','<<v.comparison.options.provisional<<','<<v.comparison.sensitivityChecked<<'\n';
 }
 std::cout<<"PASS six modes and monotonic progress\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
