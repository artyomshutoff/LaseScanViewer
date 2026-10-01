#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

struct Point { float x,y,z; };
struct Model {
    std::vector<Point> points;
    std::vector<std::vector<uint32_t>> profiles;
    std::vector<size_t> profileBreaks;
    std::vector<std::array<uint32_t,3>> faces;
    Point lo{},hi{};
    uint32_t scan=0, kind=0;
    size_t rejected=0;
    size_t auxiliaryProfiles=0;
    uint32_t selectedType=0,availableTypes=0;
    std::string label;
};
struct Reader {
    std::vector<uint8_t> bytes;
    size_t pos=0;
    explicit Reader(const std::filesystem::path& path) {
        std::ifstream f(path,std::ios::binary|std::ios::ate);
        if(!f) throw std::runtime_error("Cannot open file");
        auto len=f.tellg();
        if(len<0 || len>512LL*1024*1024) throw std::runtime_error("File exceeds the 512 MiB limit");
        bytes.resize(static_cast<size_t>(len)); f.seekg(0);
        if(!f.read(reinterpret_cast<char*>(bytes.data()),bytes.size())) throw std::runtime_error("Cannot read file");
    }
    void need(size_t n) const { if(n>bytes.size()-pos) throw std::runtime_error("Truncated BIN record at byte "+std::to_string(pos)); }
    uint32_t u32(){ need(4); uint32_t v=0; for(int k=0;k<4;k++) v|=uint32_t(bytes[pos++])<<(k*8); return v; }
    int32_t i32(){ return static_cast<int32_t>(u32()); }
    std::string string(){need(1);size_t n=bytes[pos++];need(n);std::string s(reinterpret_cast<char*>(bytes.data()+pos),n);pos+=n;return s;}
    void expect(uint32_t v){ if(u32()!=v) throw std::runtime_error("Unsupported LASE layout at byte "+std::to_string(pos-4)); }
};
inline Model readModel(const std::filesystem::path& path,int preferredType=-1) {
    Reader r(path); Model m;
    m.scan=r.u32(); r.expect(0); r.expect(0); r.expect(1);
    m.label=r.string();
    r.expect(0); r.u32(); r.u32();
    uint32_t format=r.u32(),source=r.u32();
    m.kind=r.u32(); if(m.kind<1 || m.kind>3) throw std::runtime_error("Unsupported scan type");
    // Belaz49 Reference uses (0,0); its profile/point layout is unchanged.
    if(!((format==1&&source==2)||(format==0&&source==1)||(format==0&&source==0&&m.kind==3)))throw std::runtime_error("Unsupported LASE header variant");
    uint32_t images=r.u32();
    if(images>(r.bytes.size()-r.pos)/26)throw std::runtime_error("Invalid image record count");
    for(uint32_t i=0;i<images;i++){
        // References only: never open or follow paths stored inside a scan.
        for(int j=0;j<6;j++)r.string();
        r.expect(m.scan);r.expect(1);r.u32();r.expect(1);r.expect(m.kind);
    }
    uint32_t groups=r.u32();
    if(groups==0||groups>64)throw std::runtime_error("Invalid profile group count");
    m.selectedType=preferredType<0?(format==0?1:0):uint32_t(preferredType);
    size_t totalPoints=0;
    for(uint32_t group=0;group<groups;group++){
    uint32_t count=r.u32();
    if(count==0) throw std::runtime_error("Invalid profile count");
    if(count>(r.bytes.size()-r.pos)/164) throw std::runtime_error("BIN is shorter than its declared profile count: incomplete or damaged file");
    uint32_t groupType=UINT32_MAX;
    for(uint32_t index=0;index<count;index++){
        r.expect(1); r.expect(0);uint32_t type=r.u32();r.expect(0); r.u32(); r.u32(); r.expect(6);
        if(type>1||(groupType!=UINT32_MAX&&type!=groupType))throw std::runtime_error("Unsupported or mixed profile coordinate type");
        groupType=type;m.availableTypes|=1u<<type;
        std::vector<uint32_t> profile;
        uint32_t size=r.u32();
        if(size>(r.bytes.size()-r.pos)/52 || totalPoints+size>5000000) throw std::runtime_error("Invalid point count or more than 5 million points");
        totalPoints+=size;
        r.need(size_t(size)*52+132); if(type==m.selectedType)profile.reserve(size);
        for(uint32_t j=0;j<size;j++){
            r.expect(1); r.expect(0); r.u32(); r.expect(0); r.u32(); r.u32(); r.expect(0); r.expect(7);
            int32_t x=r.i32(),y=r.i32(),z=r.i32();
            uint32_t attributes=r.u32();r.u32();if(attributes==3){r.expect(0);r.expect(0);}else if(attributes!=2)throw std::runtime_error("Unsupported point attribute record");
            if(type!=m.selectedType)continue;
            if(x==INT32_MAX || y==INT32_MAX || z==INT32_MAX || x==INT32_MIN || y==INT32_MIN || z==INT32_MIN){++m.rejected;continue;}
            profile.push_back(static_cast<uint32_t>(m.points.size())); m.points.push_back({float(x),float(y),float(z)});
        }
        // Five auxiliary XYZ records are scanner metadata, not measured points.
        for(int k=0;k<5;k++){r.expect(0);r.expect(7);r.i32();r.i32();r.i32();}
        r.expect(0);r.expect(1);r.u32();r.u32();r.i32();r.i32();
        if(r.u32()>1)throw std::runtime_error("Invalid profile status flag");r.expect(0);
        if(type==m.selectedType)m.profiles.push_back(std::move(profile));else ++m.auxiliaryProfiles;
    }
    // Keep independently stored groups disconnected during triangulation.
    if(groupType==m.selectedType&&group+1<groups)m.profileBreaks.push_back(m.profiles.size());
    }
    if(r.pos!=r.bytes.size()) throw std::runtime_error("Unexpected trailing data: unsupported BIN version");
    if(m.points.empty()) throw std::runtime_error("The scan contains no valid XYZ points");
    m.lo=m.hi=m.points.front();
    for(auto p:m.points){m.lo.x=std::min(m.lo.x,p.x);m.lo.y=std::min(m.lo.y,p.y);m.lo.z=std::min(m.lo.z,p.z);m.hi.x=std::max(m.hi.x,p.x);m.hi.y=std::max(m.hi.y,p.y);m.hi.z=std::max(m.hi.z,p.z);}
    return m;
}
inline float dist2(Point a,Point b){float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;}
inline void triangulate(Model& m,float gap=250){
    m.faces.clear();
    // Connect adjacent measured profiles only. Long edges and degenerate faces are omitted.
    auto add=[&](uint32_t a,uint32_t b,uint32_t c){
        Point p=m.points[a],q=m.points[b],r=m.points[c];
        if(std::max({dist2(p,q),dist2(p,r),dist2(q,r)})>gap*gap)return;
        Point u{q.x-p.x,q.y-p.y,q.z-p.z},v{r.x-p.x,r.y-p.y,r.z-p.z};
        Point cross{u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};
        if(dist2(cross,{0,0,0})<0.01f)return;
        m.faces.push_back({a,b,c});
    };
    for(size_t k=1;k<m.profiles.size();k++){
        if(std::find(m.profileBreaks.begin(),m.profileBreaks.end(),k)!=m.profileBreaks.end())continue;
        auto a=m.profiles[k-1],b=m.profiles[k]; if(a.size()<2||b.size()<2)continue;
        auto cmp=[&](uint32_t i,uint32_t j){return m.points[i].x<m.points[j].x;};
        std::stable_sort(a.begin(),a.end(),cmp);std::stable_sort(b.begin(),b.end(),cmp);
        size_t i=0,j=0;
        while(i+1<a.size()||j+1<b.size()){
            if(j+1==b.size() || (i+1<a.size() && dist2(m.points[a[i+1]],m.points[b[j]])<dist2(m.points[a[i]],m.points[b[j+1]]))){add(a[i],b[j],a[i+1]);i++;}
            else {add(a[i],b[j],b[j+1]);j++;}
        }
    }
}
inline std::array<float,3> color(const Model& m,Point p){
    // Full-saturation height ramp: blue -> cyan -> green -> yellow -> red.
    float t=std::clamp((p.z-m.lo.z)/std::max(1.f,m.hi.z-m.lo.z),0.f,1.f);
    float h=4.f*t;
    return {std::clamp(h-2.f,0.f,1.f),std::clamp(std::min(h,4.f-h),0.f,1.f),std::clamp(2.f-h,0.f,1.f)};
}
inline void exportPly(const Model& m,const std::filesystem::path& path,bool mesh){
    std::ofstream f(path,std::ios::binary); if(!f)throw std::runtime_error("Cannot create PLY file");
    f<<"ply\nformat ascii 1.0\ncomment LASE viewer: native coordinate units; inferred open surface\nelement vertex "<<m.points.size()<<"\nproperty float x\nproperty float y\nproperty float z\nproperty uchar red\nproperty uchar green\nproperty uchar blue\nelement face "<<(mesh?m.faces.size():0)<<"\nproperty list uchar uint vertex_indices\nend_header\n";
    for(auto p:m.points){auto c=color(m,p);f<<p.x<<' '<<p.y<<' '<<p.z<<' '<<int(c[0]*255)<<' '<<int(c[1]*255)<<' '<<int(c[2]*255)<<'\n';}
    if(mesh)for(auto t:m.faces)f<<"3 "<<t[0]<<' '<<t[1]<<' '<<t[2]<<'\n';
    f.flush();if(!f)throw std::runtime_error("PLY write failed");
}
