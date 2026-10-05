#pragma once
#include "web_preferences.hpp"
struct WebPortDialog {bool done=false;};
LRESULT CALLBACK WebPortProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 auto d=(WebPortDialog*)GetWindowLongPtrW(w,GWLP_USERDATA);
 if(msg==WM_NCCREATE){d=(WebPortDialog*)((CREATESTRUCTW*)lp)->lpCreateParams;SetWindowLongPtrW(w,GWLP_USERDATA,(LONG_PTR)d);}
 if(msg==WM_COMMAND&&LOWORD(wp)==IDOK){wchar_t value[32]{};GetDlgItemTextW(w,501,value,32);try{std::wstring text=value;size_t n;int port=std::stoi(text,&n);if(n!=text.size()||port<1||port>65535)throw std::runtime_error("port");if(!webprefs::save(port)){MessageBoxW(w,L"Не удалось сохранить порт. Проверьте права на папку программы.",L"Веб-интерфейс",MB_ICONERROR);return 0;}DestroyWindow(w);}catch(...){MessageBoxW(w,L"Введите целое число от 1 до 65535.",L"Порт веб-интерфейса",MB_ICONINFORMATION);}return 0;}
 if(msg==WM_CLOSE||(msg==WM_COMMAND&&LOWORD(wp)==IDCANCEL)){DestroyWindow(w);return 0;}
 if(msg==WM_DESTROY){d->done=true;return 0;}return DefWindowProcW(w,msg,wp,lp);
}
void webPortSettings(){
 WebPortDialog d;auto inst=GetModuleHandleW(nullptr);WNDCLASSW wc{};wc.hInstance=inst;wc.lpfnWndProc=WebPortProc;wc.lpszClassName=L"LaseWebPort";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);RegisterClassW(&wc);
 RECT r;GetWindowRect(mainWin,&r);auto w=CreateWindowExW(WS_EX_DLGMODALFRAME,L"LaseWebPort",L"Веб-интерфейс · порт",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,r.left+100,r.top+80,500,250,mainWin,nullptr,inst,&d);
 auto add=[&](const wchar_t* cls,const std::wstring& text,int id,int x,int y,int width,int height,DWORD style=0){auto c=CreateWindowW(cls,text.c_str(),WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,(HMENU)(INT_PTR)id,inst,nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)font,TRUE);return c;};
 add(L"STATIC",L"Порт (1–65535)",0,20,20,220,25);auto edit=add(L"EDIT",std::to_wstring(webprefs::port()),501,260,20,190,30,WS_BORDER|WS_TABSTOP|ES_NUMBER);SendMessageW(edit,EM_SETLIMITTEXT,5,0);
 add(L"STATIC",L"Адрес: http://127.0.0.1:порт/\nПорт применяется при следующем запуске веб-версии.\nДоступ локальный, на этом компьютере.",0,20,65,440,75);
 add(L"BUTTON",L"Сохранить",IDOK,200,155,120,32,WS_TABSTOP|BS_DEFPUSHBUTTON);add(L"BUTTON",L"Отмена",IDCANCEL,330,155,120,32,WS_TABSTOP);
 EnableWindow(mainWin,FALSE);skinSettings(w);ShowWindow(w,SW_SHOW);SetFocus(edit);MSG msg;while(!d.done&&GetMessageW(&msg,nullptr,0,0)>0)if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}EnableWindow(mainWin,TRUE);SetActiveWindow(mainWin);
}
constexpr int THEME_LIGHT=140,THEME_DARK=141,FORMAT_HTML=142,FORMAT_PDF=143;
HMENU themeMenu(){auto m=CreatePopupMenu();AppendMenuW(m,MF_STRING|(lightTheme?MF_CHECKED:0),THEME_LIGHT,L"Светлая");AppendMenuW(m,MF_STRING|(!lightTheme?MF_CHECKED:0),THEME_DARK,L"Тёмная");return m;}
HMENU formatMenu(){auto m=CreatePopupMenu();AppendMenuW(m,MF_STRING|(reportFormat==ReportFormat::HTML?MF_CHECKED:0),FORMAT_HTML,L"HTML");AppendMenuW(m,MF_STRING|(reportFormat==ReportFormat::PDF?MF_CHECKED:0),FORMAT_PDF,L"PDF");return m;}
std::wstring preferencesFile(){wchar_t file[32768]{};GetModuleFileNameW(nullptr,file,32768);return (std::filesystem::path(file).parent_path()/L"LaseScanViewer.ini").wstring();}
void loadPreferences(){auto file=preferencesFile();lightTheme=GetPrivateProfileIntW(L"Interface",L"Dark",1,file.c_str())==0;reportFormat=GetPrivateProfileIntW(L"Interface",L"PDF",0,file.c_str())?ReportFormat::PDF:ReportFormat::HTML;}
void savePreferences(){if(v2TestMode)return;auto file=preferencesFile();WritePrivateProfileStringW(L"Interface",L"Dark",lightTheme?L"0":L"1",file.c_str());WritePrivateProfileStringW(L"Interface",L"PDF",reportFormat==ReportFormat::PDF?L"1":L"0",file.c_str());}
bool preferenceCommand(int id){if(id==THEME_LIGHT||id==THEME_DARK){applyTheme(id==THEME_LIGHT);savePreferences();return true;}if(id==FORMAT_HTML||id==FORMAT_PDF){reportFormat=id==FORMAT_HTML?ReportFormat::HTML:ReportFormat::PDF;savePreferences();status(reportFormat==ReportFormat::PDF?L"Формат отчёта: PDF":L"Формат отчёта: HTML");return true;}return false;}
