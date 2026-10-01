#include "../src/control_database.hpp"
#include <cassert>
#include <iostream>
int wmain(int argc,wchar_t** argv){assert(argc==3);control::Database db;db.load(argv[1]);auto real=db.find(std::wcstoll(argv[2],nullptr,10),"");assert(real.record&&real.record->valid&&real.record->total>0);std::cout<<"real control: "<<real.record->id<<" "<<real.record->total<<"\n";assert(!db.find(999999999,"").record);assert(!db.find(real.record->id,"WRONG-TRUCK").record);
 auto path=std::filesystem::path(L"build/control323-fixture.sq3");std::filesystem::remove(path);sqlite3* fixture=nullptr;assert(sqlite3_open(path.u8string().c_str(),&fixture)==SQLITE_OK);
 assert(sqlite3_exec(fixture,"CREATE TABLE LaseTVM (MeasurementID INTEGER, IncomingMeasurementID INTEGER, OutgoingMeasurementID INTEGER, TotalVolume REAL, TruckID TEXT, Status INTEGER, TrailerVolume REAL); INSERT INTO LaseTVM VALUES (1,100,0,0,'A',5,0),(2,200,0,NULL,'B',5,0),(3,300,0,-1,'C',5,0),(4,400,0,10,'D',5,0),(5,400,0,12,'D',5,0),(6,600,0,3,'F',5,0),(6,700,0,7,'F',5,0)",nullptr,nullptr,nullptr)==SQLITE_OK);sqlite3_close(fixture);
 db.load(path);assert(db.find(1,"A").record->total==0);assert(db.find(100,"A").record->id==1);assert(!db.find(2,"B").record&&!db.find(3,"C").record);assert(!db.find(400,"D").record&&!db.find(6,"F").record);
 auto old=db.path;try{db.load(L"build/not-a-real-database.sq3");assert(false);}catch(const std::exception&){}assert(db.path==old);assert(!std::filesystem::exists(L"build/not-a-real-database.sq3"));std::cout<<"PASS lookup, aliases, missing, invalid, zero, truck mismatch, ambiguity, failed load preserves old DB\n";
}
