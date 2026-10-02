#pragma once
#include "water_fill.hpp"
struct BedDimensions {double length=0,width=0,height=0;bool available=false;};
// The hydraulic interior excludes the cab and unrelated surfaces. The minimum
// area rectangle of its XY hull is independent of the viewing/registration yaw.
// These are sampled interior dimensions, not the exterior vehicle envelope.
inline BedDimensions measureBed(const WaterFill& water){
 BedDimensions out;if(!water.available||water.geometry.points.size()<8)return out;
 using XY=std::pair<double,double>;std::vector<XY> points;std::vector<double> floors;
 for(size_t i=0;i+7<water.geometry.points.size();i+=8){floors.push_back(water.geometry.points[i].z);for(size_t j=0;j<4;++j){auto p=water.geometry.points[i+j];if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))return out;points.emplace_back(p.x,p.y);}}
 std::sort(points.begin(),points.end());points.erase(std::unique(points.begin(),points.end()),points.end());if(points.size()<3)return out;
 auto cross=[](XY a,XY b,XY c){return (b.first-a.first)*(c.second-a.second)-(b.second-a.second)*(c.first-a.first);};
 std::vector<XY> hull;
 for(auto p:points){while(hull.size()>1&&cross(hull[hull.size()-2],hull.back(),p)<=0)hull.pop_back();hull.push_back(p);}
 auto lower=hull.size();for(size_t i=points.size()-1;i-->0;){auto p=points[i];while(hull.size()>lower&&cross(hull[hull.size()-2],hull.back(),p)<=0)hull.pop_back();hull.push_back(p);}hull.pop_back();
 double best=std::numeric_limits<double>::infinity();
 for(size_t i=0;i<hull.size();++i){auto a=hull[i],b=hull[(i+1)%hull.size()];double dx=b.first-a.first,dy=b.second-a.second,n=std::hypot(dx,dy);if(n<=0)continue;dx/=n;dy/=n;
  double xmin=std::numeric_limits<double>::infinity(),ymin=xmin,xmax=-xmin,ymax=-xmin;
  for(auto p:hull){double x=p.first*dx+p.second*dy,y=-p.first*dy+p.second*dx;xmin=std::min(xmin,x);xmax=std::max(xmax,x);ymin=std::min(ymin,y);ymax=std::max(ymax,y);}
  double l=xmax-xmin,w=ymax-ymin,area=l*w;if(area<best){best=area;out.length=std::max(l,w);out.width=std::min(l,w);}
 }
 std::sort(floors.begin(),floors.end());out.height=water.level-floors[size_t((floors.size()-1)*.02)];
 out.available=std::isfinite(out.length)&&std::isfinite(out.width)&&std::isfinite(out.height)&&out.length>0&&out.width>0&&out.height>0;return out;
}
