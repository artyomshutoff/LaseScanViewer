#include "../src/bed_alignment.hpp"
#include <cassert>
#include <iostream>
static void add(Model& m,Point p){m.points.push_back(p);}
static void boundsOf(Model& m){m.lo=m.hi=m.points.front();for(auto p:m.points){
    m.lo.x=std::min(m.lo.x,p.x);m.lo.y=std::min(m.lo.y,p.y);m.lo.z=std::min(m.lo.z,p.z);
    m.hi.x=std::max(m.hi.x,p.x);m.hi.y=std::max(m.hi.y,p.y);m.hi.z=std::max(m.hi.z,p.z);
}}
int main(){
    std::array<BedAnchor,36> anchors{};
    for(size_t i=1;i<18;i+=2)anchors[i]={-30,3,80,1,1,100};
    auto correction=bedHeightCorrection(anchors,10000);
    assert(correction.variants==9&&std::abs(correction.shift+30)<1e-10&&correction.spread<1e-5);
    auto noisy=anchors;
    for(size_t i=1;i<18;i+=2)noisy[i].mad=0;
    for(size_t i=13;i<18;i+=2){noisy[i].shift=30;noisy[i].mad=120;}
    auto weighted=bedHeightCorrection(noisy,10000);
    assert(weighted.variants==9&&std::abs(weighted.shift+20)<1e-9);
    assert(std::abs(weighted.spread-std::sqrt(500.))<1e-9);
    for(size_t i=1;i<18;i+=2)noisy[i].spacing=0;
    assert(bedHeightCorrection(noisy,10000).variants==0);
    for(size_t i=1;i<18;i+=2)anchors[i].coverageY=.1;
    assert(bedHeightCorrection(anchors,10000).variants==0);
    for(size_t i=1;i<18;i+=2){anchors[i].coverageY=1;anchors[i].count=5;}
    assert(bedHeightCorrection(anchors,10000).variants==0);
    for(size_t i=1;i<18;i+=2){anchors[i].count=100;anchors[i].shift=500;}
    assert(bedHeightCorrection(anchors,10000).variants==0);
    Model a,b;a.kind=2;b.kind=1;a.selectedType=b.selectedType=0;
    for(int x=0;x<=10000;x+=25)for(int y=0;y<=2400;y+=25){
        bool rim=x<100||x>9900||y<100||y>2300;
        add(b,{float(x),float(y),rim?1500.f:0.f});
        add(a,{float(x),float(y),rim?1470.f:970.f});
    }
    // A separately moving cab must not pull the bed datum upward.
    for(int x=11000;x<=13000;x+=25)for(int y=0;y<=2000;y+=25){
        add(b,{float(x),float(y),2400});add(a,{float(x),float(y),2600});
    }
    boundsOf(a);boundsOf(b);CompareOptions o;o.aligned=true;
    auto fixed=refineBedHeight(a,b,o);
    assert(fixed.bedHeightVariants>=6&&std::abs(fixed.bedHeightAdjustment+30)<1e-6);
    assert(std::abs(fixed.dz+30)<1e-6&&o.dz==0);
    auto originalA=a.points.size(),originalB=b.points.size();
    b.kind=3;assert(refineBedHeight(a,b,o).bedHeightVariants==0);b.kind=1;
    b.selectedType=1;assert(refineBedHeight(a,b,o).bedHeightVariants==0);
    assert(a.points.size()==originalA&&b.points.size()==originalB);
    std::cout<<"PASS bed datum vs moving cab, distributed anchors, sparse/large-shift guards, reference/group exclusion, immutable input\n";
}
