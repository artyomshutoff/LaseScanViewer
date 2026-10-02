#pragma once
#include "manual_volume.hpp"

namespace web {
// Form fields remain decimal strings. The desktop module owns validation,
// conversion and arithmetic; the browser only formats its results.
inline std::string manualDecode(const std::string& text){
 std::string out;auto hex=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;};
 for(size_t i=0;i<text.size();++i){if(text[i]=='+')out+=' ';else if(text[i]=='%'){if(i+2>=text.size()||hex(text[i+1])<0||hex(text[i+2])<0)throw std::runtime_error("Invalid form encoding");out+=char(hex(text[i+1])*16+hex(text[i+2]));i+=2;}else out+=text[i];}return out;
}
inline std::string manualResponse(const std::string& body){
 if(body.size()>150000)throw std::runtime_error("Too many measurements");
 std::map<std::string,std::string> fields;std::istringstream input(body);std::string part;
 while(std::getline(input,part,'&')){auto eq=part.find('=');if(eq==std::string::npos||!fields.emplace(manualDecode(part.substr(0,eq)),manualDecode(part.substr(eq+1))).second)throw std::runtime_error("Invalid measurement form");}
 auto countText=fields["count"];if(countText.empty()||countText.find_first_not_of("0123456789")!=std::string::npos||countText.size()>3)throw std::runtime_error("Invalid row count");
 int count=std::stoi(countText);if(count<1||count>200)throw std::runtime_error("Use 1 to 200 measurements");
 auto unit=fields["unit"],target=fields["to"];if(unit!="m"&&unit!="cm")throw std::runtime_error("Select metres or centimetres");if(!target.empty()&&target!="m"&&target!="cm")throw std::runtime_error("Invalid target unit");
 std::vector<manualVolume::Row> rows(count);
 for(int i=0;i<count;++i)for(int j=0;j<3;++j){auto key="r"+std::to_string(i)+"c"+std::to_string(j);auto it=fields.find(key);if(it==fields.end()||it->second.size()>64)throw std::runtime_error("Invalid measurement field");auto& text=it->second;int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0);if(!text.empty()&&!n)throw std::runtime_error("Invalid text encoding");rows[i][j].resize(n);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),rows[i][j].data(),n);}
 if(!target.empty()&&target!=unit){for(auto& row:rows)for(auto& text:row)if(!manualVolume::blank(text)){auto converted=manualVolume::convertDimension(text,target=="cm");if(!converted)throw std::runtime_error("Исправьте некорректный размер перед сменой единиц.");text=*converted;}unit=target;}
 double scale=unit=="cm"?.01:1;auto result=manualVolume::calculate(rows,scale);bool valid=result.error.empty()&&result.count;
 std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(17)<<"{\"unit\":"<<json(unit)<<",\"count\":"<<result.count<<",\"invalidRow\":"<<result.invalidRow<<",\"valid\":"<<(valid?"true":"false")<<",\"mean\":";
 if(valid)out<<result.mean;else out<<"null";out<<",\"minimum\":";if(valid)out<<result.minimum;else out<<"null";out<<",\"maximum\":";if(valid)out<<result.maximum;else out<<"null";
 out<<",\"dimensions\":";if(valid)out<<'['<<result.dimensions[0]<<','<<result.dimensions[1]<<','<<result.dimensions[2]<<']';else out<<"null";
 out<<",\"rows\":[";for(size_t i=0;i<rows.size();++i){if(i)out<<',';out<<'[';for(int j=0;j<3;++j){if(j)out<<',';const auto& text=rows[i][j];int n=WideCharToMultiByte(CP_UTF8,0,text.data(),int(text.size()),nullptr,0,nullptr,nullptr);std::string value(n,0);WideCharToMultiByte(CP_UTF8,0,text.data(),int(text.size()),value.data(),n,nullptr,nullptr);out<<json(value);}out<<']';}
 out<<"],\"volumes\":[";for(size_t i=0;i<rows.size();++i){if(i)out<<',';auto r=manualVolume::calculate({rows[i]},scale);if(r.count&&r.error.empty())out<<r.mean;else out<<"null";}out<<"]}";return out.str();
}
}
