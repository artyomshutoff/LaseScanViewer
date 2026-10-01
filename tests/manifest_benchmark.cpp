#include "../src/registration.hpp"
#include "../src/volume.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <future>
#include <atomic>
#include <mutex>
// A frozen list of files, with no control values. Compatible with old sources
// for a side-by-side test of data not used to develop either model.
static std::string quote(const std::string& s){std::string r="\"";for(char c:s){if(c=='\"')r+='\"';r+=c;}return r+'\"';}
int main(int argc,char** argv){
    if(argc!=3)return 2;
    std::ifstream input(argv[1]);std::string line;std::getline(input,line);
    std::vector<std::pair<std::string,std::string>> files;
    while(std::getline(input,line)){auto comma=line.find(',');std::string id=std::to_string(std::stoul(line.substr(comma+1)));id=std::string(10-id.size(),'0')+id;files.push_back({line.substr(0,comma),id});}
    if(files.empty())return 2;
    std::ofstream csv(argv[2]);csv<<"dataset,id,geometric,displayed,calibrated,overlap,accepted,null_volume,note,error\n";
    std::atomic<size_t> next{0};std::atomic<int> failures{0};std::mutex mutex;std::vector<std::future<void>> workers;
    for(int t=0;t<4;++t)workers.push_back(std::async(std::launch::async,[&]{for(;;){size_t i=next++;if(i>=files.size())return;auto [folder,id]=files[i];std::ostringstream row;row<<std::setprecision(14)<<folder<<','<<id;
        try{
            auto a=readModel(std::filesystem::path(folder)/(id+"_Full.bin")),b=readModel(std::filesystem::path(folder)/(id+"_Empty.bin"));
            if(a.scan!=std::stoul(id)||b.scan!=a.scan||a.kind!=2||b.kind!=1||a.label!=b.label)throw std::runtime_error("Pair metadata mismatch");
            auto alignment=registration::align(a,b,{});auto v=calculateVolume(a,b,{},alignment.options,50,true,true);
            CompareOptions self;self.aligned=true;auto zero=calculateVolume(b,b,{},self,50,true,true);if(zero.cargo.volume!=0)throw std::runtime_error("Nonzero self volume");
            double shown=v.cargo.calibrated?v.cargo.calibratedVolume:v.cargo.volume;
            row<<','<<v.cargo.volume/1e9<<','<<shown/1e9<<','<<v.cargo.calibrated<<','<<alignment.overlap<<','<<alignment.options.aligned<<','<<zero.cargo.volume<<','<<quote(v.cargo.calibrationNote)<<',';
        }catch(const std::exception&e){++failures;row<<std::string(8,',')<<quote(e.what());}
        std::lock_guard<std::mutex> guard(mutex);csv<<row.str()<<'\n';csv.flush();std::cout<<folder<<' '<<id<<std::endl;
    }}));
    for(auto& w:workers)w.get();return failures||!csv?3:0;
}
