#include "../src/registration.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>
int main(){
 std::ofstream csv("build/alignment40/production.csv");csv<<"id,angle,dx,dy,dz,overlap,accepted,focused\n";
 for(int id:{43279,43280,43281,43296,43297,43298}){
  auto name=std::string("00000")+std::to_string(id);auto a=readModel(std::filesystem::path(L"тест")/(name+"_Full.bin")),b=readModel(std::filesystem::path(L"тест")/(name+"_Empty.bin"));auto na=a.points.size(),nb=b.points.size();int last=-1;
  auto result=registration::align(a,b,{},[&](int p){assert(p>=last&&p<=100);last=p;});auto o=result.options;
  assert(last==100&&o.registrationFocused&&std::abs(std::abs(o.angle)-180)<3);
  assert(a.points.size()==na&&b.points.size()==nb);
  csv<<std::setprecision(14)<<id<<','<<o.angle<<','<<o.dx<<','<<o.dy<<','<<o.dz<<','<<result.overlap<<','<<o.aligned<<','<<o.registrationFocused<<'\n';csv.flush();std::cout<<id<<" angle "<<o.angle<<std::endl;
 }
 std::ifstream in("build/research38/manifest.csv");std::string line;std::getline(in,line);int tested=0,invalid=0;
 while(std::getline(in,line)){auto comma=line.find(',');auto ds=line.substr(0,comma),id=std::to_string(std::stoul(line.substr(comma+1)));id=std::string(10-id.size(),'0')+id;
  try{auto a=readModel(std::filesystem::path(ds)/(id+"_Full.bin")),b=readModel(std::filesystem::path(ds)/(id+"_Empty.bin"));assert(!useVehicleFocus(a,b,vehicleFocus(a),vehicleFocus(b)));++tested;}catch(const std::runtime_error&){++invalid;}
 }
 assert(tested==181&&invalid==1);std::cout<<"PASS six reversed test pairs, unchanged 181 control routes, progress and immutable scans\n";
}
