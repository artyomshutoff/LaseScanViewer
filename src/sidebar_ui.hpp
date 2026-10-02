#pragma once
int sidebarScroll=0;
constexpr int sidebarHeight=952;
bool sidebarLayer(int id){return id==GRID||(id>=LAYER_FULL&&id<=LAYER_TARP);}
bool sidebarLayerDraw(HDC dc,RECT r,int id,bool focus){
 if(!sidebarLayer(id))return false;auto w=mainControl(id);bool enabled=IsWindowEnabled(w),checked=SendMessageW(w,BM_GETCHECK,0,0)==BST_CHECKED;
 fill(dc,r,panel);if(enabled&&GetPropW(w,L"ScanHover"))roundBox(dc,r,hoverBg,hoverBg);
 int y=(r.top+r.bottom)/2;roundBox(dc,{r.left+2,y-8,r.left+18,y+8},checked?activeBg:card,enabled&&checked?accent:border);
 if(checked){SelectObject(dc,GetStockObject(DC_PEN));SetDCPenColor(dc,enabled?accent:muted);MoveToEx(dc,r.left+5,y,nullptr);LineTo(dc,r.left+9,y+4);LineTo(dc,r.left+16,y-4);}
 wchar_t text[180]{};GetWindowTextW(w,text,180);label(dc,{r.left+27,r.top,r.right-3,r.bottom},text,enabled?ink:muted,smallFont);
 if(focus){InflateRect(&r,-1,-1);DrawFocusRect(dc,&r);}return true;
}
void sidebarSync(){
 if(!sidebarWin)return;auto s=currentScanLayers();
 for(auto [id,checked]:std::initializer_list<std::pair<int,bool>>{{LAYER_FULL,s.full},{LAYER_EMPTY,s.empty},{LAYER_CARGO,s.cargo},{LAYER_OTHER,s.other},{LAYER_WATER,s.emptyWater},{LAYER_LOADED_WATER,s.fullWater},{LAYER_TARP,s.tarp},{GRID,showZone}}){
  auto w=mainControl(id);SendMessageW(w,BM_SETCHECK,checked?BST_CHECKED:BST_UNCHECKED,0);bool available=id==LAYER_EMPTY?baseDocument>=0:id==LAYER_CARGO||id==LAYER_OTHER?comparison.has_value():id==LAYER_WATER?water.available:id==LAYER_LOADED_WATER?fullWater.available:id==LAYER_TARP?tarp.available:!model.points.empty();EnableWindow(w,!busy&&available);InvalidateRect(w,nullptr,FALSE);
 }
 EnableWindow(mainControl(OPEN_EMPTY),!busy);EnableWindow(mainControl(OPEN),!busy);
}
void sidebarPosition(){
 if(!sidebarWin)return;RECT r;GetClientRect(sidebarWin,&r);sidebarScroll=std::clamp(sidebarScroll,0,std::max(0,sidebarHeight-int(r.bottom)));
 SCROLLINFO info{sizeof(info),SIF_RANGE|SIF_PAGE|SIF_POS,0,sidebarHeight-1,UINT(r.bottom),sidebarScroll,0};SetScrollInfo(sidebarWin,SB_VERT,&info,TRUE);
 for(HWND w=GetWindow(sidebarWin,GW_CHILD);w;w=GetWindow(w,GW_HWNDNEXT)){int y=int((INT_PTR)GetPropW(w,L"SidebarY"))-1;if(y<0)continue;RECT b;GetWindowRect(w,&b);MapWindowPoints(nullptr,sidebarWin,(POINT*)&b,2);MoveWindow(w,b.left,y-sidebarScroll,b.right-b.left,b.bottom-b.top,TRUE);}InvalidateRect(sidebarWin,nullptr,TRUE);
}
void sidebarLayout(){if(!sidebarWin)return;RECT r;GetClientRect(mainWin,&r);MoveWindow(sidebarWin,0,0,300,std::max(1L,r.bottom-40),TRUE);sidebarPosition();}
void sidebarEnsureVisible(HWND w){
 if(!sidebarWin||GetParent(w)!=sidebarWin)return;RECT r,b;GetClientRect(sidebarWin,&r);GetWindowRect(w,&b);MapWindowPoints(nullptr,sidebarWin,(POINT*)&b,2);if(b.top<8)sidebarScroll+=b.top-8;else if(b.bottom>r.bottom-8)sidebarScroll+=b.bottom-r.bottom+8;else return;sidebarPosition();
}
void sidebarPaint(HDC dc){
 RECT r;GetClientRect(sidebarWin,&r);fill(dc,r,panel);int saved=SaveDC(dc);SetViewportOrgEx(dc,0,-sidebarScroll,nullptr);
 drawBrandLogo(dc,{20,16,174,54});label(dc,{20,58,280,87},L"ScanViewer",ink,numberFont);label(dc,{20,86,280,103},L"Версия 3.34.0 · настольный просмотр",muted,smallFont);
 for(auto [y,text]:std::initializer_list<std::pair<int,const wchar_t*>>{{204,L"РАКУРС"},{322,L"СЛОИ ПРОСМОТРА"},{590,L"ОТОБРАЖЕНИЕ"}}){fill(dc,{20,y,280,y+1},border);label(dc,{20,y+6,280,y+25},text,muted,smallFont);}
 label(dc,{20,704,192,734},L"Размер точек · "+std::to_wstring(int(pointSize))+L" px",muted,smallFont);
 label(dc,{20,876,280,896},controlDatabase.path.empty()?L"Контрольная база не загружена":L"SQ3 · "+std::to_wstring(controlDatabase.ids.size())+L" записей",muted,smallFont);RestoreDC(dc,saved);
}
void sidebarCapture(HDC dc){
 if(!sidebarWin)return;RECT r;GetClientRect(sidebarWin,&r);int saved=SaveDC(dc);IntersectClipRect(dc,0,0,r.right,r.bottom);sidebarPaint(dc);
 for(HWND w=GetWindow(sidebarWin,GW_CHILD);w;w=GetWindow(w,GW_HWNDNEXT)){if(!(GetWindowLongW(w,GWL_STYLE)&WS_VISIBLE))continue;RECT b;GetWindowRect(w,&b);MapWindowPoints(nullptr,mainWin,(POINT*)&b,2);drawButton(dc,b,GetDlgCtrlID(w),false,GetFocus()==w);}RestoreDC(dc,saved);
}
LRESULT CALLBACK LayerSkinProc(HWND w,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR){
 auto result=DefSubclassProc(w,msg,wp,lp);if(msg==WM_PAINT){auto dc=GetDC(w);RECT r;GetClientRect(w,&r);sidebarLayerDraw(dc,r,GetDlgCtrlID(w),GetFocus()==w);ReleaseDC(w,dc);}
 if(msg==WM_SETFOCUS){sidebarEnsureVisible(w);InvalidateRect(w,nullptr,FALSE);}if(msg==WM_KILLFOCUS||msg==WM_ENABLE)InvalidateRect(w,nullptr,FALSE);
 if(msg==WM_NCDESTROY)RemoveWindowSubclass(w,LayerSkinProc,id);return result;
}
LRESULT CALLBACK SidebarProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 switch(msg){
 case WM_COMMAND:case WM_DRAWITEM:case WM_MEASUREITEM:return SendMessageW(mainWin,msg,wp,lp);
 case WM_SIZE:sidebarPosition();return 0;
 case WM_PAINT:{PAINTSTRUCT ps;auto dc=BeginPaint(w,&ps);sidebarPaint(dc);EndPaint(w,&ps);return 0;}
 case WM_ERASEBKGND:return 1;
 case WM_VSCROLL:case WM_MOUSEWHEEL:{if(msg==WM_MOUSEWHEEL)sidebarScroll-=GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA*84;else{SCROLLINFO info{};info.cbSize=sizeof(info);info.fMask=SIF_ALL;GetScrollInfo(w,SB_VERT,&info);switch(LOWORD(wp)){case SB_LINEUP:sidebarScroll-=28;break;case SB_LINEDOWN:sidebarScroll+=28;break;case SB_PAGEUP:sidebarScroll-=int(info.nPage);break;case SB_PAGEDOWN:sidebarScroll+=int(info.nPage);break;case SB_THUMBTRACK:sidebarScroll=info.nTrackPos;break;case SB_TOP:sidebarScroll=0;break;case SB_BOTTOM:sidebarScroll=sidebarHeight;break;}}sidebarPosition();return 0;}
 }return DefWindowProcW(w,msg,wp,lp);
}
void createSidebar(HINSTANCE inst){WNDCLASSW wc{};wc.hInstance=inst;wc.lpfnWndProc=SidebarProc;wc.lpszClassName=L"LaseSidebar";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&wc);sidebarWin=CreateWindowExW(WS_EX_CONTROLPARENT,L"LaseSidebar",L"Просмотр и слои",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_VSCROLL,0,0,300,850,mainWin,nullptr,inst,nullptr);}
void createSidebarLayers(HINSTANCE inst){
 int y=350;for(auto [id,text]:std::initializer_list<std::pair<int,const wchar_t*>>{{LAYER_FULL,L"Полный скан"},{LAYER_EMPTY,L"Пустой кузов"},{LAYER_CARGO,L"Груз цветом"},{LAYER_OTHER,L"Остальные точки серым"},{LAYER_WATER,L"По воде · пустой кузов"},{LAYER_LOADED_WATER,L"По воде · над грузом"},{LAYER_TARP,L"Тент · выше бортов"},{GRID,L"Зона интереса · X, Y, Z"}}){auto w=CreateWindowW(L"BUTTON",text,WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,20,y,260,28,sidebarWin,(HMENU)(INT_PTR)id,inst,nullptr);SendMessageW(w,WM_SETFONT,(WPARAM)smallFont,TRUE);SetPropW(w,L"SidebarY",(HANDLE)(INT_PTR)(y+1));SetWindowSubclass(w,ButtonSkinProc,1,0);SetWindowSubclass(w,LayerSkinProc,2,0);y+=29;}
}
void openEmptyFile(){
 if(busy)return;if(documents.size()+loadQueue.size()>=12){status(L"Можно открыть до 12 файлов. Закройте лишний скан перед выбором базы.");return;}wchar_t path[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=mainWin;o.lpstrFilter=L"LASE BIN (*.bin)\0*.bin\0";o.lpstrFile=path;o.nMaxFile=32768;o.lpstrTitle=L"Открыть пустой кузов: Empty / Reference";o.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_EXPLORER;
 if(GetOpenFileNameW(&o)){pendingBaseOnly=true;enqueueFiles({path});}
}
bool sidebarCommand(int id){
 if(id==OPEN_EMPTY){openEmptyFile();return true;}if(!sidebarLayer(id))return false;if(busy)return true;
 if(id==GRID){showZone=!showZone;redraw();return true;}auto s=currentScanLayers();bool* value=id==LAYER_FULL?&s.full:id==LAYER_EMPTY?&s.empty:id==LAYER_CARGO?&s.cargo:id==LAYER_OTHER?&s.other:id==LAYER_WATER?&s.emptyWater:id==LAYER_LOADED_WATER?&s.fullWater:&s.tarp;*value=!*value;applyScanLayers(s);return true;
}
void sidebarTests(){
 if(!sidebarWin||GetParent(mainControl(OPEN))!=sidebarWin||GetParent(mainControl(OPEN_EMPTY))!=sidebarWin)throw std::runtime_error("Sidebar file controls missing");
 for(int id:{OPEN,OPEN_EMPTY,MANUAL_CALC,WEB_VIEW}){wchar_t text[100]{};auto w=mainControl(id);GetWindowTextW(w,text,100);HDC dc=GetDC(w);auto old=SelectObject(dc,font);SIZE size{};GetTextExtentPoint32W(dc,text,int(wcslen(text)),&size);SelectObject(dc,old);ReleaseDC(w,dc);RECT r;GetClientRect(w,&r);if(size.cx>r.right-16)throw std::runtime_error("Sidebar button text clipped");}
 RECT original;GetWindowRect(mainWin,&original);SetWindowPos(mainWin,nullptr,0,0,1120,820,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
 for(int id:{OPEN,OPEN_EMPTY,MANUAL_CALC,WEB_VIEW,EXPORT}){sidebarEnsureVisible(mainControl(id));RECT r,b;GetClientRect(sidebarWin,&r);GetWindowRect(mainControl(id),&b);MapWindowPoints(nullptr,sidebarWin,(POINT*)&b,2);if(b.top<0||b.bottom>r.bottom)throw std::runtime_error("Sidebar scrolling clipped a focused button");}
 saveInterface(renderTestDir/L"sidebar-small.bmp");SetWindowPos(mainWin,nullptr,0,0,original.right-original.left,original.bottom-original.top,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);sidebarScroll=0;sidebarPosition();
 std::ofstream(renderTestDir/L"sidebar-ok.txt")<<"PASS separate Full/Empty buttons, full labels and scrolling at 1120x820";
}
