#include "../src/volume.hpp"
#include <chrono>
#include <sstream>
#include <iostream>
#include <iomanip>
int wmain(int argc,wchar_t** argv){if(argc!=5)return 2;auto a=readModel(argv[1]),b=readModel(argv[2]);std::ifstream pose{std::filesystem::path(argv[3])};std::string line;std::getline(pose,line);std::getline(pose,line);std::stringstream ss(line);std::vector<double> values;while(std::getline(ss,line,','))values.push_back(std::stod(line));CompareOptions o;o.angle=values[4];o.dx=values[5];o.dy=values[6];o.dz=values[7];o.aligned=values[10];o.alignmentOverlap=values[8];o.bedHeightAdjustment=values[13];o.bedHeightSpread=values[14];o.bedHeightVariants=values[15];
 std::ofstream out{std::filesystem::path(argv[4])};out<<std::setprecision(17)<<"seconds,volume,calibrated,cloud\n";
 for(int i=0;i<5;++i){auto start=std::chrono::steady_clock::now();auto v=calculateVolume(a,b,{},o,50,true,true);out<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<','<<v.cargo.volume<<','<<v.cargo.calibratedVolume<<','<<v.cargo.cloud.points.size()<<'\n';}
}
