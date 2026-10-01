#include "../src/kernel_estimate.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cassert>
#include <iostream>
std::vector<std::string> fields(const std::string& line){std::vector<std::string> result;std::stringstream s(line);std::string v;while(std::getline(s,v,','))result.push_back(v);return result;}
int main(){
 std::ifstream expected("build/research311/kernel-validation.csv");std::string line;std::getline(expected,line);
 std::map<std::string,double> prediction;
 while(std::getline(expected,line)){auto v=fields(line);prediction[v[0]+"/"+v[1]]=std::stod(v[4]);}
 std::ifstream input("build/research311/features.csv");std::getline(input,line);size_t tested=0;
 while(std::getline(input,line)){
  auto row=fields(line);if(row.size()<193||row[2].empty())continue;
  double volume=std::stod(row[2]),correction=0;std::array<double,190> f;
  for(int i=0;i<190;i++)f[i]=std::stod(row[3+i]);
  assert(kernelCorrection(f,volume,correction));
  assert(std::abs(volume*(1+correction)-prediction.at(row[0]+"/"+row[1]))<1e-7);++tested;
  assert(!kernelCorrection(f,-1,correction)&&correction==0);
  f[0]=NAN;assert(!kernelCorrection(f,volume,correction)&&correction==0);
 }
 assert(tested==271);std::cout<<"PASS 271 independent Python/C++ kernel predictions, finite and domain guards\n";
}
