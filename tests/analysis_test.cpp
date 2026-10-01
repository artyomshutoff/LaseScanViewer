#include "../src/analysis.hpp"
#include "../src/cargo.hpp"
#include "../src/png.hpp"
#include <iostream>
#include <cassert>
Model plane(float z){Model m;m.selectedType=1;for(int y=0;y<10;y++){m.profiles.emplace_back();for(int x=0;x<10;x++){m.profiles.back().push_back(uint32_t(m.points.size()));m.points.push_back({x+.5f,y+.5f,z});}}m.lo={.5f,.5f,z};m.hi={9.5f,9.5f,z};return m;}
void near(double a,double b){if(std::abs(a-b)>1e-7)throw std::runtime_error("Analytic result mismatch: "+std::to_string(a)+" vs "+std::to_string(b));}
int main(){try{
    auto a=plane(3),b=plane(1);Region roi{true,0,10,0,10,-100,100};CompareOptions o;o.step=1;o.aligned=true;
    auto c=compareClouds(a,b,roi,o);near(c.positive,200);near(c.negative,0);near(c.coverage(),1);assert(c.cells.size()==100);
    auto cargo=buildCargo(c,0,true);near(cargo.volume,200);assert(cargo.cells==100);assert(cargo.geometry.faces.size()==1200);
    double meshVolume=0;for(auto f:cargo.geometry.faces){auto p=cargo.geometry.points[f[0]],q=cargo.geometry.points[f[1]],r=cargo.geometry.points[f[2]];meshVolume+=(double(p.x)*(q.y*r.z-q.z*r.y)+double(p.y)*(q.z*r.x-q.x*r.z)+double(p.z)*(q.x*r.y-q.y*r.x))/6;}near(meshVolume,cargo.volume);
    near(buildCargo(c,2,true).volume,0);
    Comparison isolated;isolated.region=roi;isolated.options=o;isolated.cells[ {0,0} ]={3,1,2,1};isolated.cells[ {5,5} ]={6,1,5,1};isolated.cells[ {8,8} ]={0,1,-1,1};
    near(buildCargo(isolated,0,true).volume,5);near(buildCargo(isolated,0,false).volume,7);near(buildCargo(isolated,3,false).volume,5);
    Comparison rim=c;rim.region.x1=25;for(int x=10;x<22;x++)rim.cells[{x,5}]={3,1,2,1};
    auto separated=buildCargo(rim,.1,false,nullptr,true);assert(separated.removedCells>0);assert(separated.geometry.hi.x<14);assert(separated.volume>=200);
    auto cleanPlane=buildCargo(c,.1,true,nullptr,true);near(cleanPlane.volume,200);
    Comparison speckle; speckle.region=roi;speckle.options=o;speckle.cells[{4,4}]={101,1,100,1};near(buildCargo(speckle,.1,true,nullptr,true).volume,0);
    o.metresPerUnit=.001;c=compareClouds(a,b,roi,o);near(c.positive*std::pow(c.options.metresPerUnit,3),.0000002);o.metresPerUnit=0;
    c=compareClouds(b,a,roi,o);near(c.positive,0);near(c.negative,200);
    auto shifted=b;for(auto& p:shifted.points)p.x+=10;o.dx=-10;c=compareClouds(a,shifted,roi,o);near(c.positive,200);o.dx=0;
    auto reversed=b;for(auto& p:reversed.points){p.x=-p.x;p.y=-p.y;}o.angle=180;c=compareClouds(a,reversed,roi,o);near(c.positive,200);c=compareClouds(a,reversed,{},o);near(c.positive,162);o.angle=0;
    b.points.resize(50);c=compareClouds(a,b,roi,o);near(c.positive,100);near(c.sharedArea,50);near(c.coverage(),.5);
    b=plane(1);roi.x1=9.75;c=compareClouds(a,b,roi,o);near(c.positive,195);near(c.sharedArea,97.5);near(c.coverage(),1);
    roi.x1=10;roi.z0=2;bool rejected=false;try{compareClouds(a,b,roi,o);}catch(...){rejected=true;}assert(rejected);roi.z0=-100;
    o.aligned=false;rejected=false;try{compareClouds(a,b,roi,o);}catch(...){rejected=true;}assert(rejected);o.aligned=true;
    b.selectedType=0;rejected=false;try{compareClouds(a,b,roi,o);}catch(...){rejected=true;}assert(rejected);b.selectedType=1;
    auto sub=cropped(a,{true,0,5,0,10,-100,100});assert(sub.points.size()==50);assert(a.points.size()==100);
    o.step=.00001;rejected=false;try{compareClouds(a,b,roi,o);}catch(...){rejected=true;}assert(rejected);o.step=1;
    a.points.push_back({.5f,.5f,5});o.estimator=0;c=compareClouds(a,b,roi,o);near(c.positive,202);o.estimator=1;c=compareClouds(a,b,roi,o);near(c.positive,201);o.estimator=2;c=compareClouds(a,b,roi,o);near(c.positive,200);
    auto filled=plane(3),empty=plane(1);empty.points.push_back({20,20,10});empty.hi={20,20,10};o.estimator=3;o.step=1;
    auto railCase=compareClouds(filled,empty,roi,o);near(buildCargo(railCase,.1,true,&filled,true,&empty).volume,200);
    for(int repeat=0;repeat<3;repeat++)for(int y=0;y<10;y++)for(int x=0;x<10;x++)filled.points.push_back({x+.5f,y+.5f,3});
    for(int y=0;y<10;y++)for(int x=0;x<10;x++)filled.points.push_back({x+.5f,y+.5f,999});
    near(compareClouds(filled,empty,roi,o).positive,200);o.step=2;near(compareClouds(filled,empty,roi,o).positive,200);
    std::vector<uint8_t> pixels{255,0,0,0,255,0,0,0,255,255,255,255};writeBytes("build/png-test.png",encodePng(2,2,pixels));
    std::cout<<"PASS: analytic volumes, signed differences, translation, missing coverage, partial cells, ROI, group validation, grid limit, 3 estimators, PNG\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
