#include "../src/registration.hpp"
#include "../src/volume.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <future>
#include <atomic>
#include <mutex>
// Targets are intentionally absent. This exercises the same registration and
// calculateVolume entry points as the desktop application, including weak poses.
int main(){std::vector<std::filesystem::path> files;for(auto&e:std::filesystem::directory_iterator("n-gk")){
 auto name=e.path().filename().string();if(name.find("_Full.bin")==std::string::npos)continue;
 auto empty=e.path().parent_path()/(name.substr(0,10)+"_Empty.bin");if(std::filesystem::exists(empty))files.push_back(e.path());
}std::sort(files.begin(),files.end());if(files.size()!=89)return 2;
 std::ofstream csv("build/ngk-full-350.csv");csv<<"id,volume,step,reconstructed,minimum,maximum,angle,dx,dy,dz,overlap,accepted,null_volume,calibrated,calibrated_m3,error\n";
 std::atomic<size_t> next{0};std::mutex lock;std::vector<std::future<void>> workers;
 for(int worker=0;worker<6;worker++)workers.push_back(std::async(std::launch::async,[&]{for(;;){size_t i=next++;if(i>=files.size())break;auto path=files[i];std::string id=path.stem().string().substr(0,10);std::ostringstream line;line<<std::setprecision(12)<<id;
 try{auto a=readModel(path),b=readModel(path.parent_path()/(id+"_Empty.bin"));
 if(a.scan!=std::stoul(id)||b.scan!=a.scan||a.label!=b.label||a.kind!=2||b.kind!=1)throw std::runtime_error("Pair metadata mismatch");
 auto alignment=registration::align(a,b,{});auto v=calculateVolume(a,b,{},alignment.options,50,true,true);
 auto zero=CompareOptions{};zero.aligned=true;auto self=calculateVolume(b,b,{},zero,50,true,true);if(self.cargo.volume!=0)throw std::runtime_error("Nonzero self volume");
 auto&o=v.comparison.options;line<<','<<v.cargo.volume/1e9<<','<<o.step<<','<<v.cargo.reconstructedVolume/1e9<<','<<v.comparison.sensitivityMin/1e9<<','<<v.comparison.sensitivityMax/1e9<<','<<o.angle<<','<<o.dx<<','<<o.dy<<','<<o.dz<<','<<alignment.overlap<<','<<o.aligned<<','<<self.cargo.volume<<','<<v.cargo.calibrated<<','<<v.cargo.calibratedVolume/1e9<<',';
 }catch(const std::exception&e){line<<",,,,,,,,,,,,,"<<e.what();}
 std::lock_guard<std::mutex> guard(lock);csv<<line.str()<<'\n';csv.flush();std::cout<<id<<" completed\n"<<std::flush;
 }}));for(auto&w:workers)w.get();return csv?0:3;
}

