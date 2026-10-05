#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include "web_open.hpp"
#include <bcrypt.h>
#include <iostream>
#include "web_core.hpp"
#include "web_manual.hpp"
#include "web_preferences.hpp"
#include "web_session.hpp"
#include "../build/web_assets.hpp"
namespace {
struct Socket {SOCKET value=INVALID_SOCKET;explicit Socket(SOCKET s):value(s){}~Socket(){if(value!=INVALID_SOCKET)closesocket(value);}Socket(const Socket&)=delete;};
struct Temp {std::filesystem::path path;~Temp(){std::error_code ec;std::filesystem::remove_all(path,ec);}};
struct Request {std::string method,path,query,body;std::map<std::string,std::string> headers;};
std::string lower(std::string s){for(auto& c:s)c=char(std::tolower((unsigned char)c));return s;}
void sendAll(SOCKET s,const std::string& bytes){size_t n=0;while(n<bytes.size()){int sent=send(s,bytes.data()+n,int(std::min<size_t>(bytes.size()-n,1024*1024)),0);if(sent<=0)return;n+=sent;}}
void respond(SOCKET s,int code,const std::string& type,const std::string& body,const std::string& attachment=""){
 std::ostringstream head;head<<"HTTP/1.1 "<<code<<" Response\r\nContent-Type: "<<type<<"\r\nContent-Length: "<<body.size()<<"\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer\r\nContent-Security-Policy: default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data: blob:; connect-src 'self'; object-src 'none'; frame-ancestors 'none'; base-uri 'none'\r\n";if(!attachment.empty())head<<"Content-Disposition: attachment; filename=\""<<attachment<<"\"\r\n";head<<"\r\n";sendAll(s,head.str());sendAll(s,body);
}
Request receive(SOCKET s){
 std::string bytes;char buffer[65536];size_t end;
 while((end=bytes.find("\r\n\r\n"))==std::string::npos){if(bytes.size()>16384)throw std::runtime_error("HTTP header too large");int n=recv(s,buffer,sizeof(buffer),0);if(n<=0)throw std::runtime_error("Incomplete request");bytes.append(buffer,n);}
 if(end>16384)throw std::runtime_error("HTTP header too large");
 Request r;std::istringstream input(bytes.substr(0,end));std::string line,version;std::getline(input,line);std::istringstream first(line);first>>r.method>>r.path>>version;if(version!="HTTP/1.1"&&version!="HTTP/1.0")throw std::runtime_error("Unsupported HTTP version");
 while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();auto colon=line.find(':');if(colon==std::string::npos)throw std::runtime_error("Invalid header");auto key=lower(line.substr(0,colon));auto value=line.substr(colon+1);while(!value.empty()&&value.front()==' ')value.erase(value.begin());if(!r.headers.emplace(key,value).second)throw std::runtime_error("Duplicate header");}
 if(r.headers.count("transfer-encoding"))throw std::runtime_error("Chunked requests are unsupported");
 size_t length=0;if(r.headers.count("content-length")){size_t consumed;auto str=r.headers.at("content-length");if(str.empty()||str.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Invalid content length");length=std::stoull(str,&consumed);if(consumed!=str.size()||length>256*1024*1024)throw std::runtime_error("File exceeds 256 MiB");}
 r.body=bytes.substr(end+4);if(r.body.size()>length)throw std::runtime_error("Unexpected request data");r.body.reserve(length);
 while(r.body.size()<length){int n=recv(s,buffer,int(std::min<size_t>(sizeof(buffer),length-r.body.size())),0);if(n<=0)throw std::runtime_error("Incomplete upload");r.body.append(buffer,n);}
 auto q=r.path.find('?');if(q!=std::string::npos){r.query=r.path.substr(q+1);r.path.resize(q);}return r;
}
std::map<std::string,std::string> parameters(const std::string& query){std::map<std::string,std::string> out;std::istringstream input(query);std::string part;while(std::getline(input,part,'&')){auto eq=part.find('=');if(eq==std::string::npos||!out.emplace(part.substr(0,eq),part.substr(eq+1)).second)throw std::runtime_error("Invalid query");}return out;}
double number(const std::map<std::string,std::string>& p,const std::string& key,double fallback,double low,double high){auto i=p.find(key);if(i==p.end())return fallback;size_t n=0;double v=std::stod(i->second,&n);if(n!=i->second.size()||!std::isfinite(v)||v<low||v>high)throw std::runtime_error("Invalid parameter: "+key);return v;}
void write(const std::filesystem::path& path,const std::string& bytes){std::ofstream f(path,std::ios::binary|std::ios::trunc);f.write(bytes.data(),bytes.size());f.close();if(!f)throw std::runtime_error("Cannot save uploaded file");}
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){
 std::filesystem::path ready;bool noBrowser=false;int selectedPort=webprefs::port();
 try{
  int argc;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);std::vector<std::filesystem::path> paths;
  for(int i=1;i<argc;++i){std::wstring arg=argv[i];if(arg==L"--no-browser")noBrowser=true;else if(arg==L"--port"&&i+1<argc){std::wstring v=argv[++i];size_t n;selectedPort=std::stoi(v,&n);if(n!=v.size()||selectedPort<0||selectedPort>65535)throw std::runtime_error("Invalid web port");}else if(arg==L"--ready-file"&&i+1<argc)ready=argv[++i];else paths.emplace_back(arg);}LocalFree(argv);
  WSADATA data;if(WSAStartup(MAKEWORD(2,2),&data))throw std::runtime_error("Winsock initialization failed");
  unsigned char random[24];if(BCryptGenRandom(nullptr,random,sizeof(random),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)throw std::runtime_error("Cannot initialize local session");std::string token;const char* hex="0123456789abcdef";for(auto c:random){token+=hex[c>>4];token+=hex[c&15];}
  Temp temp;temp.path=std::filesystem::temp_directory_path()/(L"LaseScanViewer-web-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::wstring(token.begin(),token.end()));std::filesystem::create_directory(temp.path);
  web::State state;std::optional<std::filesystem::path> fullFile,emptyFile;
  auto openLocal=[&](const std::filesystem::path& p,bool includePair){
   auto scans=webopen::read(p,includePair);
   if(includePair){if(scans.front().model.kind==2){state.empty.reset();emptyFile.reset();}else if(scans.front().model.kind==1){state.full.reset();fullFile.reset();}}
   for(auto& scan:scans){bool active=scan.model.kind==2;state.loaded(std::move(scan.model),active);(active?fullFile:emptyFile)=scan.path;}
  };
  for(const auto& p:paths)openLocal(p,paths.size()==1);
  Socket listener(socket(AF_INET,SOCK_STREAM,IPPROTO_TCP));if(listener.value==INVALID_SOCKET)throw std::runtime_error("Cannot create local web listener");BOOL exclusive=TRUE;setsockopt(listener.value,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(char*)&exclusive,sizeof(exclusive));
  sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=htons((u_short)selectedPort);
  if(bind(listener.value,(sockaddr*)&address,sizeof(address))||listen(listener.value,8))throw std::runtime_error("Порт "+std::to_string(selectedPort)+" занят или недоступен. Выберите другой порт в настройках веб-интерфейса.");int length=sizeof(address);getsockname(listener.value,(sockaddr*)&address,&length);
  auto host="127.0.0.1:"+std::to_string(ntohs(address.sin_port)),origin="http://"+host,url=origin+"/";
  if(!ready.empty())write(ready,url);
  if(!noBrowser){auto wide=std::wstring(url.begin(),url.end());if((INT_PTR)ShellExecuteW(nullptr,L"open",wide.c_str(),nullptr,nullptr,SW_SHOWNORMAL)<=32)throw std::runtime_error("Не удалось открыть браузер");}
  websession::Tabs tabs;bool stopping=false;auto last=std::chrono::steady_clock::now();std::map<std::string,std::string> exports;
  while(!stopping){
   if(tabs.expired())break;
   fd_set sockets;FD_ZERO(&sockets);FD_SET(listener.value,&sockets);timeval timeout{1,0};int available=select(0,&sockets,nullptr,nullptr,&timeout);if(available<0)break;state.poll();if(!available){if(!state.busy&&std::chrono::steady_clock::now()-last>std::chrono::minutes(60))break;continue;}
   Socket client(accept(listener.value,nullptr,nullptr));if(client.value==INVALID_SOCKET)continue;DWORD wait=30000;setsockopt(client.value,SOL_SOCKET,SO_RCVTIMEO,(char*)&wait,sizeof(wait));setsockopt(client.value,SOL_SOCKET,SO_SNDTIMEO,(char*)&wait,sizeof(wait));
   try{auto r=receive(client.value);if(r.headers["host"]!=host|| (r.headers.count("origin")&&r.headers.at("origin")!=origin)){respond(client.value,403,"application/json","{\"error\":\"Local origin required\"}");continue;}
    if(r.headers.count("sec-fetch-site")&&r.headers.at("sec-fetch-site")=="cross-site"){respond(client.value,403,"application/json","{\"error\":\"Local origin required\"}");continue;}
    if(r.method=="GET"&&r.path=="/api/session"){respond(client.value,200,"application/json","{\"token\":"+web::json(token)+"}");continue;}
    if(r.path.rfind("/api/",0)!=0){auto asset=webAssets.find(r.path=="/"?"/index.html":r.path);if(r.method!="GET"||asset==webAssets.end()){respond(client.value,404,"text/plain","Not found");continue;}respond(client.value,200,asset->second.first,asset->second.second);continue;}
    auto params=parameters(r.query);bool download=r.method=="GET"&&r.path=="/api/download";
    if(r.headers["x-lase-token"]!=token&&(!download||params["key"]!=token)){respond(client.value,403,"application/json","{\"error\":\"Invalid local session\"}");continue;}last=std::chrono::steady_clock::now();state.poll();
    if(r.method=="POST"&&(r.path=="/api/tab/open"||r.path=="/api/tab/ping"||r.path=="/api/tab/close")){
     if(r.path=="/api/tab/close")tabs.leave(params["id"]);else tabs.touch(params["id"],number(params,"hidden",0,0,1)==1);
     respond(client.value,200,"application/json","{}");continue;
    }
    if(download){auto format=params["format"];auto found=exports.find(format);if(found==exports.end())throw std::runtime_error("Export not found");auto id=state.full?state.full->scan:state.empty?state.empty->scan:0;respond(client.value,200,format=="png"?"image/png":"text/html; charset=utf-8",found->second,"LaseScanViewer-"+std::to_string(id)+"."+format);continue;}
    if(r.method=="GET"&&r.path=="/api/status"){respond(client.value,200,"application/json; charset=utf-8",state.metadata());continue;}
    if(r.method=="GET"&&r.path=="/api/scene"){respond(client.value,200,"application/octet-stream",state.scene(params["layer"],number(params,"mesh",0,0,1)==1));continue;}
    if(r.method=="POST"&&r.path=="/api/manual"){respond(client.value,200,"application/json; charset=utf-8",web::manualResponse(r.body));continue;}
    if(r.method=="GET"&&r.path=="/api/preferences"){respond(client.value,200,"application/json","{\"port\":"+std::to_string(webprefs::port())+",\"activePort\":"+std::to_string(ntohs(address.sin_port))+"}");continue;}
    if(r.method=="POST"&&r.path=="/api/preferences"){double port=number(params,"port",-1,1,65535);if(port!=std::floor(port)||!webprefs::save(int(port)))throw std::runtime_error("Не удалось сохранить порт. Проверьте права на папку программы.");respond(client.value,200,"application/json","{}");continue;}
    if(state.busy){respond(client.value,409,"application/json", "{\"error\":\"Дождитесь завершения расчёта\"}");continue;}
    if(r.method=="POST"&&r.path=="/api/open"){
     if(params["layer"]!="full"&&params["layer"]!="empty")throw std::runtime_error("Invalid scan layer");
     std::vector<wchar_t> filename(32768);OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.lpstrFile=filename.data();dialog.nMaxFile=DWORD(filename.size());dialog.lpstrFilter=L"LASE BIN\0*.bin\0Все файлы\0*.*\0";dialog.lpstrTitle=params["layer"]=="full"?L"Открыть Full / пару — Empty подгрузится автоматически":L"Открыть Empty / Reference — Full подгрузится автоматически";dialog.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
     if(GetOpenFileNameW(&dialog)){std::filesystem::path selected(filename.data());auto probe=readModel(selected);bool active=params["layer"]=="full";if((active&&probe.kind!=2)||(!active&&probe.kind==2))throw std::runtime_error("Выберите Full для груза, Empty / Reference для базы");openLocal(selected,active||!state.full);}
     else if(CommDlgExtendedError())throw std::runtime_error("Не удалось открыть диалог выбора файла");
     respond(client.value,200,"application/json; charset=utf-8",state.metadata());continue;
    }
    if(r.method=="POST"&&r.path=="/api/export"){auto format=params["format"];if((format!="png"&&format!="html")||r.body.size()>32*1024*1024)throw std::runtime_error("Invalid export");exports[format]=std::move(r.body);respond(client.value,200,"application/json","{}");continue;}
    if(r.method=="PUT"&&(r.path=="/api/full"||r.path=="/api/empty"||r.path=="/api/database")){
     auto file=temp.path/L"upload.tmp";write(file,r.body);
     if(r.path=="/api/database"){control::Database next;next.load(file);state.database=std::move(next);}
     else{auto m=readModel(file,int(number(params,"group",-1,-1,1)));triangulate(m);bool active=r.path=="/api/full";if((active&&m.kind!=2)||(!active&&m.kind==2))throw std::runtime_error("Выберите Full для груза, Empty / Reference для базы");auto retained=temp.path/(active?L"full.bin":L"empty.bin");std::filesystem::copy_file(file,retained,std::filesystem::copy_options::overwrite_existing);state.loaded(std::move(m),active);(active?fullFile:emptyFile)=retained;}
     std::filesystem::remove(file);respond(client.value,200,"application/json; charset=utf-8",state.metadata());continue;
    }
    if(r.method=="POST"&&r.path=="/api/group"){
     bool active=params["layer"]=="full";if(!active&&params["layer"]!="empty")throw std::runtime_error("Invalid scan layer");auto file=active?fullFile:emptyFile;if(!file)throw std::runtime_error("Скан не открыт");double group=number(params,"group",-1,0,1);if(group!=0&&group!=1)throw std::runtime_error("Выберите набор 0 или 1");auto m=readModel(*file,int(group));triangulate(m);state.loaded(std::move(m),active);respond(client.value,200,"application/json; charset=utf-8",state.metadata());continue;
    }
    if(r.method=="POST"&&(r.path=="/api/calculate"||r.path=="/api/align")){
     CompareOptions o;o.step=number(params,"step",100,10,2000);o.metresPerUnit=number(params,"unit",.001,.001,1);if(o.metresPerUnit!=.001&&o.metresPerUnit!=.01&&o.metresPerUnit!=1)throw std::runtime_error("Select mm, cm or m");o.adaptiveGrid=number(params,"adaptive",1,0,1)==1;o.reconstructGaps=number(params,"gaps",1,0,1)==1;o.calibratedEstimate=number(params,"estimate",1,0,1)==1;
     state.start(r.path=="/api/calculate",o);respond(client.value,202,"application/json; charset=utf-8",state.metadata());continue;
    }
    if(r.method=="POST"&&r.path=="/api/stop"){stopping=true;respond(client.value,200,"application/json","{}");continue;}
    respond(client.value,404,"application/json","{\"error\":\"Not found\"}");
   }catch(const std::exception& e){respond(client.value,400,"application/json; charset=utf-8","{\"error\":"+web::json(e.what())+"}");}
  }WSACleanup();return 0;
 }catch(const std::exception& e){if(!ready.empty())write(ready,std::string("ERROR: ")+e.what());if(!noBrowser){std::string text=e.what();int n=MultiByteToWideChar(CP_UTF8,0,text.data(),int(text.size()),nullptr,0);std::wstring w(n,0);MultiByteToWideChar(CP_UTF8,0,text.data(),int(text.size()),w.data(),n);MessageBoxW(nullptr,w.c_str(),L"LaseScanViewer Web",MB_ICONERROR);}return 1;}
}
