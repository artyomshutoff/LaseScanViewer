#pragma once
#include "vendor/sqlite/sqlite3.h"
#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <optional>
#include <cmath>
#include <memory>
#include <filesystem>
#include <stdexcept>
namespace control {
struct Record {int64_t id=0,incoming=0,outgoing=0;double total=0,trailer=0;int status=0;bool valid=false;std::string truck;};
struct Match {std::optional<Record> record;std::string reason;};
class Database {
public:
 std::filesystem::path path;std::map<int64_t,std::vector<Record>> ids,aliases;
 void load(const std::filesystem::path& file){
  sqlite3* raw=nullptr;int rc=sqlite3_open_v2(file.u8string().c_str(),&raw,SQLITE_OPEN_READONLY,nullptr);std::unique_ptr<sqlite3,decltype(&sqlite3_close)> db(raw,sqlite3_close);
  if(rc!=SQLITE_OK)throw std::runtime_error(raw?sqlite3_errmsg(raw):"Cannot open SQLite");sqlite3_busy_timeout(raw,1500);
  sqlite3_stmt* q=nullptr;rc=sqlite3_prepare_v2(raw,"SELECT MeasurementID, IncomingMeasurementID, OutgoingMeasurementID, TotalVolume, TruckID, Status, TrailerVolume FROM LaseTVM",-1,&q,nullptr);
  std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)> stmt(q,sqlite3_finalize);if(rc!=SQLITE_OK)throw std::runtime_error(sqlite3_errmsg(raw));
  Database next;next.path=file;size_t count=0;
  while((rc=sqlite3_step(q))==SQLITE_ROW){if(++count>1000000)throw std::runtime_error("Database exceeds 1000000 records");Record r;r.id=sqlite3_column_int64(q,0);r.incoming=sqlite3_column_int64(q,1);r.outgoing=sqlite3_column_int64(q,2);r.total=sqlite3_column_double(q,3);r.valid=(sqlite3_column_type(q,3)==SQLITE_FLOAT||sqlite3_column_type(q,3)==SQLITE_INTEGER)&&std::isfinite(r.total)&&r.total>=0;
   const auto* text=sqlite3_column_text(q,4);r.truck=text?(const char*)text:"";r.status=sqlite3_column_int(q,5);r.trailer=sqlite3_column_double(q,6);next.ids[r.id].push_back(r);
   if(r.incoming>0)next.aliases[r.incoming].push_back(r);if(r.outgoing>0&&r.outgoing!=r.incoming)next.aliases[r.outgoing].push_back(r);
  }if(rc!=SQLITE_DONE)throw std::runtime_error(sqlite3_errmsg(raw));*this=std::move(next);
 }
 Match find(int64_t id,const std::string& truck)const{
  if(path.empty())return {{},"База не загружена"};auto direct=ids.find(id);auto fallback=aliases.find(id);const auto* rows=direct!=ids.end()?&direct->second:fallback!=aliases.end()?&fallback->second:nullptr;
  if(!rows)return {{},"Нет записи для измерения "+std::to_string(id)};if(rows->size()!=1)return {{},"Неоднозначное соответствие измерения"};auto r=rows->front();
  if(!truck.empty()&&!r.truck.empty()&&truck!=r.truck)return {{},"TruckID не совпадает со сканом"};if(!r.valid)return {{},"TotalVolume не задан"};return {r,""};
 }
};
}
