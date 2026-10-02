#include "../src/volume.hpp"
#include <sstream>
#include <cassert>
#include <iostream>
int main(){
 std::ifstream input("build/ngk-full-341-retest.csv");std::string line;CompareOptions o;double expected=0;
 while(std::getline(input,line)){std::stringstream ss(line);std::vector<std::string> row;std::string v;while(std::getline(ss,v,','))row.push_back(v);if(row[0]!="0000014301")continue;
 o.angle=std::stod(row[6]);o.dx=std::stod(row[7]);o.dy=std::stod(row[8]);o.dz=std::stod(row[9]);o.aligned=true;expected=std::stod(row[1]);}
 assert(expected>0);
 auto a=readModel("n-gk/0000014301_Full.bin"),b=readModel("n-gk/0000014301_Empty.bin");
 o=refineBedHeight(a,b,o);expected=26.76677829647827;
 auto result=calculateVolume(a,b,{},o,50,true,true);assert(result.cargo.calibrated);
 assert(std::abs(result.cargo.volume/1e9-expected)<1e-7);
 assert(std::abs(result.cargo.calibratedVolume/1e9-26.998430431209005)<1e-6);
 auto original=result.cargo;auto c=result.comparison;
 a.scan=b.scan=999999;a.label=b.label="unseen-metadata";
 calibrateVolume(a,b,c,result.cargo,false);assert(result.cargo.calibrated&&std::abs(result.cargo.calibratedVolume-original.calibratedVolume)<1e-4);
 assert(result.cargo.volume==original.volume&&result.cargo.cloud.points.size()==original.cloud.points.size()&&result.cargo.geometry.faces.size()==original.geometry.faces.size());
 calibrateVolume(a,b,c,result.cargo,true);assert(!result.cargo.calibrated);
 c.options.calibratedEstimate=false;calibrateVolume(a,b,c,result.cargo,false);assert(!result.cargo.calibrated);
 c.options.calibratedEstimate=true;c.options.metresPerUnit=1;calibrateVolume(a,b,c,result.cargo,false);assert(!result.cargo.calibrated);
 c.options.metresPerUnit=.001;b.kind=3;calibrateVolume(a,b,c,result.cargo,false);assert(!result.cargo.calibrated);
 // The new dataset must match the independently evaluated Python model too.
 std::ifstream dmu("build/dmu-full-351.csv");CompareOptions d;
 while(std::getline(dmu,line)){std::stringstream ss(line);std::vector<std::string> row;std::string value;while(std::getline(ss,value,','))row.push_back(value);if(row[0]!="0000000008")continue;
 d.angle=std::stod(row[6]);d.dx=std::stod(row[7]);d.dy=std::stod(row[8]);d.dz=std::stod(row[9]);d.aligned=true;}
 auto da=readModel("dmu/0000000008_Full.bin"),db=readModel("dmu/0000000008_Empty.bin");
 d=refineBedHeight(da,db,d);
 auto dm=calculateVolume(da,db,{},d,50,true,true);assert(dm.cargo.calibrated);
 assert(std::abs(dm.cargo.calibratedVolume/1e9-16.390811715401433)<1e-6);
 assert(std::abs(dm.cargo.volume/1e9-16.390368431762695)<1e-7);
 assert(dm.comparison.datumSensitivityChecked&&dm.comparison.datumSensitivityMin<=dm.cargo.volume&&dm.comparison.datumSensitivityMax>=dm.cargo.volume);
 auto repeated=calculateVolume(da,db,{},d,50,true,true);
 assert(repeated.cargo.volume==dm.cargo.volume&&repeated.cargo.calibratedVolume==dm.cargo.calibratedVolume);
 auto expectedDmu=dm.cargo.calibratedVolume;da.scan=db.scan=42;da.label=db.label="different-name";
 calibrateVolume(da,db,dm.comparison,dm.cargo,false);
 assert(dm.cargo.calibrated&&std::abs(dm.cargo.calibratedVolume-expectedDmu)<1e-4);
 auto changed=dm.comparison;changed.options.estimator=3;
 calibrateVolume(da,db,changed,dm.cargo,false);assert(!dm.cargo.calibrated&&dm.cargo.calibratedVolume==0);
 changed=dm.comparison;changed.adaptiveGridUsed=false;
 calibrateVolume(da,db,changed,dm.cargo,false);assert(!dm.cargo.calibrated);
 changed=dm.comparison;changed.refinementRejected=true;
 dm.cargo.calibrated=true;dm.cargo.calibratedVolume=123;
 calibrateVolume(da,db,changed,dm.cargo,false);assert(!dm.cargo.calibrated&&dm.cargo.calibratedVolume==0);
 assert(dm.cargo.calibrationNote.find("Сетка неустойчива")!=std::string::npos);
 // Invalid / out-of-domain feature vectors must fall back without a stale value.
 changed=dm.comparison;dm.cargo.volume=1e9;
 calibrateVolume(da,db,changed,dm.cargo,false);assert(!dm.cargo.calibrated&&dm.cargo.calibratedVolume==0);
 std::cout<<"PASS independent n-gk/dmu predictions, metadata independence, unchanged geometry/points, toggle, ROI, units, domain and reference guards\n";
}

