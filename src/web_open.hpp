#pragma once
#include "model.hpp"
#include <cwctype>

namespace webopen {
struct Scan { std::filesystem::path path; Model model; };
// Read both scans before changing the live session so a bad companion cannot
// leave a half-loaded pair. Only the same measurement in the selected folder
// is eligible for automatic pairing.
inline std::vector<Scan> read(const std::filesystem::path& selected,bool includePair) {
 std::vector<Scan> scans;
 auto model=readModel(selected);
 auto kind=model.kind;
 triangulate(model);
 scans.push_back({selected,std::move(model)});
 if(!includePair)return scans;
 auto name=selected.filename().wstring(),lower=name;
 std::transform(lower.begin(),lower.end(),lower.begin(),[](wchar_t c){return wchar_t(towlower(c));});
 const std::wstring suffix=kind==2?L"_full.bin":L"_empty.bin";
 if(lower.size()<suffix.size()||lower.substr(lower.size()-suffix.size())!=suffix)return scans;
 name.replace(name.size()-suffix.size(),suffix.size(),kind==2?L"_Empty.bin":L"_Full.bin");
 auto companion=selected.parent_path()/name;
 if(!std::filesystem::is_regular_file(companion))return scans;
 auto pair=readModel(companion);
 if(pair.scan!=scans.front().model.scan||(kind==2?pair.kind!=1:pair.kind!=2))return scans;
 triangulate(pair);
 scans.push_back({companion,std::move(pair)});
 return scans;
}
}
