#include "../src/cargo_shape.hpp"
#include <cassert>
#include <iostream>
int main(){
    Comparison c;c.options.step=100;c.region={true,0,800,0,400,-10000,10000};
    c.sharedArea=c.totalArea=320000;c.options.bedHeightSpread=20;
    for(int x=0;x<8;++x)for(int y=0;y<4;++y)
        c.cells[{x,y}]={3000,2000,1000,10000,x==0};
    auto g=buildCargo(c,50,true);auto f=cargoShapeFeatures(c,g);
    assert(std::abs(f[0]-.0032)<1e-12);
    assert(std::abs(f[1]-2400/std::sqrt(320000.))<1e-12);
    assert(f[2]==0&&f[3]==0&&f[4]==0);
    assert(f[5]==.125&&f[6]==1&&f[7]==.02&&f[8]==2&&f[9]==1);
    // Translation cannot identify a scan or change its shape descriptors.
    auto shifted=c;shifted.region.x0+=10000;shifted.region.x1+=10000;
    for(auto& kv:shifted.cells){kv.second.active+=50000;kv.second.base+=50000;}
    auto translated=cargoShapeFeatures(shifted,buildCargo(shifted,50,true));
    for(size_t i=0;i<f.size();++i)assert(std::abs(f[i]-translated[i])<1e-10);
    for(auto&kv:c.cells){kv.second.active+=kv.first.first*100;kv.second.delta+=kv.first.first*100;}
    auto slope=cargoShapeFeatures(c,buildCargo(c,50,true));
    assert(std::abs(slope[2]-std::sqrt(52500.)/1000)<1e-10);
    assert(std::abs(slope[9]-1.35)<1e-10);
    auto empty=cargoShapeFeatures(c,Cargo{});for(double v:empty)assert(v==0);
    std::cout<<"PASS analytic shape, support, dispersion, translation invariance, empty result\n";
}
