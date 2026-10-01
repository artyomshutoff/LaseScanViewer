#pragma once
constexpr int THEME_LIGHT=140,THEME_DARK=141,FORMAT_HTML=142,FORMAT_PDF=143;
HMENU themeMenu(){auto m=CreatePopupMenu();AppendMenuW(m,MF_STRING|(lightTheme?MF_CHECKED:0),THEME_LIGHT,L"Светлая");AppendMenuW(m,MF_STRING|(!lightTheme?MF_CHECKED:0),THEME_DARK,L"Тёмная");return m;}
HMENU formatMenu(){auto m=CreatePopupMenu();AppendMenuW(m,MF_STRING|(reportFormat==ReportFormat::HTML?MF_CHECKED:0),FORMAT_HTML,L"HTML");AppendMenuW(m,MF_STRING|(reportFormat==ReportFormat::PDF?MF_CHECKED:0),FORMAT_PDF,L"PDF");return m;}
std::wstring preferencesFile(){wchar_t file[32768]{};GetModuleFileNameW(nullptr,file,32768);return (std::filesystem::path(file).parent_path()/L"LaseScanViewer.ini").wstring();}
void loadPreferences(){auto file=preferencesFile();lightTheme=GetPrivateProfileIntW(L"Interface",L"Dark",1,file.c_str())==0;reportFormat=GetPrivateProfileIntW(L"Interface",L"PDF",0,file.c_str())?ReportFormat::PDF:ReportFormat::HTML;}
void savePreferences(){if(v2TestMode)return;auto file=preferencesFile();WritePrivateProfileStringW(L"Interface",L"Dark",lightTheme?L"0":L"1",file.c_str());WritePrivateProfileStringW(L"Interface",L"PDF",reportFormat==ReportFormat::PDF?L"1":L"0",file.c_str());}
bool preferenceCommand(int id){if(id==THEME_LIGHT||id==THEME_DARK){applyTheme(id==THEME_LIGHT);savePreferences();return true;}if(id==FORMAT_HTML||id==FORMAT_PDF){reportFormat=id==FORMAT_HTML?ReportFormat::HTML:ReportFormat::PDF;savePreferences();status(reportFormat==ReportFormat::PDF?L"Формат отчёта: PDF":L"Формат отчёта: HTML");return true;}return false;}
