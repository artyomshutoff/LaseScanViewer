#include "../src/registration.hpp"
#include "../src/volume.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <future>
#include <atomic>
#include <mutex>
#include <chrono>

// No database or control volumes are read here. Every pair runs the application
// registration and volume calculation from scratch, including rejected poses.
static std::string csvText(const std::string& value){
    std::string out="\"";for(char c:value){if(c=='\"')out+='\"';out+=c;}return out+'\"';
}
int main(){
    std::set<std::string> known;
    std::ifstream manifest("build/research38/manifest.csv");std::string item;
    std::getline(manifest,item);
    while(std::getline(manifest,item)){auto comma=item.find(',');known.insert(item.substr(0,comma)+"/"+std::to_string(std::stoul(item.substr(comma+1))));}
    std::vector<std::filesystem::path> files;
    for(auto folder:{"n-gk","dmu"}){
        size_t before=files.size();
        for(const auto& e:std::filesystem::directory_iterator(folder)){
            auto name=e.path().filename().string();
            if(name.size()!=19||name.substr(10)!="_Full.bin")continue;
            if(!known.count(std::string(folder)+"/"+std::to_string(std::stoul(name.substr(0,10)))))continue;
            auto empty=e.path().parent_path()/(name.substr(0,10)+"_Empty.bin");
            if(std::filesystem::exists(empty))files.push_back(e.path());
        }
        (void)before;
    }
    std::sort(files.begin(),files.end());
    std::ofstream csv("build/control-full-380.csv");
    csv<<"dataset,id,volume,step,reconstructed,minimum,maximum,angle,dx,dy,dz,overlap,accepted,null_volume,calibrated,calibrated_m3,group,seconds,calibration_note,bed_shift,bed_spread,bed_variants,datum_min,datum_max,error\n";
    std::atomic<size_t> next{0},completed{0};std::atomic<int> failures{0};std::mutex lock;
    std::vector<std::future<void>> workers;
    for(int worker=0;worker<6;worker++)workers.push_back(std::async(std::launch::async,[&]{
        for(;;){
            size_t i=next++;if(i>=files.size())return;
            auto path=files[i];std::string id=path.stem().string().substr(0,10);
            std::ostringstream line;line<<std::setprecision(14)<<path.parent_path().string()<<','<<id;
            try{
                auto start=std::chrono::steady_clock::now();
                auto a=readModel(path),b=readModel(path.parent_path()/(id+"_Empty.bin"));
                if(a.scan!=std::stoul(id)||b.scan!=a.scan||a.label!=b.label||a.kind!=2||b.kind!=1)
                    throw std::runtime_error("Pair metadata mismatch");
                auto alignment=registration::align(a,b,{});
                int progress=-1;
                auto v=calculateVolume(a,b,{},alignment.options,50,true,true,[&](int p){
                    if(p<progress||p<0||p>100)throw std::runtime_error("Invalid progress");progress=p;
                });
                if(progress!=100)throw std::runtime_error("Calculation incomplete");
                CompareOptions zero;zero.aligned=true;
                auto self=calculateVolume(b,b,{},zero,50,true,true);
                if(self.cargo.volume!=0||self.cargo.calibrated)throw std::runtime_error("Nonzero self volume");
                auto& o=v.comparison.options;
                double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
                line<<','<<v.cargo.volume/1e9<<','<<o.step<<','<<v.cargo.reconstructedVolume/1e9
                    <<','<<v.comparison.sensitivityMin/1e9<<','<<v.comparison.sensitivityMax/1e9
                    <<','<<o.angle<<','<<o.dx<<','<<o.dy<<','<<o.dz<<','<<alignment.overlap
                    <<','<<o.aligned<<','<<self.cargo.volume<<','<<v.cargo.calibrated
                    <<','<<v.cargo.calibratedVolume/1e9<<','<<a.selectedType<<','<<seconds
                    <<','<<csvText(v.cargo.calibrationNote)<<','<<o.bedHeightAdjustment<<','<<o.bedHeightSpread<<','<<o.bedHeightVariants<<','<<v.comparison.datumSensitivityMin/1e9<<','<<v.comparison.datumSensitivityMax/1e9<<',';
            }catch(const std::exception& e){++failures;line<<std::string(23,',')<<csvText(e.what());}
            std::lock_guard<std::mutex> guard(lock);
            csv<<line.str()<<'\n';csv.flush();std::cout<<++completed<<"/182 "<<path.string()<<std::endl;
        }
    }));
    for(auto& w:workers)w.get();return csv&&failures==0?0:3;
}
