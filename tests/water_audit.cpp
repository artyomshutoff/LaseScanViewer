#include "../src/registration.hpp"
#include "../src/volume.hpp"
#include "../src/water_fill.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <future>
#include <atomic>
#include <mutex>
#include <chrono>

// No reference volumes. Reuse frozen 3.8 alignment (its code is unchanged),
// then run the current application volume pipeline, including rejected poses.
static std::string csvText(const std::string& value){
    std::string out="\"";for(char c:value){if(c=='\"')out+='\"';out+=c;}return out+'\"';
}
std::vector<std::string> split(std::string s){std::vector<std::string> out;std::string v;bool quoted=false;for(size_t i=0;i<s.size();++i){char c=s[i];if(c=='"'){if(quoted&&i+1<s.size()&&s[i+1]=='"'){v+='"';++i;}else quoted=!quoted;}else if(c==','&&!quoted){out.push_back(v);v.clear();}else v+=c;}out.push_back(v);return out;}
int main(){
    std::map<std::string,std::vector<std::string>> poses;std::ifstream poseFile("build/control310/raw.csv");std::string poseLine;std::getline(poseFile,poseLine);while(std::getline(poseFile,poseLine)){auto row=split(poseLine);poses[row[0]+"/"+row[1]]=row;}
    std::vector<std::filesystem::path> files;
    for(auto folder:{L"n-gk",L"dmu",L"тест"}){
        size_t before=files.size();
        for(const auto& e:std::filesystem::directory_iterator(folder)){
            auto name=e.path().filename().string();
            if(name.size()!=19||name.substr(10)!="_Full.bin")continue;
            auto empty=e.path().parent_path()/(name.substr(0,10)+"_Empty.bin");
            if(std::filesystem::exists(empty))files.push_back(e.path());
        }
        (void)before;
    }
    std::sort(files.begin(),files.end());
    std::ofstream csv("build/water312/audit.csv");
    csv<<"dataset,id,volume,water,level,cells,available,error\n";
    std::atomic<size_t> next{0},completed{0};std::atomic<int> failures{0};std::mutex lock;
    std::vector<std::future<void>> workers;
    for(int worker=0;worker<6;worker++)workers.push_back(std::async(std::launch::async,[&]{
        for(;;){
            size_t i=next++;if(i>=files.size())return;
            auto path=files[i];std::string id=path.stem().string().substr(0,10);
            std::ostringstream line;line<<std::setprecision(14)<<path.parent_path().u8string()<<','<<id;
            try{
                auto start=std::chrono::steady_clock::now();
                auto a=readModel(path),b=readModel(path.parent_path()/(id+"_Empty.bin"));
                if(a.scan!=std::stoul(id)||b.scan!=a.scan||a.label!=b.label||a.kind!=2||b.kind!=1)
                    throw std::runtime_error("Pair metadata mismatch");
                auto row=poses.at(path.parent_path().u8string()+"/"+id);registration::Result alignment;auto& saved=alignment.options;saved.step=100;saved.angle=std::stod(row[7]);saved.dx=std::stod(row[8]);saved.dy=std::stod(row[9]);saved.dz=std::stod(row[10]);saved.aligned=row[12]=="1";saved.alignmentOverlap=alignment.overlap=std::stod(row[11]);saved.bedHeightAdjustment=std::stod(row[19]);saved.bedHeightSpread=std::stod(row[20]);saved.bedHeightVariants=std::stoul(row[21]);
                saved.step=std::stod(row[3]);
                auto c=compareClouds(a,b,{},saved);c.requestedStep=100;auto cargo=buildCargo(c,50,true,nullptr,true);
                auto water=waterFill(b,c,cargo);
                line<<','<<cargo.volume/1e9<<','<<water.volume/1e9<<','<<water.level<<','<<water.cells<<','<<water.available<<',';
            }catch(const std::exception& e){++failures;line<<std::string(6,',')<<csvText(e.what());}
            std::lock_guard<std::mutex> guard(lock);
            csv<<line.str()<<'\n';csv.flush();std::cout<<++completed<<"/"<<files.size()<<" "<<path.u8string()<<std::endl;
        }
    }));
    for(auto& w:workers)w.get();return csv&&failures==0?0:3;
}
