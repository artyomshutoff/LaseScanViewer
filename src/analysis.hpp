#pragma once
#include "model.hpp"
#include <map>
#include <set>

struct Region {
    bool enabled=false;
    double x0=0,x1=0,y0=0,y1=0,z0=0,z1=0;
    bool contains(Point p)const{return !enabled||(p.x>=x0&&p.x<=x1&&p.y>=y0&&p.y<=y1&&p.z>=z0&&p.z<=z1);}
    void validate()const{if(enabled&&(!std::isfinite(x0)||!std::isfinite(x1)||!std::isfinite(y0)||!std::isfinite(y1)||!std::isfinite(z0)||!std::isfinite(z1)||x1<=x0||y1<=y0||z1<=z0))throw std::runtime_error("Invalid region bounds");}
};
inline Region bounds(const Model& m){return {true,m.lo.x,m.hi.x,m.lo.y,m.hi.y,m.lo.z,m.hi.z};}
inline Model cropped(const Model& input,const Region& region){
    region.validate();if(!region.enabled)return input;
    Model m=input;m.points.clear();m.faces.clear();m.profiles.clear();
    for(const auto& row:input.profiles){std::vector<uint32_t> out;for(auto i:row)if(region.contains(input.points[i])){out.push_back(uint32_t(m.points.size()));m.points.push_back(input.points[i]);}m.profiles.push_back(std::move(out));}
    if(m.points.empty())throw std::runtime_error("The selected region contains no points");
    m.lo=m.hi=m.points.front();for(auto p:m.points){m.lo.x=std::min(m.lo.x,p.x);m.lo.y=std::min(m.lo.y,p.y);m.lo.z=std::min(m.lo.z,p.z);m.hi.x=std::max(m.hi.x,p.x);m.hi.y=std::max(m.hi.y,p.y);m.hi.z=std::max(m.hi.z,p.z);}
    triangulate(m);return m;
}
struct CompareOptions {
    double step=100,metresPerUnit=.001,dx=0,dy=0,dz=0,angle=0;
    int estimator=4; // 0 highest, 1 mean, 2 lowest, 3 median, 4 robust lower surface
    bool aligned=false,provisional=false;
    double alignmentOverlap=-1;
    bool reconstructGaps=true;
    double rimHeightAdjustment=0;
    bool adaptiveGrid=true;
    bool calibratedEstimate=true;
    double bedHeightAdjustment=0,bedHeightSpread=0;
    size_t bedHeightVariants=0;
    bool registrationFocused=false;
};
inline Point transformBase(Point p,const CompareOptions& o){double a=o.angle*3.141592653589793/180,c=std::cos(a),s=std::sin(a);return {float(c*p.x-s*p.y+o.dx),float(s*p.x+c*p.y+o.dy),float(p.z+o.dz)};}
struct PreparedBaseTransform {
    double c,s,dx,dy,dz;
    explicit PreparedBaseTransform(const CompareOptions& o){double a=o.angle*3.141592653589793/180;c=std::cos(a);s=std::sin(a);dx=o.dx;dy=o.dy;dz=o.dz;}
    Point operator()(Point p)const{return {float(c*p.x-s*p.y+dx),float(s*p.x+c*p.y+dy),float(p.z+dz)};}
};
using CellKey=std::pair<int,int>;
// Consecutive scan points often share a cell. Reuse that lookup without an
// extra hash index; the ordered owner and sample/sum order remain unchanged.
template<class Value> struct OrderedCellLookup {
    std::map<CellKey,Value>& grid;CellKey previous{};Value* recent=nullptr;
    explicit OrderedCellLookup(std::map<CellKey,Value>& g,size_t):grid(g){}
    Value& operator()(CellKey key){if(recent&&key==previous)return *recent;previous=key;
        recent=&grid[key];
        return *recent;}
};
struct HeightCell {
    double sum=0,lo=0,hi=0;size_t count=0;std::vector<double> samples;
    void add(double z){if(!count)lo=hi=z;lo=std::min(lo,z);hi=std::max(hi,z);sum+=z;++count;samples.push_back(z);}
    double value(int method){if(method==4){
        if(samples.size()<10)method=3;
        else{double rank=(samples.size()-1)*.1;size_t k=size_t(rank);std::nth_element(samples.begin(),samples.begin()+k,samples.end());
            double lower=samples[k],upper=k+1<samples.size()?*std::min_element(samples.begin()+k+1,samples.end()):lower;
            return lower+(rank-k)*(upper-lower);}
    }if(method==3){auto mid=samples.begin()+samples.size()/2;std::nth_element(samples.begin(),mid,samples.end());double v=*mid;return samples.size()%2?v:(v+*std::max_element(samples.begin(),mid))/2;}return method==1?sum/count:method==2?lo:hi;}
};
struct DifferenceCell {double active=0,base=0,delta=0,area=0;bool reconstructed=false;};
struct Comparison {
    Region region;CompareOptions options;
    double requestedStep=0;bool adaptiveGridUsed=false;
    bool refinementRejected=false;
    bool sensitivityChecked=false;double sensitivityMin=0,sensitivityMax=0;std::string warning;
    bool datumSensitivityChecked=false;double datumSensitivityMin=0,datumSensitivityMax=0;
    std::map<CellKey,DifferenceCell> cells;
    size_t activeCells=0,baseCells=0;double positive=0,negative=0,sharedArea=0,totalArea=0,maxAbs=0;
    double reconstructedArea=0;
    CellKey key(Point p)const{return {int(std::floor((p.x-region.x0)/options.step)),int(std::floor((p.y-region.y0)/options.step))};}
    double coverage()const{return totalArea>0?sharedArea/totalArea:0;}
};
inline Comparison compareClouds(const Model& active,const Model& base,Region roi,CompareOptions options){
    if(active.points.empty()||base.points.empty())throw std::runtime_error("Two nonempty clouds are required");
    if(active.selectedType!=base.selectedType)throw std::runtime_error("Select the same coordinate group in both files");
    if(!options.aligned){
        if(!std::isfinite(options.alignmentOverlap)||options.alignmentOverlap<0)throw std::runtime_error("Run alignment or confirm common coordinates in comparison settings");
        options.provisional=true;
    }
    if(!std::isfinite(options.angle)||!std::isfinite(options.step)||options.step<=0||!std::isfinite(options.dx)||!std::isfinite(options.dy)||!std::isfinite(options.dz)||!std::isfinite(options.metresPerUnit)||options.metresPerUnit<0||options.estimator<0||options.estimator>4)throw std::runtime_error("Invalid comparison settings");
    PreparedBaseTransform prepared(options);
    if(!roi.enabled){Point lo=active.lo,hi=active.hi;for(auto p:base.points){p=prepared(p);lo.x=std::min(lo.x,p.x);lo.y=std::min(lo.y,p.y);hi.x=std::max(hi.x,p.x);hi.y=std::max(hi.y,p.y);}roi={true,lo.x,hi.x,lo.y,hi.y,-std::numeric_limits<double>::max(),std::numeric_limits<double>::max()};}
    roi.validate();double nx=std::ceil((roi.x1-roi.x0)/options.step),ny=std::ceil((roi.y1-roi.y0)/options.step);
    if(nx<1||ny<1||nx>1000000||ny>1000000||nx*ny>1000000)throw std::runtime_error("Grid exceeds 1 million cells: increase cell size or reduce region");
    Comparison result;result.region=roi;result.options=options;result.totalArea=(roi.x1-roi.x0)*(roi.y1-roi.y0);
    if(options.registrationFocused)result.warning="При совмещении исключён окружающий фон; проверьте положение кузова.";
    if(!options.aligned)result.warning+="Совмещение ненадёжно: объём рассчитан предварительно по найденному повороту и сдвигу. Проверьте наложение кузова.";
    auto build=[&](const Model& m,bool shift){std::map<CellKey,HeightCell> grid;
        OrderedCellLookup<HeightCell> lookup(grid,m.points.size());
        for(auto p:m.points){if(shift){p=prepared(p);}if(!roi.contains(p))continue;
            auto k=result.key(p);k.first=std::min(k.first,int(nx)-1);k.second=std::min(k.second,int(ny)-1);lookup(k).add(p.z);}
        return grid;
    };
    auto a=build(active,false),b=build(base,true);result.activeCells=a.size();result.baseCells=b.size();
    std::map<CellKey,double> ah,bh;
    for(auto& [k,v]:a)ah[k]=v.value(options.estimator);
    for(auto& [k,v]:b)bh[k]=v.value(options.estimator);
    // Only mixed-height cells are regularized. A thin but consistently measured
    // rib is preserved; ordinary smooth/sloping surface cells are untouched.
    auto denoise=[&](std::map<CellKey,double>& grid,const std::map<CellKey,HeightCell>& observations){
        auto original=grid;
        for(auto& [k,z]:grid){
            if(observations.at(k).hi-observations.at(k).lo<2*options.step)continue;
            std::vector<double> nearby;
            for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++){
                auto it=original.find({k.first+x,k.second+y});if(it!=original.end())nearby.push_back(it->second);
            }
            if(nearby.size()<7)continue;
            std::sort(nearby.begin(),nearby.end());z=nearby[nearby.size()/2];
        }
    };
    if(options.estimator==4){denoise(ah,a);denoise(bh,b);}
    // Interpolate only short, bracketed gaps from original observations.
    // Two non-collinear opposing pairs must agree; no recursive filling,
    // extrapolation, steep walls or cells missing in BOTH scans are allowed.
    auto repair=[&](const std::map<CellKey,double>& source,const std::map<CellKey,double>& other){
        auto out=source;if(!options.reconstructGaps)return out;
        for(auto [k,unused]:other){(void)unused;if(source.count(k))continue;std::vector<double> estimates;
            for(auto d:std::initializer_list<CellKey>{{1,0},{0,1},{1,1},{1,-1}}){
                int left=0,right=0;double zl=0,zr=0;
                for(int n=1;n<=3;n++){auto it=source.find({k.first-n*d.first,k.second-n*d.second});if(it!=source.end()){left=n;zl=it->second;break;}}
                for(int n=1;n<=3;n++){auto it=source.find({k.first+n*d.first,k.second+n*d.second});if(it!=source.end()){right=n;zr=it->second;break;}}
                if(!left||!right||left+right>4)continue;
                double distance=(left+right)*options.step*std::hypot(double(d.first),double(d.second));
                if(std::abs(zr-zl)>distance*1.5)continue;
                estimates.push_back((zl*right+zr*left)/(left+right));
            }
            if(estimates.size()<2)continue;
            auto limits=std::minmax_element(estimates.begin(),estimates.end());
            if(*limits.second-*limits.first>options.step*.5)continue;
            std::sort(estimates.begin(),estimates.end());size_t n=estimates.size();
            out[k]=n%2?estimates[n/2]:(estimates[n/2-1]+estimates[n/2])/2;
        }
        // Complete enclosed occlusions only. The observed boundary is fixed;
        // harmonic relaxation cannot exceed its height range. No extrapolation
        // into exterior space, recursive growth, or doubly unobserved cells.
        std::set<CellKey> missing;
        for(auto [k,z]:other)if(!out.count(k)){(void)z;missing.insert(k);}
        double span=std::min(roi.x1-roi.x0,roi.y1-roi.y0);
        int maxCells=int(std::min(256.,.04*span*span/(options.step*options.step)));
        while(!missing.empty()){
            std::vector<CellKey> component{*missing.begin()};missing.erase(missing.begin());
            std::set<CellKey> boundary;bool enclosed=true;
            for(size_t i=0;i<component.size();++i)for(auto d:std::initializer_list<CellKey>{{1,0},{-1,0},{0,1},{0,-1}}){
                CellKey k{component[i].first+d.first,component[i].second+d.second};
                auto it=missing.find(k);if(it!=missing.end()){component.push_back(k);missing.erase(it);}
                else if(out.count(k))boundary.insert(k);
                else if(!other.count(k))enclosed=false;
            }
            if(!enclosed||component.size()<2||int(component.size())>maxCells||boundary.size()<8)continue;
            double mean=0,lo=1e30,hi=-1e30;for(auto k:boundary){double z=out.at(k);mean+=z;lo=std::min(lo,z);hi=std::max(hi,z);}
            if(hi-lo>span*.3)continue;
            mean/=boundary.size();std::map<CellKey,double> fill;for(auto k:component)fill[k]=mean;
            for(int iteration=0;iteration<600;iteration++){
                double change=0;
                for(auto k:component){double sum=0;for(auto d:std::initializer_list<CellKey>{{1,0},{-1,0},{0,1},{0,-1}}){CellKey q{k.first+d.first,k.second+d.second};auto it=fill.find(q);sum+=it!=fill.end()?it->second:out.at(q);}double z=sum*.25;change=std::max(change,std::abs(z-fill[k]));fill[k]=z;}
                if(change<options.step*.0001)break;
            }
            out.insert(fill.begin(),fill.end());
        }
        return out;
    };
    auto repairedA=repair(ah,bh),repairedB=repair(bh,ah);
    for(auto& [k,av]:repairedA){auto it=repairedB.find(k);if(it==repairedB.end())continue;
        double area=std::min(options.step,roi.x1-(roi.x0+k.first*options.step))*std::min(options.step,roi.y1-(roi.y0+k.second*options.step));
        double bv=it->second,d=av-bv;bool repaired=!ah.count(k)||!bh.count(k);
        result.cells[k]={av,bv,d,area,repaired};if(repaired)result.reconstructedArea+=area;result.sharedArea+=area;result.positive+=std::max(d,0.)*area;result.negative+=std::max(-d,0.)*area;result.maxAbs=std::max(result.maxAbs,std::abs(d));
    }
    if(result.cells.empty())throw std::runtime_error("No shared grid cells: check region, group, translation and cell size");
    return result;
}
inline std::array<float,3> differenceColor(const Comparison& c,Point p){
    auto k=c.key(p);int nx=int(std::ceil((c.region.x1-c.region.x0)/c.options.step)),ny=int(std::ceil((c.region.y1-c.region.y0)/c.options.step));k.first=std::min(k.first,nx-1);k.second=std::min(k.second,ny-1);
    auto it=c.cells.find(k);if(it==c.cells.end())return {.28f,.28f,.28f};
    float t=float(it->second.delta/std::max(1e-12,c.maxAbs)),a=std::abs(t);
    return t>=0?std::array<float,3>{.25f+.75f*a,.8f*(1-a),.65f*(1-a)}:std::array<float,3>{.25f*(1-a),.8f*(1-a),.65f+.35f*a};
}

