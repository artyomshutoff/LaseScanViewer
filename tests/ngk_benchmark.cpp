#include "../src/rim_alignment.hpp"
#include "../src/cargo.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>
// Reuse frozen 3.2.1 rigid poses so the comparison isolates the two changes:
// upper-structure Z refinement and the lower-surface estimator. No labels or
// control volumes are read by this executable.
int main(){try{
 std::ifstream input("build/ngk-baseline.csv");if(!input)throw std::runtime_error("Missing frozen baseline CSV");
 std::ofstream out("build/ngk-final-predictions.csv");out<<std::setprecision(10)<<"id,old,correction,new\n";
 std::string line;std::getline(input,line);size_t count=0;
 while(std::getline(input,line)){
  std::stringstream ss(line);std::vector<std::string> row;std::string s;while(std::getline(ss,s,','))row.push_back(s);
  if(row.size()<13)throw std::runtime_error("Invalid baseline row");
  auto a=readModel(std::filesystem::path("n-gk")/(row[0]+"_Full.bin")),b=readModel(std::filesystem::path("n-gk")/(row[0]+"_Empty.bin"));
  if(a.scan!=std::stoul(row[0])||b.scan!=a.scan||a.kind!=2||b.kind!=1||a.label!=b.label)throw std::runtime_error("BIN pair metadata mismatch");
  CompareOptions o;o.aligned=true;o.angle=std::stod(row[4]);o.dx=std::stod(row[5]);o.dy=std::stod(row[6]);o.dz=std::stod(row[7]);
  double shift=rimHeightCorrection(a,b,o);o.dz+=shift;
  auto c=compareClouds(a,b,{},o);auto cargo=buildCargo(c,50,true,&a,true,&b);
  out<<row[0]<<','<<row[12]<<','<<shift<<','<<cargo.volume/1e9<<'\n';++count;
 }
 if(count!=89)throw std::runtime_error("Expected all 89 paired measurements");
 out.flush();if(!out)throw std::runtime_error("CSV write failed");std::cout<<"PASS 89 matched BIN pairs, geometry-only predictions\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
