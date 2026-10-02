#include "../src/manual_volume.hpp"
#include <cassert>
#include <iostream>
int main(){
 using namespace manualVolume;
 std::vector<Row> rows{{L"5",L"2",L"1"},{L"5,0",L"2",L"1,5"},{L"5",L"2",L"1"},{L"5",L"2",L"1.5"},{L"5",L"2",L"1"}};
 auto r=calculate(rows);assert(r.count==5&&r.mean==12&&r.minimum==10&&r.maximum==15&&r.dimensions[2]==1.2);
 assert(differencePercent(15,r.mean)==25);assert(differencePercent(0,r.mean)==-100);assert(!differencePercent(10,0));
 rows.push_back({});assert(calculate(rows).mean==12);rows.erase(rows.begin()+1);assert(calculate(rows).mean==11.25);
 for(auto invalid:{L"0",L"-1",L"nan",L"inf",L"1,2.3",L"1foo",L"1 2",L"",L"1e309"}){rows[0][0]=invalid;assert(!calculate(rows).error.empty());}
 assert(dimension(L" 2,5 ")==2.5);assert(!dimension(L"0."+std::wstring(400,L'0')+L"1"));
 assert(calculate(std::vector<Row>(5)).count==0);
 auto centimetres=calculate({{L"500",L"200",L"100"},{L"500",L"200",L"150"}},.01);
 assert(centimetres.mean==12.5&&centimetres.minimum==10&&centimetres.maximum==15&&centimetres.dimensions[0]==5);
 assert(!calculate(rows,0).error.empty());assert(!calculate(rows,-1).error.empty());
 assert(convertDimension(L"5.05",true)==L"505");assert(convertDimension(L"505",false)==L"5.05");
 assert(convertDimension(L" 0002,3100 ",true)==L"231");assert(convertDimension(L"136",false)==L"1.36");
 assert(convertDimension(L".025",true)==L"2.5");assert(convertDimension(L"2.5",false)==L"0.025");
 assert(convertDimension(L"0005.",true)==L"500");assert(!convertDimension(L"1e3",true));assert(!convertDimension(L"0",false));
 for(auto value:{L"5.05",L"2.31",L"1.36",L"2.2",L"1.4",L"1.23456789012345",L"0.00000001"}){
  std::wstring text=value;for(int i=0;i<100;++i){text=*convertDimension(text,true);text=*convertDimension(text,false);}assert(text==value);
 }
 std::vector<Row> measured{{L"5.05",L"2.31",L"1.36"},{L"5",L"2.2",L"1.4"},{L"5.25",L"2.25",L"1.3"}};
 auto expected=calculate(measured);for(int i=0;i<100;++i){auto converted=measured;for(auto& row:converted)for(auto& text:row)text=*convertDimension(text,true);assert(std::abs(calculate(converted,.01).mean-expected.mean)<1e-12);for(auto& row:converted)for(auto& text:row)text=*convertDimension(text,false);assert(converted==measured);}
 // Mean of per-measurement volumes differs from product of mean dimensions.
 r=calculate({{L"2",L"1",L"1"},{L"4",L"2",L"1"}});assert(r.mean==5&&r.dimensions[0]*r.dimensions[1]*r.dimensions[2]==4.5);
 std::cout<<"PASS manual volumes, arithmetic means, ranges, signs, blanks, invalid input and decimal comma\n";
}
