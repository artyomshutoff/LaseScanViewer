#if defined(PERF_332_BASELINE)
#include "../build/control332/baseline/src/registration.hpp"
#include "../build/control332/baseline/src/calibration_features.hpp"
#elif defined(PERF_331_BASELINE)
#include "../build/perf331/baseline/src/registration.hpp"
#include "../build/perf331/baseline/src/calibration_features.hpp"
#else
#include "../src/registration.hpp"
#include "../src/calibration_features.hpp"
#endif
#include <iostream>
#include <iomanip>
#include <sstream>
#include <mutex>
thread_local std::array<double,190> recordedFeatures{};
thread_local bool recorded=false;
inline std::array<double,190> recordedVolumeFeatures(const Model& a,const Model& b,const Comparison& c,const Cargo& cargo,const std::function<void(int)>& progress={}){recorded=true;recordedFeatures=volumeFeatures(a,b,c,cargo,progress);return recordedFeatures;}
#define volumeFeatures recordedVolumeFeatures
#if defined(PERF_332_BASELINE)
#include "../build/control332/baseline/src/volume.hpp"
#elif defined(PERF_331_BASELINE)
#include "../build/perf331/baseline/src/volume.hpp"
#else
#include "../src/volume.hpp"
#endif
#undef volumeFeatures
std::string quoted(const std::string& s){std::string out="\"";for(char c:s){if(c=='\"')out+='\"';out+=c;}return out+'\"';}
#ifdef VOLUME_CACHE_332
// Verification only: replay the immutable 3.31 pose, not a target volume.
std::vector<std::string> csvColumns(const std::string& s){std::vector<std::string> out;std::string item;std::istringstream input(s);while(std::getline(input,item,',')){if(item.size()>1&&item.front()=='"')item=item.substr(1,item.size()-2);out.push_back(item);}if(!s.empty()&&s.back()==',')out.emplace_back();return out;}
#endif
int wmain(int argc,wchar_t** argv){
 if(argc<3)return 2;std::ifstream manifest(argv[1]);std::vector<std::pair<std::string,std::filesystem::path>> paths;std::string line;
 while(std::getline(manifest,line)){auto tab=line.find('\t');if(tab==std::string::npos)continue;paths.push_back({line.substr(0,tab),std::filesystem::u8path(line.substr(tab+1))});}
#ifdef VOLUME_CACHE_332
 if(argc<5)return 2;std::map<std::pair<std::string,unsigned long>,registration::Result> replay;std::ifstream previous(argv[4]);std::getline(previous,line);
 while(std::getline(previous,line)){auto row=csvColumns(line);if(row.size()<26||!row[25].empty())continue;registration::Result fit;auto& o=fit.options;o.angle=std::stod(row[11]);o.dx=std::stod(row[12]);o.dy=std::stod(row[13]);o.dz=std::stod(row[14]);fit.overlap=std::stod(row[15]);fit.rms=std::stod(row[16]);o.aligned=std::stoi(row[17]);fit.structureRefined=std::stoi(row[18]);o.bedHeightAdjustment=std::stod(row[19]);o.bedHeightSpread=std::stod(row[20]);o.bedHeightVariants=std::stoul(row[21]);o.alignmentOverlap=fit.overlap;o.provisional=!o.aligned||fit.overlap<.55;replay[{row[0],std::stoul(row[1])}]=fit;}
#endif
 std::filesystem::path output(argv[2]);std::filesystem::create_directories(output);std::ofstream csv(output/"raw.csv"),features(output/"features.csv");
 csv<<"dataset,id,label,group,geometric_m3,calculated_m3,calibrated,step,reconstructed_m3,minimum,maximum,angle,dx,dy,dz,overlap,rms,accepted,structure_refined,bed_shift,bed_spread,bed_variants,cells,body_excluded,seconds,error\n";
 features<<"dataset,id";for(int i=0;i<190;i++)features<<",f"<<i;features<<'\n';
 std::atomic<size_t> next{0},completed{0};std::mutex lock;std::vector<std::future<void>> jobs;int workers=argc>3?std::stoi(argv[3]):4;
 for(int w=0;w<workers;w++)jobs.push_back(std::async(std::launch::async,[&]{for(;;){size_t i=next++;if(i>=paths.size())return;auto [dataset,path]=paths[i];auto id=path.filename().string().substr(0,10);std::ostringstream row,feature;row<<std::setprecision(17)<<quoted(dataset)<<','<<id;
  try{auto start=std::chrono::steady_clock::now();auto a=readModel(path),b=readModel(path.parent_path()/(id+"_Empty.bin"));if(a.scan!=std::stoul(id)||b.scan!=a.scan||a.label!=b.label||a.kind!=2||b.kind!=1)throw std::runtime_error("Pair metadata mismatch");
#ifdef VOLUME_CACHE_332
   auto alignment=replay.at({dataset,std::stoul(id)});
#else
   auto alignment=registration::align(a,b,{});
#endif
   recorded=false;
   auto v=calculateVolume(a,b,{},alignment.options,50,true,true);if(!recorded)recordedFeatures=volumeFeatures(a,b,v.comparison,v.cargo);auto& o=v.comparison.options;
   row<<','<<quoted(a.label)<<','<<a.selectedType<<','<<v.cargo.volume*1e-9<<','<<(v.cargo.calibrated?v.cargo.calibratedVolume:v.cargo.volume)*1e-9<<','<<v.cargo.calibrated<<','<<o.step<<','<<v.cargo.reconstructedVolume*1e-9<<','<<v.comparison.sensitivityMin*1e-9<<','<<v.comparison.sensitivityMax*1e-9<<','<<o.angle<<','<<o.dx<<','<<o.dy<<','<<o.dz<<','<<alignment.overlap<<','<<alignment.rms<<','<<o.aligned<<','<<alignment.structureRefined<<','<<o.bedHeightAdjustment<<','<<o.bedHeightSpread<<','<<o.bedHeightVariants<<','<<v.cargo.cells<<','<<v.cargo.bodyPointsExcluded<<','<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<',';
   feature<<std::setprecision(17)<<quoted(dataset)<<','<<id;for(double f:recordedFeatures)feature<<','<<f;
  }catch(const std::exception& e){row<<std::string(24,',')<<quoted(e.what());}
  std::lock_guard<std::mutex> guard(lock);csv<<row.str()<<'\n';csv.flush();if(!feature.str().empty()){features<<feature.str()<<'\n';features.flush();}std::cout<<++completed<<'/'<<paths.size()<<" "<<dataset<<'/'<<id<<std::endl;
 }}));for(auto& job:jobs)job.get();return 0;
}
