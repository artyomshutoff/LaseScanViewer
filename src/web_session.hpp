#pragma once
#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <stdexcept>
namespace websession {
// Each browser document owns a lease. Reloads and other open tabs keep the
// server alive; the final departure leaves a short reconnect grace period.
class Tabs {
 using Clock=std::chrono::steady_clock;
 struct Lease {Clock::time_point seen;bool hidden;};
 std::map<std::string,Lease> tabs;
 bool attached=false;
 std::optional<Clock::time_point> emptySince;
public:
 using Time=Clock::time_point;
 static void validate(const std::string& id){if(id.size()!=32||id.find_first_not_of("0123456789abcdef")!=std::string::npos)throw std::runtime_error("Invalid browser tab");}
 void touch(const std::string& id,bool hidden,Time now=Clock::now()){validate(id);tabs[id]={now,hidden};attached=true;emptySince.reset();}
 void leave(const std::string& id,Time now=Clock::now()){validate(id);attached=true;tabs.erase(id);if(tabs.empty()&&!emptySince)emptySince=now;}
 bool expired(Time now=Clock::now()){
  for(auto i=tabs.begin();i!=tabs.end();){auto ttl=i->second.hidden?std::chrono::seconds(3600):std::chrono::seconds(120);if(now-i->second.seen>ttl)i=tabs.erase(i);else ++i;}
  if(!attached||!tabs.empty())return false;
  if(!emptySince)emptySince=now;
  return now-*emptySince>=std::chrono::seconds(8);
 }
};
}
