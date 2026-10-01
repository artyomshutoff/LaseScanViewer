#include "../src/volume.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>
int main(){std::ifstream in("build/ngk-full-340.csv");std::ofstream out("build/body-regression.csv");out<<"id,volume,old_volume,cargo_points,excluded_body_points\n";std::string line;std::getline(in,line);size_t count=0;
while(std::getline(in,line)){std::stringstream stream(line);std::vector<std::string> row;std::string s;while(std::getline(stream,s,','))row.push_back(s);
auto a=readModel(std::filesystem::path("n-gk")/(row[0]+"_Full.bin")),b=readModel(std::filesystem::path("n-gk")/(row[0]+"_Empty.bin"));CompareOptions o;o.aligned=true;o.step=std::stod(row[2]);o.angle=std::stod(row[6]);o.dx=std::stod(row[7]);o.dy=std::stod(row[8]);o.dz=std::stod(row[9]);auto grid=compareClouds(a,b,{},o);auto cargo=buildCargo(grid,50,true,&a,true,&b);
double v=cargo.volume/1e9;if(std::abs(v-std::stod(row[1]))>1e-5)throw std::runtime_error("Changed volume "+row[0]);if(cargo.cloud.points.empty())throw std::runtime_error("Empty cargo cloud "+row[0]);
out<<std::setprecision(12)<<row[0]<<','<<v<<','<<row[1]<<','<<cargo.cloud.points.size()<<','<<cargo.bodyPointsExcluded<<'\n';out.flush();++count;
}if(count!=89)return 2;std::cout<<"PASS 89 pairs: unchanged volumes, nonempty classified cargo\n";}
