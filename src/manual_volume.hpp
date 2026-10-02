#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace manualVolume {
using Row=std::array<std::wstring,3>;
inline bool blank(const std::wstring& text){return text.find_first_not_of(L" \t\r\n")==std::wstring::npos;}
inline std::optional<double> dimension(std::wstring text){
 auto first=text.find_first_not_of(L" \t\r\n"),last=text.find_last_not_of(L" \t\r\n");
 if(first==std::wstring::npos)return {};text=text.substr(first,last-first+1);
 std::replace(text.begin(),text.end(),L',',L'.');
 if(text.find_first_not_of(L"0123456789.")!=std::wstring::npos)return {};
 std::wistringstream input(text);input.imbue(std::locale::classic());double value=0;
 input>>value;if(input.fail()||!input.eof()||!std::isfinite(value)||value<=0)return {};return value;
}
// Metres <-> centimetres is a decimal-point shift, not binary floating-point
// multiplication. Keep the entered decimal precision and never generate a
// 5.049999... suffix merely by switching units.
inline std::optional<std::wstring> convertDimension(std::wstring text,bool toCentimetres){
 if(!dimension(text))return {};
 auto first=text.find_first_not_of(L" \t\r\n"),last=text.find_last_not_of(L" \t\r\n");text=text.substr(first,last-first+1);
 std::replace(text.begin(),text.end(),L',',L'.');auto dot=text.find(L'.');
 int position=dot==std::wstring::npos?int(text.size()):int(dot);
 if(dot!=std::wstring::npos)text.erase(dot,1);
 auto significant=text.find_first_not_of(L'0');if(significant==std::wstring::npos)return {};
 position-=int(significant);text.erase(0,significant);position+=toCentimetres?2:-2;
 if(position<=0)text=L"0."+std::wstring(size_t(-position),L'0')+text;
 else if(position>=int(text.size()))text.append(size_t(position-int(text.size())),L'0');
 else text.insert(size_t(position),L".");
 if(text.find(L'.')!=std::wstring::npos){while(text.back()==L'0')text.pop_back();if(text.back()==L'.')text.pop_back();}
 if(!dimension(text))return {};return text;
}
struct Result {
 size_t count=0,invalidRow=0;std::array<double,3> dimensions{};
 double mean=0,minimum=0,maximum=0;std::string error;
};
inline Result calculate(const std::vector<Row>& rows,double metresPerInputUnit=1){
 Result result;long double sum=0;std::array<long double,3> dimensions{};
 if(!std::isfinite(metresPerInputUnit)||metresPerInputUnit<=0){result.error="Invalid input unit";return result;}
 for(size_t i=0;i<rows.size();++i){
  if(std::all_of(rows[i].begin(),rows[i].end(),blank))continue;
  std::array<double,3> values{};
  for(int j=0;j<3;++j){auto value=dimension(rows[i][j]);if(!value){result.invalidRow=i+1;result.error="Enter three positive dimensions in metres";return result;}values[j]=*value;}
  for(auto& value:values)value*=metresPerInputUnit;
  double volume=values[0]*values[1]*values[2];
  if(!std::isfinite(volume)||volume<=0){result.invalidRow=i+1;result.error="Volume exceeds the numeric range";return result;}
  if(!result.count)result.minimum=result.maximum=volume;
  result.minimum=std::min(result.minimum,volume);result.maximum=std::max(result.maximum,volume);
  sum+=volume;for(int j=0;j<3;++j)dimensions[j]+=values[j];++result.count;
 }
 if(result.count){result.mean=double(sum/result.count);for(int j=0;j<3;++j)result.dimensions[j]=double(dimensions[j]/result.count);}
 return result;
}
// Manual measurements are the reference; zero scan volume remains comparable.
inline std::optional<double> differencePercent(double scan,double reference){
 if(!std::isfinite(scan)||scan<0||!std::isfinite(reference)||reference<=0)return {};
 double percent=(scan/reference-1)*100;return std::isfinite(percent)?std::optional<double>(percent):std::nullopt;
}
}
