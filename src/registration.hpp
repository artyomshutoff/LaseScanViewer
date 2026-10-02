#pragma once
#include "analysis.hpp"
#include "gpu_compute.hpp"
#include "rim_alignment.hpp"
#include "bed_alignment.hpp"
#include "registration_focus.hpp"
#include <tuple>
#include <functional>
#include <future>
#include <atomic>
#include <thread>
#include <chrono>
#include "parallel_work.hpp"

namespace registration {
inline double coord(Point p,int axis){return axis==0?p.x:axis==1?p.y:p.z;}
inline double distance(Point a,Point b){double x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;}
struct Tree {
    struct Node{Point p;int left=-1,right=-1,axis=0;};
    std::vector<Node> nodes;
    mutable std::shared_ptr<compute::TreeBuffers> gpuBuffers;
    int build(std::vector<Point>& p,int lo,int hi,int depth){if(lo>=hi)return -1;int mid=(lo+hi)/2,axis=depth%3;std::nth_element(p.begin()+lo,p.begin()+mid,p.begin()+hi,[axis](Point a,Point b){return coord(a,axis)<coord(b,axis);});int n=int(nodes.size());nodes.push_back({p[mid],-1,-1,axis});int l=build(p,lo,mid,depth+1),r=build(p,mid+1,hi,depth+1);nodes[n].left=l;nodes[n].right=r;return n;}
    explicit Tree(std::vector<Point> p){nodes.reserve(p.size());build(p,0,int(p.size()),0);}
    void nearest(int i,Point p,double& best,Point& q,int* matchIndex=nullptr)const{
        // Balanced tree depth is at most 31 for the int-indexed node array.
        // Deferred far branches retain the recursive traversal and strict tie
        // rule; test their plane only after the near branch has improved best.
        struct Branch{int node;double plane;};std::array<Branch,64> stack;size_t top=0;
        if(nodes.empty())return;
        for(;;){
            while(i>=0){const auto& n=nodes[i];double d=distance(p,n.p);if(d<best||(matchIndex&&*matchIndex<0)){best=d;q=n.p;if(matchIndex)*matchIndex=i;}
                double split=coord(p,n.axis)-coord(n.p,n.axis);int farNode=split<0?n.right:n.left;
                if(farNode>=0)stack[top++]={farNode,split*split};i=split<0?n.left:n.right;
            }
            if(!top)break;auto branch=stack[--top];if(branch.plane<best)i=branch.node;
        }
    }
    int nearestIndex(Point p)const{double d=std::numeric_limits<double>::infinity();Point q{};int index=-1;nearest(0,p,d,q,&index);return index;}
    std::pair<double,Point> nearest(Point p)const{double d=std::numeric_limits<double>::max();Point q{};nearest(0,p,d,q);return {d,q};}
    std::vector<std::pair<double,Point>> nearestBatch(const std::vector<Point>& points)const{std::vector<std::pair<double,Point>> hits;if(compute::nearestGPU(nodes,gpuBuffers,points,hits))return hits;hits.reserve(points.size());for(auto p:points)hits.push_back(nearest(p));return hits;}
};
inline std::vector<Point> sample(const Model& m,double step,size_t maximum=5000){
    struct Average{double x=0,y=0,z=0;size_t n=0;};std::map<std::tuple<int,int,int>,Average> cells;for(auto p:m.points){auto& v=cells[std::make_tuple(int(std::floor(p.x/step)),int(std::floor(p.y/step)),int(std::floor(p.z/step)))];v.x+=p.x;v.y+=p.y;v.z+=p.z;++v.n;}
    std::vector<Point> out;size_t stride=std::max<size_t>(1,(cells.size()+maximum-1)/maximum),i=0;for(auto& v:cells)if(i++%stride==0)out.push_back({float(v.second.x/v.second.n),float(v.second.y/v.second.n),float(v.second.z/v.second.n)});return out;
}
inline void trimGround(std::vector<Point>& points){
    if(points.empty())return;std::vector<float> z;for(auto p:points)z.push_back(p.z);std::sort(z.begin(),z.end());double low=z[size_t((z.size()-1)*.02)],high=z[size_t((z.size()-1)*.98)],cut=low+(high-low)*.2;
    points.erase(std::remove_if(points.begin(),points.end(),[cut](Point p){return p.z<cut;}),points.end());
}
// Fine registration uses reciprocal correspondences and a robust residual cutoff.
// Changed material has no close, mutually supported counterpart in the empty scan.
struct RefineLevel {
    double voxel;std::vector<Point> a,b;Tree at,bt;
    static std::vector<Point> prepared(const Model& m,double step){auto p=sample(m,step);trimGround(p);return p;}
    RefineLevel(const Model& active,const Model& base,double v):voxel(v),a(prepared(active,v)),b(prepared(base,v)),at(a),bt(b){}
};
inline std::vector<RefineLevel> prepareRefinement(const Model& a,const Model& b,double span){
    std::vector<RefineLevel> levels;levels.reserve(3);for(double divisor:{100.,180.,280.})levels.emplace_back(a,b,span/divisor);return levels;
}
inline CompareOptions refinePrepared(const std::vector<RefineLevel>& levels,CompareOptions o){
    for(const auto& level:levels){
        double voxel=level.voxel;const auto& a=level.a;const auto& b=level.b;const auto& at=level.at;const auto& bt=level.bt;
        if(a.size()<100||b.size()<100)continue;
        struct Pair{Point p,q;double d;};std::vector<Pair> pairs;std::vector<double> residuals,weights,medianBuffer;
        pairs.reserve(b.size());residuals.reserve(b.size());weights.reserve(b.size());medianBuffer.reserve(b.size());
        auto median=[&](const std::vector<double>& v){medianBuffer.assign(v.begin(),v.end());auto mid=medianBuffer.begin()+medianBuffer.size()/2;std::nth_element(medianBuffer.begin(),mid,medianBuffer.end());return *mid;};
        for(int iteration=0;iteration<35;iteration++){
            pairs.clear();residuals.clear();weights.clear();
            double rad=o.angle*3.141592653589793/180,cs=std::cos(rad),sn=std::sin(rad);
            PreparedBaseTransform transform(o);
            for(auto original:b){Point p=transform(original);auto hit=at.nearest(p);if(hit.first>std::pow(voxel*3,2))continue;Point q=hit.second;
                Point inverse{float(cs*(q.x-o.dx)+sn*(q.y-o.dy)),float(-sn*(q.x-o.dx)+cs*(q.y-o.dy)),float(q.z-o.dz)};
                if(distance(bt.nearest(inverse).second,original)>voxel*voxel*2.25)continue;
                double d=std::sqrt(hit.first);pairs.push_back({p,q,d});residuals.push_back(d);
            }
            if(pairs.size()<80)break;
            double med=median(residuals);for(auto& v:residuals)v=std::abs(v-med);double cutoff=std::min(voxel*3,std::max(voxel*.6,med+2.5*1.4826*median(residuals)));
            double sum=0,px=0,py=0,pz=0,qx=0,qy=0,qz=0;
            for(auto m:pairs){double u=m.d/cutoff,w=u<1?std::pow(1-u*u,2):0;weights.push_back(w);sum+=w;px+=w*m.p.x;py+=w*m.p.y;pz+=w*m.p.z;qx+=w*m.q.x;qy+=w*m.q.y;qz+=w*m.q.z;}if(sum<30)break;px/=sum;py/=sum;pz/=sum;qx/=sum;qy/=sum;qz/=sum;
            double cross=0,dot=0;for(size_t i=0;i<pairs.size();i++){auto m=pairs[i];cross+=weights[i]*((m.p.x-px)*(m.q.y-qy)-(m.p.y-py)*(m.q.x-qx));dot+=weights[i]*((m.p.x-px)*(m.q.x-qx)+(m.p.y-py)*(m.q.y-qy));}
            double da=std::clamp(std::atan2(cross,dot),-.02,.02),c=std::cos(da),t=std::sin(da),dx=qx-c*px+t*py,dy=qy-t*px-c*py,dz=qz-pz,oldx=o.dx;
            o.dx=c*o.dx-t*o.dy+dx;o.dy=t*oldx+c*o.dy+dy;o.dz+=dz;o.angle+=da*180/3.141592653589793;
            if(std::abs(da)<1e-7&&std::sqrt(dx*dx+dy*dy+dz*dz)<voxel*.001)break;
        }
    }o.angle=std::remainder(o.angle,360.);return o;
}
inline CompareOptions refine(const Model& active,const Model& base,CompareOptions o,double span){return refinePrepared(prepareRefinement(active,base,span),o);}
#include "structural_alignment.hpp"
struct Result{CompareOptions options;double rms=0,overlap=0,score=0;bool reliable=false,structureRefined=false;};
// Gravity-preserving rigid registration: yaw + XYZ, no scale or reflections.
// Multiple yaw seeds avoid a local ICP minimum when the truck is reversed.
inline Result alignClouds(const Model& active,const Model& base,CompareOptions options,const std::function<void(int)>& progress={}){
    if(active.selectedType!=base.selectedType)throw std::runtime_error("Auto alignment requires matching coordinate groups");
    double span=std::max({active.hi.x-active.lo.x,active.hi.y-active.lo.y,base.hi.x-base.lo.x,base.hi.y-base.lo.y});
    if(span<=0||std::min(active.hi.y-active.lo.y,base.hi.y-base.lo.y)<span*.02||std::min(active.hi.z-active.lo.z,base.hi.z-base.lo.z)<span*.005)throw std::runtime_error("Degenerate registration cloud");
    auto a=sample(active,span/100,2500),b=sample(base,span/100,2500);auto allA=a,allB=b;
    // Exclude the lower ground band: the stationary road is not part of the truck.
    trimGround(a);trimGround(b);
    if(a.size()<100||b.size()<100)throw std::runtime_error("Too few spatial points for auto alignment");
    auto centerOf=[](const std::vector<Point>& points){Point lo=points.front(),hi=lo;for(auto p:points){lo.x=std::min(lo.x,p.x);lo.y=std::min(lo.y,p.y);lo.z=std::min(lo.z,p.z);hi.x=std::max(hi.x,p.x);hi.y=std::max(hi.y,p.y);hi.z=std::max(hi.z,p.z);}std::vector<float> xs,ys;for(auto p:points){xs.push_back(p.x);ys.push_back(p.y);}std::sort(xs.begin(),xs.end());std::sort(ys.begin(),ys.end());size_t l=size_t(points.size()*.02),h=size_t((points.size()-1)*.98);return Point{(xs[l]+xs[h])/2,(ys[l]+ys[h])/2,lo.z};};
    Tree tree(a);Point ac=centerOf(allA);
    // For true Empty/Full pairs material can add height, not remove the bed.
    // Use this as a bounded tie-breaker; never reward greater positive volume.
    double physicalStep=span/40;std::map<CellKey,HeightCell> fullGrid;
    if(active.kind==2&&base.kind==1)for(auto p:allA)fullGrid[{int(std::floor(p.x/physicalStep)),int(std::floor(p.y/physicalStep))}].add(p.z);
    std::map<CellKey,double> fullHeights;for(auto& [k,v]:fullGrid)fullHeights[k]=v.value(3);
    auto physicalPenalty=[&](const CompareOptions& o){if(fullHeights.empty())return 0.;std::map<CellKey,HeightCell> emptyGrid;PreparedBaseTransform transform(o);for(auto p:allB){p=transform(p);emptyGrid[{int(std::floor(p.x/physicalStep)),int(std::floor(p.y/physicalStep))}].add(p.z);}double loss=0;size_t shared=0;for(auto& [k,v]:emptyGrid){auto it=fullHeights.find(k);if(it==fullHeights.end())continue;++shared;double d=std::clamp(v.value(3)-it->second-span*.015,0.,span*.05);loss+=d*d;}return shared?loss/shared:span*span*.0025;};
    Result best;best.score=std::numeric_limits<double>::max();
    struct Match{double d;Point p,q;};
    std::vector<Result> candidates;
    auto evaluate=[&](CompareOptions o){
        double limit=span*.015,sum=0,sq=0;size_t inliers=0;
        PreparedBaseTransform transform(o);
        for(auto p:b){double d=tree.nearest(transform(p)).first;sum+=std::min(d,limit*limit);if(d<limit*limit){sq+=d;++inliers;}}
        o.angle=std::remainder(o.angle,360.);
        return Result{o,inliers?std::sqrt(sq/inliers):limit,double(inliers)/b.size(),sum/b.size()+physicalPenalty(o),false};
    };
    auto upper=[](const std::vector<Point>& points){std::vector<float> z;for(auto p:points)z.push_back(p.z);auto it=z.begin()+size_t((z.size()-1)*.95);std::nth_element(z.begin(),it,z.end());return double(*it);};double upperShift=upper(allA)-upper(allB);
    auto upperBand=[&](const std::vector<Point>& cloud){double cut=upper(cloud)-span*.04;std::vector<Point> out;for(auto p:cloud)if(p.z>=cut)out.push_back(p);return out;};
    auto rimA=upperBand(allA),rimB=upperBand(allB);
    if(rimA.size()<100||rimB.size()<100){rimA=a;rimB=b;}
    Tree rimTree(rimA);
    double refinementCenter=0,refinementZ=0;
    // Four whole-cloud searches (two overlap fractions and two height seeds),
    // followed by two independent upper-band searches. Do not force occluded
    // bed points to match the new material surface.
    auto solveSeed=[&](int seed){
        std::vector<Match> matches;matches.reserve(b.size());
        bool rim=seed>=96&&seed<144;const auto& source=rim?rimB:b;const auto& target=rim?rimTree:tree;
        auto o=options;o.angle=seed<144?(seed%24)*15.:refinementCenter+(seed-149)*2.;o.dx=o.dy=o.dz=0;std::vector<Point> rotated;rotated.reserve(allB.size());PreparedBaseTransform rotation(o);for(auto p:(rim?rimB:allB))rotated.push_back(rotation(p));auto center=centerOf(rotated);auto targetCenter=rim?centerOf(rimA):ac;o.dx=targetCenter.x-center.x;o.dy=targetCenter.y-center.y;o.dz=seed<96&&seed%48<24?ac.z-center.z:seed<144?upperShift:refinementZ;
        for(int iter=0;iter<100;iter++){
            matches.clear();PreparedBaseTransform transform(o);
            if(compute::selected.load()==compute::Backend::GPU){std::vector<Point> queries;queries.reserve(source.size());for(auto p:source)queries.push_back(transform(p));auto hits=target.nearestBatch(queries);for(size_t i=0;i<queries.size();i++)matches.push_back({hits[i].first,queries[i],hits[i].second});}
            else for(auto p:source){auto t=transform(p);auto nearestPoint=target.nearest(t);matches.push_back({nearestPoint.first,t,nearestPoint.second});}
            size_t keep=matches.size()*(seed<48||seed>=120?.60:.30);std::nth_element(matches.begin(),matches.begin()+keep,matches.end(),[](const Match& x,const Match& y){return x.d<y.d;});
            double px=0,py=0,pz=0,qx=0,qy=0,qz=0;for(size_t i=0;i<keep;i++){auto& m=matches[i];px+=m.p.x;py+=m.p.y;pz+=m.p.z;qx+=m.q.x;qy+=m.q.y;qz+=m.q.z;}px/=keep;py/=keep;pz/=keep;qx/=keep;qy/=keep;qz/=keep;
            double cross=0,dot=0;for(size_t i=0;i<keep;i++){auto& m=matches[i];double x=m.p.x-px,y=m.p.y-py,u=m.q.x-qx,v=m.q.y-qy;cross+=x*v-y*u;dot+=x*u+y*v;}double angle=std::atan2(cross,dot),c=std::cos(angle),s=std::sin(angle),dx=qx-c*px+s*py,dy=qy-s*px-c*py,dz=qz-pz;
            double oldx=o.dx;o.dx=c*o.dx-s*o.dy+dx;o.dy=s*oldx+c*o.dy+dy;o.dz+=dz;o.angle+=angle*180/3.141592653589793;
            if(std::abs(angle)<1e-6&&std::sqrt(dx*dx+dy*dy+dz*dz)<span*1e-6)break;
        }
        return evaluate(o);
    };
    // The first 144 hypotheses are independent. Collect by seed index so the
    // winner, ties and later refinement retain the serial evaluation order.
    std::array<Result,144> coarse;std::atomic<int> nextSeed{0},completed{0};
    unsigned workers=std::min(4u,std::max(1u,std::thread::hardware_concurrency()));
#ifdef REGISTRATION_SERIAL
    workers=1;
#endif
    if(workers==1){for(int seed=0;seed<144;seed++){coarse[seed]=solveSeed(seed);if(progress)progress(seed*85/155);}}
    else{
        std::vector<std::future<void>> tasks;
        for(unsigned worker=0;worker<workers;worker++)tasks.push_back(std::async(std::launch::async,[&]{for(;;){int seed=nextSeed.fetch_add(1);if(seed>=144)return;coarse[seed]=solveSeed(seed);completed.fetch_add(1);}}));
        for(auto& task:tasks){while(task.wait_for(std::chrono::milliseconds(30))!=std::future_status::ready){if(progress)progress(completed.load()*85/155);}task.get();}
    }
    for(auto candidate:coarse){candidates.push_back(candidate);if(candidate.score<best.score)best=candidate;}
    refinementCenter=best.options.angle;refinementZ=best.options.dz;
    for(int seed=144;seed<155;seed++){if(progress)progress(seed*85/155);auto candidate=solveSeed(seed);candidates.push_back(candidate);if(candidate.score<best.score)best=candidate;}

    // Refine distinct competing basins, not just the first coarse winner.
    // Translation diversity matters on long, almost parallel bed walls.
    std::sort(candidates.begin(),candidates.end(),[](const Result& x,const Result& y){return x.score<y.score;});
    std::vector<Result> finalists;
    for(auto candidate:candidates){
        bool duplicate=false;
        for(auto f:finalists)if(std::abs(std::remainder(candidate.options.angle-f.options.angle,360.))<12&&
            std::hypot(candidate.options.dx-f.options.dx,candidate.options.dy-f.options.dy)<span*.04&&
            std::abs(candidate.options.dz-f.options.dz)<span*.015){duplicate=true;break;}
        if(!duplicate)finalists.push_back(candidate);
        if(finalists.size()==8)break;
    }
    auto refinementLevels=prepareRefinement(active,base,span);
    if(progress)progress(85);
    std::vector<Result> refinedFinalists(finalists.size());
    // Each candidate reads the same prepared trees and owns its result. Merge
    // in the original order to preserve tie-breaking and ambiguity checks.
    parallelJobs(finalists.size(),[&](size_t i){refinedFinalists[i]=evaluate(refinePrepared(refinementLevels,finalists[i].options));});
    for(size_t i=0;i<finalists.size();++i){
        if(progress)progress(85+int(i*14/finalists.size()));
        auto refined=refinedFinalists[i];
        if(refined.score<finalists[i].score)finalists[i]=refined;
        if(finalists[i].score<best.score)best=finalists[i];
        candidates.push_back(finalists[i]);
    }
    bool ambiguous=false;
    for(auto candidate:candidates)if(std::abs(std::remainder(candidate.options.angle-best.options.angle,360.))>30&&candidate.score<best.score*1.1)ambiguous=true;
    if(active.kind==2&&base.kind==1){
        auto adjusted=best.options;adjusted.rimHeightAdjustment=rimHeightCorrection(active,base,adjusted);
        adjusted.dz+=adjusted.rimHeightAdjustment;
        adjusted=refineBedHeight(active,base,adjusted);best=evaluate(adjusted);
    }
    if(progress)progress(98);
    if(active.kind==2&&base.kind==1&&best.overlap<.55){
        if(auto rescued=rescueStructure(active,base,best.options,span)){auto adjusted=*rescued;adjusted.dz-=adjusted.rimHeightAdjustment+adjusted.bedHeightAdjustment;adjusted.rimHeightAdjustment=rimHeightCorrection(active,base,adjusted);adjusted.dz+=adjusted.rimHeightAdjustment;adjusted=refineBedHeight(active,base,adjusted);best=evaluate(adjusted);best.structureRefined=true;}
    }
    double cap=span*.015;
    // A loaded bed can hide most of the empty surface. Require anchors spread
    // over both dimensions rather than treating all occlusion as misalignment.
    Point lo{1e30f,1e30f,0},hi{-1e30f,-1e30f,0},allLo=lo,allHi=hi;size_t anchors=0;
    for(auto p:b){allLo.x=std::min(allLo.x,p.x);allLo.y=std::min(allLo.y,p.y);allHi.x=std::max(allHi.x,p.x);allHi.y=std::max(allHi.y,p.y);if(tree.nearest(transformBase(p,best.options)).first<cap*cap){++anchors;lo.x=std::min(lo.x,p.x);lo.y=std::min(lo.y,p.y);hi.x=std::max(hi.x,p.x);hi.y=std::max(hi.y,p.y);}}
    bool distributed=anchors>=100&&(hi.x-lo.x)>.75*(allHi.x-allLo.x)&&(hi.y-lo.y)>.70*(allHi.y-allLo.y);
    best.reliable=!ambiguous&&best.overlap>=.55&&best.rms<span*.01;
    best.options.aligned=!ambiguous&&(best.overlap>=.45||(best.overlap>=.25&&distributed))&&best.rms<span*.01;
    best.options.provisional=best.options.aligned&&!best.reliable;
    best.options.alignmentOverlap=best.overlap;
    if(progress)progress(100);
    return best;
}
// Isolate a mutually supported vehicle footprint before searching yaw. A fixed
// scanner-side wall can otherwise win the objective while the truck is reversed.
inline Result align(const Model& active,const Model& base,CompareOptions options,const std::function<void(int)>& progress={}){
 options.registrationFocused=false;
 if(active.kind==2&&base.kind==1){
  auto a=vehicleFocus(active),b=vehicleFocus(base);
  if(useVehicleFocus(active,base,a,b)){
   options.registrationFocused=true;
   return alignClouds(a,b,options,progress);
  }
 }
 return alignClouds(active,base,options,progress);
}

}




