#pragma once
#include "model.hpp"
namespace view {
struct Angles {double yaw=0,pitch=0;};
// Longitudinal axis of the measured object in XY. Trim sparse distant returns
// before PCA, use original Full throughout, and keep the sign deterministic.
inline double heading(const Model& model){
 if(model.points.size()<3)return 0;
 std::vector<double> xs,ys;size_t stride=std::max<size_t>(1,model.points.size()/8000);
 for(size_t i=0;i<model.points.size();i+=stride){auto p=model.points[i];if(std::isfinite(p.x)&&std::isfinite(p.y)){xs.push_back(p.x);ys.push_back(p.y);}}
 if(xs.size()<3)return 0;std::sort(xs.begin(),xs.end());std::sort(ys.begin(),ys.end());size_t edge=(xs.size()-1)/50;
 double mx=0,my=0;size_t count=0;
 auto inside=[&](Point p){return p.x>=xs[edge]&&p.x<=xs[xs.size()-1-edge]&&p.y>=ys[edge]&&p.y<=ys[ys.size()-1-edge];};
 for(size_t i=0;i<model.points.size();i+=stride){auto p=model.points[i];if(inside(p)){mx+=p.x;my+=p.y;++count;}}
 if(count<3)return 0;mx/=count;my/=count;double xx=0,xy=0,yy=0;
 for(size_t i=0;i<model.points.size();i+=stride){auto p=model.points[i];if(inside(p)){double x=p.x-mx,y=p.y-my;xx+=x*x;xy+=x*y;yy+=y*y;}}
 if(std::hypot(xx-yy,2*xy)<1e-9)return 0;
 return .5*std::atan2(2*xy,xx-yy)*180/3.141592653589793;
}
enum class Preset {Top,Front,Side,General};
inline Angles preset(Preset value,double longitudinal){
 if(value==Preset::General)return {165-longitudinal,55};
 if(value==Preset::Top)return {0,0};
 // R_x(-pitch) R_z(yaw): Z is screen-up at +90 degrees pitch.
 // Front looks along length (width horizontal); side shows length horizontal.
 return {value==Preset::Front?90-longitudinal:-longitudinal,90};
}
// Fit the entire frame, including the region outline, for the overview.
inline Model frameCorners(Point lo,Point hi){
 Model out;for(int i=0;i<8;++i)out.points.push_back({i&1?hi.x:lo.x,i&2?hi.y:lo.y,i&4?hi.z:lo.z});return out;
}
struct Fit {double zoom=1,x=0,y=0;};
inline Fit fit(const Model& visible,Angles angles,Point lo,Point hi,double aspect,double fillX=1/1.15,double fillY=1/1.15){
 Fit out;if(visible.points.empty()||aspect<=0)return out;
 double span=std::max({hi.x-lo.x,hi.y-lo.y,hi.z-lo.z,1.f}),k=2/span;
 double a=angles.yaw*3.141592653589793/180,p=-angles.pitch*3.141592653589793/180;
 double c=std::cos(a),s=std::sin(a),cp=std::cos(p),sp=std::sin(p);
 double xmin=1e100,ymin=xmin,xmax=-xmin,ymax=-xmin;
 for(auto point:visible.points){double x=(point.x-(lo.x+hi.x)*.5)*k,y=(point.y-(lo.y+hi.y)*.5)*k,z=(point.z-(lo.z+hi.z)*.5)*k;
  double u=c*x-s*y,v=cp*(s*x+c*y)-sp*z;xmin=std::min(xmin,u);xmax=std::max(xmax,u);ymin=std::min(ymin,v);ymax=std::max(ymax,v);
 }
 double radius=std::hypot(std::hypot(hi.x-lo.x,hi.y-lo.y),hi.z-lo.z)/span;
 double base=std::max(radius,.01)*1.08/std::min(aspect,1.);
 double required=std::max({(xmax-xmin)/(2*aspect*fillX),(ymax-ymin)/(2*fillY),.01});
 out.zoom=std::clamp(base/required,.1,50.);out.x=-(xmin+xmax)/2;out.y=-(ymin+ymax)/2;return out;
}
}
