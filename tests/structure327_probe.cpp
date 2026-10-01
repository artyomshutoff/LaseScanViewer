#include "../src/registration.hpp"
#include "../src/volume.hpp"
#include <iostream>
#include <iomanip>
int wmain(int argc,wchar_t** argv){
 if(argc<3)return 2;auto a=readModel(argv[1]),b=readModel(argv[2]);auto alignment=registration::align(a,b,{});auto original=alignment.options;
 double span=std::max({a.hi.x-a.lo.x,a.hi.y-a.lo.y,b.hi.x-b.lo.x,b.hi.y-b.lo.y});
 registration::SurfaceCloud ac(registration::steepSurfaces(a,span/300)),bc(registration::steepSurfaces(b,span/300));
 std::cout<<std::setprecision(12)<<"span "<<span<<" surfaces "<<ac.tree.nodes.size()<<' '<<bc.tree.nodes.size()<<'\n';
 auto score=registration::structuralScore(ac,bc,original,span);std::cout<<"original "<<original.angle<<' '<<original.dx<<' '<<original.dy<<" score "<<score.cost<<' '<<score.coverage<<' '<<score.rms<<' '<<score.spread<<'\n';
 for(double offset:{-.24,-.20,-.16,-.12,-.08,-.04,0.,.04,.08,.12,.16,.20,.24}){
  auto seed=original;bool x=a.hi.x-a.lo.x>a.hi.y-a.lo.y;if(x)seed.dx+=offset*span;else seed.dy+=offset*span;
  auto fitted=registration::fitStructure(ac,bc,seed,span);auto s=registration::structuralScore(ac,bc,fitted,span);
  std::cout<<offset<<" pose "<<fitted.angle<<' '<<fitted.dx<<' '<<fitted.dy<<" score "<<s.cost<<' '<<s.coverage<<' '<<s.rms<<' '<<s.spread;
  if(s.spread&&s.cost<score.cost*.94){auto refined=registration::refine(a,b,fitted,span);auto rs=registration::structuralScore(ac,bc,refined,span);std::cout<<" refined "<<refined.angle<<' '<<refined.dx<<' '<<refined.dy<<' '<<rs.cost<<' '<<rs.coverage;fitted=refined;fitted.dz-=fitted.rimHeightAdjustment+fitted.bedHeightAdjustment;fitted.rimHeightAdjustment=rimHeightCorrection(a,b,fitted);fitted.dz+=fitted.rimHeightAdjustment;fitted=refineBedHeight(a,b,fitted);auto v=calculateVolume(a,b,{},fitted,50,true,true);std::cout<<" vol "<<v.cargo.volume*1e-9<<" final "<<(v.cargo.calibrated?v.cargo.calibratedVolume:v.cargo.volume)*1e-9;}
  std::cout<<std::endl;
 }
}
