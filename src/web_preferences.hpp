#pragma once
namespace webprefs {
inline std::wstring file(){wchar_t path[32768]{};GetModuleFileNameW(nullptr,path,32768);return (std::filesystem::path(path).parent_path()/L"LaseScanViewer.ini").wstring();}
inline int port(){int value=GetPrivateProfileIntW(L"Web",L"Port",3000,file().c_str());return value>=1&&value<=65535?value:3000;}
inline bool save(int value){return value>=1&&value<=65535&&WritePrivateProfileStringW(L"Web",L"Port",std::to_wstring(value).c_str(),file().c_str());}
}
