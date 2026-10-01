#pragma once
#include "analysis.hpp"
// Registration-only foreground proposal. Original measurements remain intact.
inline Model vehicleFocus(const Model&m){
 if(m.points.empty())return m;
 double span=std::max(m.hi.x-m.lo.x,m.hi.y-m.lo.y);if(!std::isfinite(span)||span<=0)return m;
 double step=span/100;std::vector<float> zs;for(auto p:m.points)zs.push_back(p.z);std::sort(zs.begin(),zs.end());double low=zs[size_t((zs.size()-1)*.02)],high=zs[size_t((zs.size()-1)*.98)],cut=low+.2*(high-low);
 std::set<CellKey> occupied,core;
 for(auto p:m.points)if(p.z>=cut)occupied.insert({int(std::floor(p.x/step)),int(std::floor(p.y/step))});
 for(auto k:occupied){int n=0;for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y)n+=occupied.count({k.first+x,k.second+y});if(n>=7)core.insert(k);}
 std::vector<CellKey> best;
 while(!core.empty()){std::vector<CellKey> cc{*core.begin()};core.erase(core.begin());for(size_t i=0;i<cc.size();++i)for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y){auto it=core.find({cc[i].first+x,cc[i].second+y});if(it!=core.end()){cc.push_back(*it);core.erase(it);}}if(cc.size()>best.size())best=std::move(cc);}
 if(best.size()<50)return m;
 int x0=best[0].first,x1=x0,y0=best[0].second,y1=y0;for(auto k:best){x0=std::min(x0,k.first);x1=std::max(x1,k.first);y0=std::min(y0,k.second);y1=std::max(y1,k.second);}
 // Extend along the long axis to retain the adjacent cab, but not side fences.
 bool alongX=x1-x0>y1-y0;double padX=alongX?span*.09:step*2,padY=alongX?step*2:span*.09;
 Region roi{true,x0*step-padX,(x1+1)*step+padX,y0*step-padY,(y1+1)*step+padY,low+(.10*(high-low)),m.hi.z+1};
 if((roi.x1-roi.x0)*(roi.y1-roi.y0)>.82*(m.hi.x-m.lo.x)*(m.hi.y-m.lo.y))return m;
 Model out=m;out.points.clear();out.profiles.clear();out.faces.clear();out.profileBreaks.clear();for(auto p:m.points)if(roi.contains(p))out.points.push_back(p);
 if(out.points.size()<m.points.size()*.35)return m;
 out.lo=out.hi=out.points.front();for(auto p:out.points){out.lo.x=std::min(out.lo.x,p.x);out.lo.y=std::min(out.lo.y,p.y);out.lo.z=std::min(out.lo.z,p.z);out.hi.x=std::max(out.hi.x,p.x);out.hi.y=std::max(out.hi.y,p.y);out.hi.z=std::max(out.hi.z,p.z);}return out;
}



inline bool useVehicleFocus(const Model& a,const Model& b,const Model& fa,const Model& fb){
 if(a.kind!=2||b.kind!=1||a.points.empty()||b.points.empty())return false;
 double ra=double(fa.points.size())/a.points.size(),rb=double(fb.points.size())/b.points.size();
 // Both scans must retain their dominant object. Reject asymmetric proposals
 // caused by an occluded floor, and negligible crops with no useful separation.
 return ra>=.75&&ra<=.95&&rb>=.75&&rb<=.95&&std::abs(ra-rb)<=.15;
}
