#pragma once
#include "registration.hpp"
#include "volume.hpp"
#include "water_preview.hpp"
#include "water_loaded.hpp"
#include "virtual_tarp.hpp"
#include "bed_dimensions.hpp"
#include "control_database.hpp"
#include "view_presets.hpp"
#include <optional>
#include <sstream>
#include <iomanip>
#include <cstring>
namespace web {
inline std::string json(const std::string& text){std::string out="\"";for(unsigned char c:text){if(c=='"'||c=='\\'){out+='\\';out+=char(c);}else if(c<32){const char* hex="0123456789abcdef";out+="\\u00";out+=hex[c>>4];out+=hex[c&15];}else out+=char(c);}return out+'"';}
struct Result {CompareOptions pose;std::optional<VolumeResult> volume;WaterFill water,loaded;VirtualTarp tarp;Model plane;BedDimensions dimensions;};
class State {
public:
 std::optional<Model> full,empty;CompareOptions options;std::optional<Result> result;
 control::Database database;std::future<Result> task;std::atomic<int> progress{0},phase{0};bool busy=false;unsigned revision=0;std::string error;
 void loaded(Model next,bool active){if(busy)throw std::runtime_error("Расчёт выполняется");if(active){if(next.kind!=2)throw std::runtime_error("Выберите Full для скана с грузом");full=std::move(next);}else {if(next.kind==2)throw std::runtime_error("Для базы выберите Empty или Reference");empty=std::move(next);}result.reset();options=CompareOptions{};error.clear();++revision;}
 void start(bool calculate,CompareOptions requested){
  if(busy)throw std::runtime_error("Расчёт уже выполняется");
  if(!full||!empty)throw std::runtime_error("Откройте Full и Empty / Reference");
  if(full->selectedType!=empty->selectedType)throw std::runtime_error("Выберите одинаковый набор координат Full и Empty");
  if(full->label!=empty->label)throw std::runtime_error("Идентификаторы кузова Full и Empty не совпадают");
  if(empty->kind==1&&full->scan!=empty->scan)throw std::runtime_error("Номера измерений Full и Empty не совпадают");
  progress=0;phase=1;error.clear();
  task=std::async(std::launch::async,[this,calculate,requested]{
   Result out;auto fit=registration::align(*full,*empty,requested,[&](int n){progress=calculate?n*60/100:n;});out.pose=fit.options;
   if(calculate){phase=2;out.volume=calculateVolume(*full,*empty,{},fit.options,50,true,true,[&](int n){progress=60+n*35/100;});
    auto& c=out.volume->comparison;out.pose=c.options;phase=3;out.water=waterFill(*empty,c,out.volume->cargo);out.loaded=waterAboveLoad(*full,c,out.water);out.tarp=virtualTarp(out.volume->cargo,out.water,&c);out.dimensions=measureBed(out.water);out.plane=waterPreviewSurface(out.water,c.requestedStep>0?c.requestedStep:c.options.step);
   }progress=100;return out;
  });busy=true;
 }
 void poll(){if(busy&&task.wait_for(std::chrono::seconds(0))==std::future_status::ready){try{auto next=task.get();options=next.pose;result=std::move(next);++revision;}catch(const std::exception& e){error=e.what();}busy=false;phase=0;}}
 std::string metadata(){poll();std::ostringstream o;o<<std::setprecision(17)<<"{\"version\":\"3.35.18\",\"busy\":"<<(busy?"true":"false")<<",\"progress\":"<<progress.load()<<",\"phase\":"<<phase.load()<<",\"revision\":"<<revision<<",\"error\":"<<json(error)<<",\"heading\":"<<(full?view::heading(*full):empty?view::heading(*empty):0);
  auto model=[&](const char* key,const std::optional<Model>& m){o<<",\""<<key<<"\":";if(!m){o<<"null";return;}o<<"{\"scan\":"<<m->scan<<",\"label\":"<<json(m->label)<<",\"kind\":"<<m->kind<<",\"points\":"<<m->points.size()<<",\"group\":"<<m->selectedType<<",\"groups\":"<<m->availableTypes<<",\"lo\":["<<m->lo.x<<','<<m->lo.y<<','<<m->lo.z<<"],\"hi\":["<<m->hi.x<<','<<m->hi.y<<','<<m->hi.z<<"]}";};model("full",full);model("empty",empty);
  o<<",\"databaseRecords\":"<<database.ids.size()<<",\"result\":";
  if(!result)o<<"null";else{auto& r=*result;o<<"{\"angle\":"<<r.pose.angle<<",\"overlap\":"<<r.pose.alignmentOverlap<<",\"aligned\":"<<(r.pose.aligned?"true":"false")<<",\"volume\":";
   if(!r.volume)o<<"null";else{auto& v=*r.volume;double scale=std::pow(v.comparison.options.metresPerUnit,3),value=(v.cargo.calibrated?v.cargo.calibratedVolume:v.cargo.volume)*scale;
    o<<"{\"m3\":"<<value<<",\"metresPerUnit\":"<<v.comparison.options.metresPerUnit<<",\"geometric\":"<<v.cargo.volume*scale<<",\"step\":"<<v.comparison.options.step<<",\"provisional\":"<<(v.comparison.options.provisional?"true":"false")<<",\"warning\":"<<json(v.comparison.warning)<<",\"waterAvailable\":"<<(r.water.available?"true":"false")<<",\"water\":"<<r.water.volume*scale<<",\"aboveLoad\":"<<r.loaded.volume*scale<<",\"aboveRim\":"<<r.tarp.volume*scale<<",\"dimensions\":";
    if(r.dimensions.available)o<<'['<<r.dimensions.length*r.pose.metresPerUnit<<','<<r.dimensions.width*r.pose.metresPerUnit<<','<<r.dimensions.height*r.pose.metresPerUnit<<']';else o<<"null";o<<'}';
   }o<<'}';
  }
  auto match=full?database.find(full->scan,full->label):control::Match{{},"Откройте Full"};o<<",\"control\":";if(match.record)o<<"{\"total\":"<<match.record->total<<",\"truck\":"<<json(match.record->truck)<<'}';else o<<"null";o<<",\"controlReason\":"<<json(match.reason)<<'}';return o.str();
 }
 // Wire format: uint32 vertex count, index count, XYZ/RGB float32, uint32 indices.
 // Uses every native point (no decimation) and the same colour ramp as desktop.
 std::string scene(const std::string& layer,bool mesh){
  const Model* m=nullptr;const Model* ramp=nullptr;bool base=false,gray=false,blue=false;
  if(layer=="full"&&full)m=&*full;else if(layer=="empty"&&empty){m=&*empty;base=true;}
  else if(layer=="context"&&full){m=&*full;gray=true;}
  else if(result&&result->volume){auto& r=*result;auto& c=r.volume->cargo;ramp=&c.geometry;
   if(layer=="cargo")m=mesh?&c.geometry:&c.cloud;
   else if(layer=="water"||layer=="loadedWater"){m=&r.plane;blue=true;}
   else if(layer=="tarp")m=mesh?&r.tarp.geometry:&r.tarp.cloud;
  }
  std::set<std::tuple<float,float,float>> selected;
  if(gray&&result&&result->volume)for(auto p:result->volume->cargo.cloud.points)selected.emplace(p.x,p.y,p.z);
  std::string bytes(8,'\0');uint32_t count=0,indices=0;
  auto append=[&](const void* p,size_t n){bytes.append(static_cast<const char*>(p),n);};
  if(m){for(auto p:m->points){if(gray&&selected.count({p.x,p.y,p.z}))continue;auto col=color(ramp?*ramp:*m,p);if(base){p=transformBase(p,options);col={.1f,.75f,1.f};}if(gray){float g=.32f+.38f*std::clamp((p.z-m->lo.z)/std::max(1.f,m->hi.z-m->lo.z),0.f,1.f);col={g,g,g};}if(blue)col={0,0,1};float vertex[]={p.x,p.y,p.z,col[0],col[1],col[2]};append(vertex,sizeof(vertex));++count;}
   if(mesh&&!gray){for(auto face:m->faces){append(face.data(),sizeof(uint32_t)*3);indices+=3;}}
  }std::memcpy(bytes.data(),&count,4);std::memcpy(bytes.data()+4,&indices,4);return bytes;
 }
};
}
