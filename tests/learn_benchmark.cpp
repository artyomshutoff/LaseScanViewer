#include "../src/registration.hpp"
#include "../src/cargo.hpp"
#include <iostream>
#include <sstream>
#include <future>
#include <mutex>
#include <atomic>
#include <iomanip>
int wmain(int argc,wchar_t**argv){if(argc<3)return 2;std::filesystem::path dir=argv[1],out=argv[2];std::vector<std::filesystem::path> files;for(auto& e:std::filesystem::directory_iterator(dir))if(e.path().extension()==L".bin")files.push_back(e.path());std::sort(files.begin(),files.end());std::mutex lock;std::atomic<size_t> next{0};std::ofstream csv(out);csv<<"id,split,points,basepoints,angle,dx,dy,dz,overlap,rms,accepted,raw_m3,cargo_m3,negative_m3,coverage,v50,v150,null_volume,error\n";std::vector<std::future<void>> workers;
int workerCount=argc>3?std::clamp(std::stoi(argv[3]),1,12):4;
for(int t=0;t<workerCount;t++)workers.push_back(std::async(std::launch::async,[&]{while(true){size_t i=next++;if(i>=files.size())break;auto f=files[i];auto name=f.filename().wstring();if(name.find(L"_Empty")==std::wstring::npos)continue;auto full=f;name.replace(name.find(L"_Empty"),6,L"_Full");full=f.parent_path()/name;if(!std::filesystem::exists(full))continue;std::ostringstream line;line<<std::fixed<<std::setprecision(6);auto id=f.stem().string().substr(0,10);line<<id<<','<<(std::stoul(id)%5==0?"validation":"development")<<',';
try{auto a=readModel(full),b=readModel(f);auto r=registration::align(a,b,{});auto o=r.options;o.aligned=true;auto c=compareClouds(a,b,{},o);auto cargo=buildCargo(c,50,true,&a,true,&b);auto self=compareClouds(b,b,{},CompareOptions{100,.001,0,0,0,0,0,true});auto null=buildCargo(self,50,true,&b,true,&b);o.step=50;auto small=compareClouds(a,b,{},o);double v50=buildCargo(small,50,true,&a,true,&b).volume/1e9;o.step=150;auto big=compareClouds(a,b,{},o);double v150=buildCargo(big,50,true,&a,true,&b).volume/1e9;line<<a.points.size()<<','<<b.points.size()<<','<<o.angle<<','<<o.dx<<','<<o.dy<<','<<o.dz<<','<<r.overlap<<','<<r.rms<<','<<r.options.aligned<<','<<c.positive/1e9<<','<<cargo.volume/1e9<<','<<c.negative/1e9<<','<<c.coverage()<<','<<v50<<','<<v150<<','<<null.volume<<',';}
catch(const std::exception&e){line<<",,,,,,,,,,,,,,,,,"<<e.what();}std::lock_guard<std::mutex> guard(lock);csv<<line.str()<<'\n';csv.flush();std::cout<<id<<" done\n"<<std::flush;
}}));for(auto& w:workers)w.get();}

