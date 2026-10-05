#pragma once
// Dialog controls use the same palette as the viewer; native keyboard behavior
// and auto-check/radio behavior stay intact.
void settingsCheckPaint(HWND w,HDC dc){
 RECT r;GetClientRect(w,&r);fill(dc,r,bg);bool enabled=IsWindowEnabled(w),checked=SendMessageW(w,BM_GETCHECK,0,0)==BST_CHECKED;RECT box{1,(r.bottom-16)/2,17,(r.bottom+16)/2};fill(dc,box,checked&&enabled?accent:card);SetDCBrushColor(dc,enabled?border:muted);FrameRect(dc,&box,(HBRUSH)GetStockObject(DC_BRUSH));if(checked){auto pen=CreatePen(PS_SOLID,2,enabled?bg:muted);auto old=SelectObject(dc,pen);MoveToEx(dc,box.left+3,box.top+8,nullptr);LineTo(dc,box.left+7,box.top+12);LineTo(dc,box.left+13,box.top+4);SelectObject(dc,old);DeleteObject(pen);}wchar_t text[1024]{};GetWindowTextW(w,text,1024);r.left=25;label(dc,r,text,enabled?ink:muted);if(GetFocus()==w){InflateRect(&r,-2,-2);DrawFocusRect(dc,&r);}
}
LRESULT CALLBACK SettingsCheckProc(HWND w,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR){
 if(msg==WM_PAINT){PAINTSTRUCT ps;auto dc=BeginPaint(w,&ps);settingsCheckPaint(w,dc);EndPaint(w,&ps);return 0;}
 if(msg==WM_PRINTCLIENT){settingsCheckPaint(w,(HDC)wp);return 0;}
 if(msg==WM_ERASEBKGND)return 1;
 auto result=DefSubclassProc(w,msg,wp,lp);if(msg==BM_SETCHECK||msg==BM_CLICK||msg==WM_ENABLE||msg==WM_SETFOCUS||msg==WM_KILLFOCUS||msg==WM_KEYUP||msg==WM_LBUTTONUP)InvalidateRect(w,nullptr,FALSE);return result;
}
void captureSettingsDialog(HWND w,const std::filesystem::path& path){
 RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);RECT r;GetClientRect(w,&r);HDC dc=GetDC(w),mem=CreateCompatibleDC(dc);BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=r.right;bi.bmiHeader.biHeight=-r.bottom;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;void* bits;auto bmp=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,nullptr,0);auto old=SelectObject(mem,bmp);PrintWindow(w,mem,PW_CLIENTONLY);BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(bi.bmiHeader);header.bfSize=header.bfOffBits+r.right*r.bottom*4;std::ofstream file(path,std::ios::binary);file.write((char*)&header,sizeof(header));file.write((char*)&bi.bmiHeader,sizeof(bi.bmiHeader));file.write((char*)bits,r.right*r.bottom*4);SelectObject(mem,old);DeleteObject(bmp);DeleteDC(mem);ReleaseDC(w,dc);
}
void settingsComboPaint(HWND w,HDC dc){
 RECT r;GetClientRect(w,&r);fill(dc,r,card);SetDCBrushColor(dc,border);FrameRect(dc,&r,(HBRUSH)GetStockObject(DC_BRUSH));int index=int(SendMessageW(w,CB_GETCURSEL,0,0));std::wstring text;if(index>=0){int n=int(SendMessageW(w,CB_GETLBTEXTLEN,index,0));if(n>=0){text.resize(n+1);SendMessageW(w,CB_GETLBTEXT,index,(LPARAM)text.data());text.resize(n);}}RECT t=r;t.left+=10;t.right-=28;label(dc,t,text,IsWindowEnabled(w)?ink:muted);int x=r.right-14,y=r.bottom/2;auto pen=CreatePen(PS_SOLID,1,muted);auto old=SelectObject(dc,pen);MoveToEx(dc,x-3,y-2,nullptr);LineTo(dc,x,y+1);LineTo(dc,x+3,y-2);SelectObject(dc,old);DeleteObject(pen);if(GetFocus()==w){InflateRect(&r,-3,-3);DrawFocusRect(dc,&r);}
}
LRESULT CALLBACK SettingsComboProc(HWND w,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR){
 if(msg==WM_PAINT){PAINTSTRUCT ps;auto dc=BeginPaint(w,&ps);settingsComboPaint(w,dc);EndPaint(w,&ps);return 0;}
 if(msg==WM_PRINTCLIENT){settingsComboPaint(w,(HDC)wp);return 0;}
 auto result=DefSubclassProc(w,msg,wp,lp);if(msg==CB_SETCURSEL||msg==WM_SETFOCUS||msg==WM_KILLFOCUS||msg==WM_KEYDOWN||msg==WM_LBUTTONUP)InvalidateRect(w,nullptr,FALSE);return result;
}
LRESULT CALLBACK SettingsSkinProc(HWND w,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR){
 if(msg==WM_ERASEBKGND){RECT r;GetClientRect(w,&r);fill((HDC)wp,r,bg);return 1;}
 if(msg==WM_CTLCOLORSTATIC||msg==WM_CTLCOLORBTN||msg==WM_CTLCOLOREDIT||msg==WM_CTLCOLORLISTBOX){auto dc=(HDC)wp;bool field=msg==WM_CTLCOLOREDIT||msg==WM_CTLCOLORLISTBOX;SetTextColor(dc,IsWindowEnabled((HWND)lp)?ink:muted);SetBkColor(dc,field?card:bg);SetDCBrushColor(dc,field?card:bg);return (LRESULT)GetStockObject(DC_BRUSH);}
 if(msg==WM_DRAWITEM){auto d=(DRAWITEMSTRUCT*)lp;if(d&&d->CtlType==ODT_COMBOBOX){fill(d->hDC,d->rcItem,(d->itemState&ODS_SELECTED)?activeBg:card);if(d->itemID!=UINT(-1)){int n=int(SendMessageW(d->hwndItem,CB_GETLBTEXTLEN,d->itemID,0));if(n>=0){std::wstring text(n+1,0);SendMessageW(d->hwndItem,CB_GETLBTEXT,d->itemID,(LPARAM)text.data());text.resize(n);RECT r=d->rcItem;r.left+=8;label(d->hDC,r,text,ink);}}return TRUE;}if(d&&d->CtlType==ODT_BUTTON){RECT r=d->rcItem;fill(d->hDC,r,(d->itemState&ODS_SELECTED)?activeBg:card);SetDCBrushColor(d->hDC,border);FrameRect(d->hDC,&r,(HBRUSH)GetStockObject(DC_BRUSH));wchar_t text[512]{};GetWindowTextW(d->hwndItem,text,512);label(d->hDC,r,text,(d->itemState&ODS_DISABLED)?muted:ink,font,DT_CENTER|DT_VCENTER|DT_SINGLELINE);if(d->itemState&ODS_FOCUS){InflateRect(&r,-3,-3);DrawFocusRect(d->hDC,&r);}return TRUE;}}
 return DefSubclassProc(w,msg,wp,lp);
}
void skinSettings(HWND w){
 SetWindowSubclass(w,SettingsSkinProc,73,0);
 HMODULE theme=LoadLibraryW(L"uxtheme.dll");using SetTheme=HRESULT(WINAPI*)(HWND,LPCWSTR,LPCWSTR);auto set=theme?(SetTheme)GetProcAddress(theme,"SetWindowTheme"):nullptr;
 for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t cls[32]{};GetClassNameW(c,cls,32);if(set)set(c,lightTheme?L"Explorer":L"DarkMode_Explorer",nullptr);if(wcscmp(cls,L"Button")==0){auto style=GetWindowLongW(c,GWL_STYLE);int type=style&BS_TYPEMASK;if(type==BS_PUSHBUTTON||type==BS_DEFPUSHBUTTON)SetWindowLongW(c,GWL_STYLE,(style&~BS_TYPEMASK)|BS_OWNERDRAW);}}
 for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t cls[32]{};GetClassNameW(c,cls,32);if(wcscmp(cls,L"ComboBox")==0){SetWindowLongW(c,GWL_STYLE,GetWindowLongW(c,GWL_STYLE)|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS);SendMessageW(c,CB_SETITEMHEIGHT,0,24);SetWindowSubclass(c,SettingsComboProc,74,0);}if(wcscmp(cls,L"Button")==0){int type=GetWindowLongW(c,GWL_STYLE)&BS_TYPEMASK;if(type==BS_AUTOCHECKBOX||type==BS_CHECKBOX)SetWindowSubclass(c,SettingsCheckProc,75,0);}}
 if(theme)FreeLibrary(theme);
 if(HMODULE dwm=LoadLibraryW(L"dwmapi.dll")){using Set=HRESULT(WINAPI*)(HWND,DWORD,LPCVOID,DWORD);auto setDwm=(Set)GetProcAddress(dwm,"DwmSetWindowAttribute");BOOL dark=!lightTheme;if(setDwm){if(FAILED(setDwm(w,20,&dark,sizeof(dark))))setDwm(w,19,&dark,sizeof(dark));}FreeLibrary(dwm);}
 refreshCaptionTheme(w);RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
}
void settingsTip(HWND w,HWND control,const wchar_t* text){
 auto tips=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP,0,0,0,0,w,nullptr,GetModuleHandleW(nullptr),nullptr);SendMessageW(tips,TTM_SETMAXTIPWIDTH,0,420);TOOLINFOW t{};t.cbSize=sizeof(t);t.uFlags=TTF_IDISHWND|TTF_SUBCLASS;t.hwnd=w;t.uId=(UINT_PTR)control;t.lpszText=const_cast<wchar_t*>(text);SendMessageW(tips,TTM_ADDTOOLW,0,(LPARAM)&t);
}
