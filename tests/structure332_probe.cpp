#include "../src/registration.hpp"
#include "../src/volume.hpp"
#include <iostream>
#include <iomanip>
int wmain(int argc,wchar_t**argv){if(argc<3)return 2;
 auto a=readModel(argv[1]),b=readModel(argv[2]);auto old=registration::align(a,b,{});double span=std::max({a.hi.x-a.lo.x,a.hi.y-a.lo.y,b.hi.x-b.lo.x,b.hi.y-b.lo.y});
 registration::SurfaceCloud ac(registration::steepSurfaces(a,span/300)),bc(registration::steepSurfaces(b,span/300));
 auto center=[](const registration::SurfaceCloud& c){std::vector<float> x,y;for(auto n:c.tree.nodes){x.push_back(n.p.x);y.push_back(n.p.y);}std::sort(x.begin(),x.end());std::sort(y.begin(),y.end());return Point{(x[x.size()/50]+x[x.size()*49/50])/2,(y[y.size()/50]+y[y.size()*49/50])/2,0};};
 auto ca=center(ac),cb=center(bc);struct Candidate{CompareOptions o;registration::StructureFit s;};std::array<Candidate,36> candidates;
 parallelJobs(candidates.size(),[&](size_t i){double angle=(i/3)*30.;double rad=angle*3.141592653589793/180,c=std::cos(rad),s=std::sin(rad);auto o=old.options;o.angle=angle;o.dx=ca.x-c*cb.x+s*cb.y+(int(i%3)-1)*span*.12;o.dy=ca.y-s*cb.x-c*cb.y;o.dz=old.options.dz;o=registration::fitStructure(ac,bc,o,span);candidates[i]={o,registration::structuralScore(ac,bc,o,span)};});
 std::sort(candidates.begin(),candidates.end(),[](const Candidate&a,const Candidate&b){return a.s.cost<b.s.cost;});std::cout<<std::setprecision(15);
 for(size_t i=0;i<8;++i){auto z=candidates[i];auto o=z.o;std::cout<<"candidate,"<<i<<",angle,"<<o.angle<<",dx,"<<o.dx<<",dy,"<<o.dy<<",coverage,"<<z.s.coverage<<",cost,"<<z.s.cost<<",spread,"<<z.s.spread<<std::endl;if(i<3){o.dz-=o.rimHeightAdjustment+o.bedHeightAdjustment;o.rimHeightAdjustment=rimHeightCorrection(a,b,o);o.dz+=o.rimHeightAdjustment;o=refineBedHeight(a,b,o);o.aligned=true;auto v=calculateVolume(a,b,{},o,50,true,true);std::cout<<"volume,"<<v.cargo.volume*1e-9<<",final,"<<(v.cargo.calibrated?v.cargo.calibratedVolume:v.cargo.volume)*1e-9<<std::endl;}}
}
