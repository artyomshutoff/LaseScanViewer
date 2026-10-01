#include "../src/volume.hpp"
#include <sstream>
#include <iostream>
#include <future>
#include <mutex>
#include <atomic>
#include <iomanip>
int main(){std::ifstream input("build/learn-audit-3.1.csv");std::vector<std::vector<std::string>> rows;std::string line;std::getline(input,line);while(std::getline(input,line)){std::stringstream stream(line);std::vector<std::string> row;std::string s;while(std::getline(stream,s,','))row.push_back(s);rows.push_back(row);}if(rows.size()!=69)return 2;
std::ofstream csv("build/volume-audit-3.2.csv");csv<<"id,accepted,old_largest_m3,new_largest_m3,new_all_m3,reconstructed_m3,raw_m3,removed_cells,cells,min_m3,max_m3,self_m3,error\n";std::mutex mutex;std::atomic<size_t> next{0};std::vector<std::future<void>> tasks;
for(int t=0;t<4;t++)tasks.push_back(std::async(std::launch::async,[&]{while(true){size_t i=next++;if(i>=rows.size())break;auto& row=rows[i];std::ostringstream out;out<<std::fixed<<std::setprecision(6)<<row[0]<<','<<row[10]<<','<<row[12]<<',';
try{auto a=readModel(std::filesystem::path("Learn")/(row[0]+"_Full.bin")),b=readModel(std::filesystem::path("Learn")/(row[0]+"_Empty.bin"));CompareOptions o;o.angle=std::stod(row[4]);o.dx=std::stod(row[5]);o.dy=std::stod(row[6]);o.dz=std::stod(row[7]);o.aligned=true;auto result=calculateVolume(a,b,{},o,50,true,true);auto all=buildCargo(result.comparison,50,false,&a,true,&b);CompareOptions identity;identity.aligned=true;auto zero=buildCargo(compareClouds(b,b,{},identity),50,true,&b,true,&b);out<<result.cargo.volume/1e9<<','<<all.volume/1e9<<','<<result.cargo.reconstructedVolume/1e9<<','<<result.comparison.positive/1e9<<','<<result.cargo.removedCells<<','<<result.cargo.cells<<','<<result.comparison.sensitivityMin/1e9<<','<<result.comparison.sensitivityMax/1e9<<','<<zero.volume/1e9<<',';}
catch(const std::exception& e){out<<",,,,,,,,,"<<e.what();}std::lock_guard<std::mutex> guard(mutex);csv<<out.str()<<'\n';csv.flush();std::cout<<row[0]<<" done\n"<<std::flush;}}));for(auto& task:tasks)task.get();}
