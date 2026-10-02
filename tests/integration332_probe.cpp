#include "../src/volume.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <mutex>
std::vector<std::string> split(const std::string&s){std::vector<std::string> v;std::string item;std::istringstream in(s);while(std::getline(in,item,',')){if(item.size()>1&&item.front()=='"')item=item.substr(1,item.size()-2);v.push_back(item);}if(!s.empty()&&s.back()==',')v.emplace_back();return v;}
int wmain(int argc,wchar_t**argv){if(argc<4)return 2;std::map<std::pair<std::string,int>,CompareOptions> poses;std::ifstream f(argv[2]);std::string line;std::getline(f,line);while(std::getline(f,line)){auto r=split(line);if(r.size()<26||!r[25].empty())continue;CompareOptions o;o.angle=std::stod(r[11]);o.dx=std::stod(r[12]);o.dy=std::stod(r[13]);o.dz=std::stod(r[14]);o.aligned=true;poses[{r[0],std::stoi(r[1])}]=o;}
 std::ifstream manifest(argv[1]);std::vector<std::pair<std::string,std::filesystem::path>> paths;while(std::getline(manifest,line)){auto tab=line.find('\t');if(tab!=std::string::npos)paths.push_back({line.substr(0,tab),std::filesystem::u8path(line.substr(tab+1))});}std::ofstream out(argv[3]);out<<std::setprecision(17)<<"dataset,id,original,phase0,phase1,phase2,phase3,mean75,mean100,median75,median100,fine50,coarse125\n";std::mutex lock;
 parallelJobs(paths.size(),[&](size_t i){auto [dataset,path]=paths[i];int id=std::stoi(path.filename().string().substr(0,10));auto found=poses.find({dataset,id});if(found==poses.end())return;auto a=readModel(path),b=readModel(path.parent_path()/(path.filename().string().substr(0,10)+"_Empty.bin"));auto o=found->second;auto original=calculateVolume(a,b,{},o,50,true,true);std::array<double,10> values{};auto region=original.comparison.region;double actual=original.comparison.options.step;
 for(size_t j=0;j<4;++j){auto roi=region;roi.x0-=double(j&1)*actual*.5;roi.y0-=double(j>>1)*actual*.5;auto variant=original.comparison.options;auto c=compareClouds(a,b,roi,variant);values[j]=buildCargo(c,50,true,nullptr,true,nullptr,false).volume;}
 for(size_t j=4;j<10;++j){auto variant=o;variant.estimator=j<6?1:j<8?3:4;variant.step=j==4||j==6?75:j==5||j==7?100:j==8?50:125;auto c=compareClouds(a,b,{},variant);values[j]=buildCargo(c,50,true,nullptr,true,nullptr,false).volume;}
 std::ostringstream row;row<<std::setprecision(17)<<'"'<<dataset<<'"'<<','<<id<<','<<original.cargo.volume*1e-9;for(auto v:values)row<<','<<v*1e-9;std::lock_guard<std::mutex>guard(lock);out<<row.str()<<'\n';out.flush();std::cout<<i+1<<'/'<<paths.size()<<" "<<id<<std::endl;});
}
