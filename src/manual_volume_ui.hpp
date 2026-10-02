#pragma once
#include "manual_volume.hpp"

std::vector<manualVolume::Row> manualMeasurements(5);
bool manualCentimetres=false;
struct ManualDialog {
 HWND window=nullptr,table=nullptr;bool done=false,rebuilding=false;int scroll=0,test=0;bool centimetres=false;
 std::array<HWND,3> headings{};
 int lineHeight=24,rowHeight=40,tableY=120,tableHeight=200,statsY=380,dimensionsY=474,compareY=510,compareHeight=116,footerY=640,clientHeight=692;
 std::wstring summary;
 std::vector<manualVolume::Row> rows;
 struct Widgets {HWND number=nullptr,volume=nullptr,remove=nullptr;std::array<HWND,3> edits{};};
 std::vector<Widgets> widgets;manualVolume::Result result;
};
constexpr int MANUAL_ADD=710,MANUAL_RESULTS=711,MANUAL_STATUS=712,MANUAL_UNITS=713,MANUAL_INFO=714;
HWND manualControl(HWND parent,const wchar_t* cls,const std::wstring& text,int id,int x,int y,int width,int height,DWORD style=0){
 auto w=CreateWindowW(cls,text.c_str(),WS_CHILD|WS_VISIBLE|style,x,y,width,height,parent,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);
 SendMessageW(w,WM_SETFONT,(WPARAM)font,TRUE);return w;
}
LRESULT manualColors(UINT msg,WPARAM wp){
 SetTextColor((HDC)wp,ink);SetBkColor((HDC)wp,(msg==WM_CTLCOLOREDIT||msg==WM_CTLCOLORLISTBOX)?card:bg);
 SetDCBrushColor((HDC)wp,(msg==WM_CTLCOLOREDIT||msg==WM_CTLCOLORLISTBOX)?card:bg);return (LRESULT)GetStockObject(DC_BRUSH);
}
void manualDrawButton(DRAWITEMSTRUCT* item){
 bool enabled=IsWindowEnabled(item->hwndItem);auto r=item->rcItem;
 if(item->CtlType==ODT_COMBOBOX){fill(item->hDC,r,(item->itemState&ODS_SELECTED)?activeBg:card);wchar_t text[80]{};if(item->itemID!=UINT(-1))SendMessageW(item->hwndItem,CB_GETLBTEXT,item->itemID,(LPARAM)text);r.left+=10;label(item->hDC,r,text,ink,font);return;}
 roundBox(item->hDC,r,(item->itemState&ODS_SELECTED)?activeBg:card,border);
 wchar_t text[80]{};GetWindowTextW(item->hwndItem,text,80);
 label(item->hDC,r,text,enabled?ink:muted,font,DT_CENTER|DT_SINGLELINE|DT_VCENTER);
 if(item->itemState&ODS_FOCUS){InflateRect(&r,-3,-3);DrawFocusRect(item->hDC,&r);}
}
void manualLayoutRows(ManualDialog& d){
 RECT r;GetClientRect(d.table,&r);int total=int(d.widgets.size())*d.rowHeight;
 d.scroll=std::clamp(d.scroll,0,std::max(0,total-int(r.bottom)));
 SCROLLINFO info{sizeof(info),SIF_RANGE|SIF_PAGE|SIF_POS,0,std::max(0,total-1),UINT(r.bottom),d.scroll,0};SetScrollInfo(d.table,SB_VERT,&info,TRUE);
 for(size_t i=0;i<d.widgets.size();++i){auto& row=d.widgets[i];int y=int(i)*d.rowHeight-d.scroll;
  MoveWindow(row.number,8,y+3,34,d.rowHeight-8,TRUE);
  for(int j=0;j<3;++j)MoveWindow(row.edits[j],50+j*154,y+2,140,d.rowHeight-8,TRUE);
  MoveWindow(row.volume,516,y+3,128,d.rowHeight-8,TRUE);MoveWindow(row.remove,652,y+2,64,d.rowHeight-8,TRUE);
 }
 InvalidateRect(d.table,nullptr,TRUE);
}
void manualUpdate(ManualDialog& d){
 if(d.rebuilding)return;
 for(size_t i=0;i<d.widgets.size();++i){auto& row=d.widgets[i];
  for(int j=0;j<3;++j){wchar_t text[128]{};GetWindowTextW(row.edits[j],text,128);d.rows[i][j]=text;}
  auto value=manualVolume::calculate({d.rows[i]},d.centimetres?.01:1);SetWindowTextW(row.volume,value.count&&value.error.empty()?(num(value.mean,3)+L" м³").c_str():L"—");
 }
 d.result=manualVolume::calculate(d.rows,d.centimetres?.01:1);manualMeasurements=d.rows;
 std::wstring text,summary;
 if(!d.result.error.empty()){
  text=L"Строка "+std::to_wstring(d.result.invalidRow)+L": введите три положительных размера.";
  summary=L"Проверьте ввод. Пустую строку можно заполнить или удалить.";
 }else if(!d.result.count){text=L"Введите длину, ширину и высоту хотя бы в одной строке.";summary=L"Средний объём: —     Минимальный: —     Максимальный: —";}
 else{
  auto& v=d.result;text=L"Заполнено "+std::to_wstring(v.count)+L" из "+std::to_wstring(d.rows.size())+L" · пустые строки не учитываются";
  summary=L"Средние размеры (Д × Ш × В): "+num(v.dimensions[0],3)+L" × "+num(v.dimensions[1],3)+L" × "+num(v.dimensions[2],3)+L" м\n";
  auto scan=displayedVolume();
  if(scan){auto percent=manualVolume::differencePercent(*scan,v.mean);
   summary+=L"По скану: "+num(*scan,3)+L" м³"+(comparison->options.provisional?L" (предварительно)":L"");
   if(percent)summary+=L"\nСкан − обмеры: "+std::wstring(*scan>=v.mean?L"+":L"")+num(*scan-v.mean,3)+L" м³; "+std::wstring(*percent>=0?L"+":L"")+num(*percent,2)+L"% (абсолютно "+num(std::abs(*percent),2)+L"%)";
  }else summary+=L"Сравнение со сканом: сначала рассчитайте объём и задайте единицы.";
 }
 SetWindowTextW(GetDlgItem(d.window,MANUAL_STATUS),text.c_str());
 EnableWindow(GetDlgItem(d.window,MANUAL_ADD),d.rows.size()<200);
 for(auto& row:d.widgets)EnableWindow(row.remove,d.rows.size()>1);
 d.summary=summary;RECT cards{20,d.statsY,750,d.footerY};InvalidateRect(d.window,&cards,TRUE);
}
void manualRebuild(ManualDialog& d){
 d.rebuilding=true;
 for(auto& row:d.widgets){DestroyWindow(row.number);DestroyWindow(row.volume);DestroyWindow(row.remove);for(auto edit:row.edits)DestroyWindow(edit);}
 d.widgets.clear();
 for(size_t i=0;i<d.rows.size();++i){ManualDialog::Widgets row;
  row.number=manualControl(d.table,L"STATIC",std::to_wstring(i+1),0,0,0,34,30);
  for(int j=0;j<3;++j){row.edits[j]=manualControl(d.table,L"EDIT",d.rows[i][j],1000+int(i)*3+j,0,0,140,30,WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL);SendMessageW(row.edits[j],EM_SETLIMITTEXT,64,0);}
  row.volume=manualControl(d.table,L"STATIC",L"—",0,0,0,128,30);
  row.remove=manualControl(d.table,L"BUTTON",L"Удалить",2000+int(i),0,0,64,30,BS_OWNERDRAW|WS_TABSTOP);
  d.widgets.push_back(row);
 }
 d.rebuilding=false;manualLayoutRows(d);manualUpdate(d);
}
LRESULT CALLBACK ManualTableProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 auto d=(ManualDialog*)GetWindowLongPtrW(w,GWLP_USERDATA);
 if(msg==WM_NCCREATE){d=(ManualDialog*)((CREATESTRUCTW*)lp)->lpCreateParams;d->table=w;SetWindowLongPtrW(w,GWLP_USERDATA,(LONG_PTR)d);}
 if(d){
  if(msg==WM_COMMAND){int id=LOWORD(wp);
   if(HIWORD(wp)==EN_CHANGE){manualUpdate(*d);return 0;}
   if(HIWORD(wp)==EN_SETFOCUS&&!d->rebuilding){int index=(id-1000)/3;RECT r;GetClientRect(w,&r);if(index>=0&&index<int(d->rows.size())){int top=index*d->rowHeight;if(top<d->scroll)d->scroll=top;else if(top+d->rowHeight>d->scroll+r.bottom)d->scroll=top+d->rowHeight-r.bottom;manualLayoutRows(*d);}return 0;}
   if(id>=2000&&id<2200&&HIWORD(wp)==BN_CLICKED&&d->rows.size()>1){size_t index=size_t(id-2000);if(index<d->rows.size()){manualUpdate(*d);d->rows.erase(d->rows.begin()+index);manualRebuild(*d);SetFocus(d->widgets[std::min(index,d->widgets.size()-1)].edits[0]);}return 0;}
  }
  if(msg==WM_VSCROLL||msg==WM_MOUSEWHEEL){
   if(msg==WM_MOUSEWHEEL)d->scroll-=GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA*d->rowHeight*3;
   else{SCROLLINFO info{};info.cbSize=sizeof(info);info.fMask=SIF_ALL;GetScrollInfo(w,SB_VERT,&info);switch(LOWORD(wp)){case SB_LINEUP:d->scroll-=d->rowHeight;break;case SB_LINEDOWN:d->scroll+=d->rowHeight;break;case SB_PAGEUP:d->scroll-=int(info.nPage);break;case SB_PAGEDOWN:d->scroll+=int(info.nPage);break;case SB_THUMBTRACK:d->scroll=info.nTrackPos;break;case SB_TOP:d->scroll=0;break;case SB_BOTTOM:d->scroll=info.nMax;break;}}
   manualLayoutRows(*d);return 0;
  }
  if(msg==WM_SIZE){manualLayoutRows(*d);return 0;}
  if(msg==WM_CTLCOLOREDIT||msg==WM_CTLCOLORSTATIC||msg==WM_CTLCOLORLISTBOX)return manualColors(msg,wp);
  if(msg==WM_DRAWITEM){manualDrawButton((DRAWITEMSTRUCT*)lp);return TRUE;}
  if(msg==WM_PRINTCLIENT){RECT r;GetClientRect(w,&r);fill((HDC)wp,r,bg);return 0;}
  if(msg==WM_ERASEBKGND){RECT r;GetClientRect(w,&r);fill((HDC)wp,r,bg);return 1;}
 }
 return DefWindowProcW(w,msg,wp,lp);
}
void manualChangeUnits(ManualDialog& d,bool centimetres){
 manualUpdate(d);if(centimetres==d.centimetres)return;
 // Partial rows are allowed. Refuse the switch only for nonempty invalid fields.
 auto rows=d.rows;
 for(auto& row:rows)for(auto& text:row)if(!manualVolume::blank(text)){
  auto value=manualVolume::dimension(text);if(!value){SendDlgItemMessageW(d.window,MANUAL_UNITS,CB_SETCURSEL,d.centimetres,0);MessageBoxW(d.window,L"Исправьте некорректный размер перед сменой единиц.",L"Единицы измерения",MB_ICONINFORMATION);return;}
  auto converted=manualVolume::convertDimension(text,centimetres);if(!converted){SendDlgItemMessageW(d.window,MANUAL_UNITS,CB_SETCURSEL,d.centimetres,0);MessageBoxW(d.window,L"Размер после перевода выходит за допустимый числовой диапазон.",L"Единицы измерения",MB_ICONINFORMATION);return;}
  text=*converted;
 }
 d.centimetres=centimetres;manualCentimetres=centimetres;d.rows=std::move(rows);
 const wchar_t* names[]={L"Длина",L"Ширина",L"Высота"};for(int j=0;j<3;++j)SetWindowTextW(d.headings[j],(std::wstring(names[j])+(centimetres?L", см":L", м")).c_str());
 manualRebuild(d);
}
void manualPaintCards(ManualDialog& d,HDC dc){
 const bool valid=d.result.count&&d.result.error.empty();int line=d.lineHeight;
 label(dc,{20,16,500,48},L"Обмеры кузова",ink,numberFont);
 label(dc,{20,54,530,54+line},L"Введите размеры в пяти точках или добавьте свои",muted,smallFont);
 label(dc,{550,16,750,38},L"ЕДИНИЦЫ ИЗМЕРЕНИЯ",muted,smallFont);
 const wchar_t* titles[]={L"СРЕДНИЙ ОБЪЁМ",L"МИНИМАЛЬНЫЙ",L"МАКСИМАЛЬНЫЙ"};double values[]={d.result.mean,d.result.minimum,d.result.maximum};int bottom=d.statsY+2*line+36;
 for(int i=0;i<3;++i){int x=20+i*248;roundBox(dc,{x,d.statsY,x+234,bottom},i==0?activeBg:card,border);label(dc,{x+14,d.statsY+10,x+220,d.statsY+10+line},titles[i],muted,smallFont);label(dc,{x+14,d.statsY+line+16,x+220,bottom-10},valid?num(values[i],3)+L" м³":L"—",i==0?accent:ink,numberFont);}
 std::wstring dimensions=valid?L"Средние размеры: "+num(d.result.dimensions[0],3)+L" × "+num(d.result.dimensions[1],3)+L" × "+num(d.result.dimensions[2],3)+L" м":L"Средние размеры: —";
 label(dc,{20,d.dimensionsY,750,d.dimensionsY+line},dimensions,muted,font);
 int y=d.compareY;roundBox(dc,{20,y,750,y+d.compareHeight},card,border);
 label(dc,{36,y+10,540,y+10+line},L"Сравнение со сканом",ink,font);
 auto scan=displayedVolume();
 if(scan&&comparison&&comparison->options.provisional)label(dc,{535,y+10,734,y+10+line},L"Предварительная оценка",muted,smallFont,DT_RIGHT|DT_SINGLELINE|DT_VCENTER);
 if(valid&&scan){
  label(dc,{36,y+line+14,330,y+2*line+12},L"ОБЪЁМ ПО СКАНУ",muted,smallFont);
  label(dc,{36,y+2*line+12,330,y+d.compareHeight-10},num(*scan,3)+L" м³",ink,numberFont);
  label(dc,{358,y+line+14,734,y+2*line+12},L"СКАН − СРЕДНИЙ ОБЪЁМ ОБМЕРОВ",muted,smallFont);
  auto percent=manualVolume::differencePercent(*scan,d.result.mean);double delta=*scan-d.result.mean;
  std::wstring diff=(delta>=0?L"+":L"")+num(delta,3)+L" м³";
  if(percent)diff+=L"   /   "+std::wstring(*percent>=0?L"+":L"")+num(*percent,2)+L"%";
  label(dc,{358,y+2*line+12,734,y+3*line+12},diff,accent,font);
  label(dc,{358,y+3*line+12,734,y+d.compareHeight-6},percent?L"Абсолютное расхождение: "+num(std::abs(*percent),2)+L"%":L"Процент не определён",muted,smallFont);
 }else{
  auto message=!d.result.error.empty()?L"Исправьте отмеченную строку, чтобы продолжить расчёт.":!valid?L"Заполните обмеры для сравнения с рассчитанным объёмом.":L"Рассчитайте объём скана и задайте масштаб координат.";
  label(dc,{36,y+line+20,734,y+d.compareHeight-12},message,muted,font);
 }
 label(dc,{166,d.footerY,602,d.footerY+42},L"Обмеры: прямоугольный кузов.\nПо скану: объём груза, а не вместимость кузова.",muted,smallFont,DT_LEFT|DT_WORDBREAK);
}
void manualMeasureLayout(ManualDialog& d){
 HDC dc=GetDC(mainWin);auto previous=SelectObject(dc,font);TEXTMETRICW metric{};GetTextMetricsW(dc,&metric);SelectObject(dc,previous);ReleaseDC(mainWin,dc);
 d.lineHeight=std::max(24,int(metric.tmHeight)+4);d.rowHeight=d.lineHeight+16;d.tableHeight=d.rowHeight*5;
 d.statsY=d.tableY+d.tableHeight+60;d.dimensionsY=d.statsY+2*d.lineHeight+46;
 d.compareY=d.dimensionsY+d.lineHeight+12;d.compareHeight=3*d.lineHeight+44;d.footerY=d.compareY+d.compareHeight+14;d.clientHeight=d.footerY+52;
}
void manualDialogSelfTest(ManualDialog& d);
LRESULT CALLBACK ManualProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 auto d=(ManualDialog*)GetWindowLongPtrW(w,GWLP_USERDATA);
 if(msg==WM_NCCREATE){d=(ManualDialog*)((CREATESTRUCTW*)lp)->lpCreateParams;SetWindowLongPtrW(w,GWLP_USERDATA,(LONG_PTR)d);}
 if(d){
  if(msg==WM_COMMAND){int id=LOWORD(wp);
   if(id==MANUAL_INFO){MessageBoxW(w,L"Объём обмера = длина × ширина × высота.\nСредний объём — среднее объёмов заполненных строк.\n\nРазница = объём по скану − средний объём обмеров.\nПроцент = разница / средний объём обмеров × 100.\n\nОбмеры оценивают прямоугольный кузов. Объём груза по скану может отличаться от его вместимости.",L"Как считается сравнение",MB_ICONINFORMATION);return 0;}
   if(id==MANUAL_UNITS&&HIWORD(wp)==CBN_SELCHANGE){manualChangeUnits(*d,SendMessageW(GetDlgItem(w,MANUAL_UNITS),CB_GETCURSEL,0,0)==1);return 0;}
   if(id==MANUAL_ADD&&d->rows.size()<200){manualUpdate(*d);d->rows.push_back({});manualRebuild(*d);d->scroll=int(d->rows.size())*d->rowHeight;manualLayoutRows(*d);SetFocus(d->widgets.back().edits[0]);return 0;}
   if(id==IDOK||id==IDCANCEL){manualUpdate(*d);DestroyWindow(w);return 0;}
  }
  if(msg==WM_MEASUREITEM){auto item=(MEASUREITEMSTRUCT*)lp;if(item->CtlType==ODT_COMBOBOX){item->itemHeight=d->lineHeight+6;return TRUE;}}
  if(msg==WM_PAINT){PAINTSTRUCT ps;auto dc=BeginPaint(w,&ps);manualPaintCards(*d,dc);EndPaint(w,&ps);return 0;}
  if(msg==WM_PRINTCLIENT){RECT r;GetClientRect(w,&r);fill((HDC)wp,r,bg);manualPaintCards(*d,(HDC)wp);return 0;}
  if(msg==WM_TIMER&&d->test){KillTimer(w,1);manualDialogSelfTest(*d);DestroyWindow(w);return 0;}
  if(msg==WM_CLOSE){manualUpdate(*d);DestroyWindow(w);return 0;}
  if(msg==WM_DESTROY){d->done=true;return 0;}
  if(msg==WM_CTLCOLOREDIT||msg==WM_CTLCOLORSTATIC||msg==WM_CTLCOLORLISTBOX)return manualColors(msg,wp);
  if(msg==WM_DRAWITEM){manualDrawButton((DRAWITEMSTRUCT*)lp);return TRUE;}
  if(msg==WM_ERASEBKGND){RECT r;GetClientRect(w,&r);fill((HDC)wp,r,bg);return 1;}
 }
 return DefWindowProcW(w,msg,wp,lp);
}
void manualCalculator(int test=0){
 if(busy)return;ManualDialog d;d.rows=manualMeasurements;d.test=test;d.centimetres=manualCentimetres;
 HINSTANCE inst=GetModuleHandleW(nullptr);WNDCLASSW wc{};wc.hInstance=inst;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.lpfnWndProc=ManualProc;wc.lpszClassName=L"LaseManualVolume";RegisterClassW(&wc);
 wc.lpfnWndProc=ManualTableProc;wc.lpszClassName=L"LaseManualVolumeTable";RegisterClassW(&wc);
 manualMeasureLayout(d);RECT parent;GetWindowRect(mainWin,&parent);RECT area{0,0,770,d.clientHeight};AdjustWindowRectEx(&area,WS_CAPTION|WS_SYSMENU,FALSE,WS_EX_DLGMODALFRAME);
 d.window=CreateWindowExW(WS_EX_DLGMODALFRAME|WS_EX_CONTROLPARENT,L"LaseManualVolume",L"Обмеры кузова · ручной калькулятор объёма",WS_CAPTION|WS_SYSMENU,parent.left+std::max(0L,(parent.right-parent.left-(area.right-area.left))/2),parent.top+std::max(0L,(parent.bottom-parent.top-(area.bottom-area.top))/2),area.right-area.left,area.bottom-area.top,mainWin,nullptr,inst,&d);
 if(!d.window)throw std::runtime_error("Cannot create manual calculator");
 if(HMODULE dwm=LoadLibraryW(L"dwmapi.dll")){using SetAttribute=HRESULT(WINAPI*)(HWND,DWORD,LPCVOID,DWORD);auto set=(SetAttribute)GetProcAddress(dwm,"DwmSetWindowAttribute");BOOL dark=!lightTheme;if(set){if(FAILED(set(d.window,20,&dark,sizeof(dark))))set(d.window,19,&dark,sizeof(dark));set(d.window,35,&panel,sizeof(panel));set(d.window,36,&ink,sizeof(ink));}FreeLibrary(dwm);}
 auto units=manualControl(d.window,L"COMBOBOX",L"",MANUAL_UNITS,550,42,200,150,CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS|WS_TABSTOP|WS_VSCROLL);
 SendMessageW(units,CB_SETITEMHEIGHT,WPARAM(-1),d.lineHeight+6);SendMessageW(units,CB_SETITEMHEIGHT,0,d.lineHeight+6);
 SendMessageW(units,CB_ADDSTRING,0,(LPARAM)L"Метры (м)");SendMessageW(units,CB_ADDSTRING,0,(LPARAM)L"Сантиметры (см)");SendMessageW(units,CB_SETCURSEL,d.centimetres,0);
 const wchar_t* titles[]={L"№",L"Длина, м",L"Ширина, м",L"Высота, м",L"Объём, м³"};int positions[]={28,70,224,378,536};
 for(int j=0;j<5;++j){auto h=manualControl(d.window,L"STATIC",titles[j],0,positions[j],d.tableY-d.lineHeight-4,j?140:34,d.lineHeight);if(j>=1&&j<=3){d.headings[j-1]=h;if(d.centimetres)SetWindowTextW(h,(std::wstring(titles[j]).substr(0,std::wstring(titles[j]).size()-1)+L"см").c_str());}}
 d.table=CreateWindowExW(WS_EX_CONTROLPARENT,L"LaseManualVolumeTable",L"",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_VSCROLL,20,d.tableY,738,d.tableHeight,d.window,nullptr,inst,&d);
 manualControl(d.window,L"BUTTON",L"+ Добавить обмер",MANUAL_ADD,20,d.tableY+d.tableHeight+12,190,36,BS_OWNERDRAW|WS_TABSTOP);
 auto status=manualControl(d.window,L"STATIC",L"",MANUAL_STATUS,230,d.tableY+d.tableHeight+18,520,d.lineHeight+4,SS_LEFTNOWORDWRAP);SendMessageW(status,WM_SETFONT,(WPARAM)smallFont,TRUE);
 manualControl(d.window,L"BUTTON",L"Как считается",MANUAL_INFO,20,d.footerY,128,36,BS_OWNERDRAW|WS_TABSTOP);
 manualControl(d.window,L"BUTTON",L"Закрыть",IDOK,626,d.footerY,124,36,BS_OWNERDRAW|WS_TABSTOP);
 manualRebuild(d);EnableWindow(mainWin,FALSE);ShowWindow(d.window,SW_SHOW);SetFocus(d.widgets.front().edits[0]);if(test)SetTimer(d.window,1,50,nullptr);
 MSG msg;while(!d.done&&GetMessageW(&msg,nullptr,0,0)>0)if(!IsDialogMessageW(d.window,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}
 EnableWindow(mainWin,TRUE);SetActiveWindow(mainWin);
}
void manualDialogSelfTest(ManualDialog& d){
 try{
  auto capture=[&](const wchar_t* name){RedrawWindow(d.window,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);RECT r;GetClientRect(d.window,&r);int width=r.right,height=r.bottom;HDC dc=GetDC(d.window),memory=CreateCompatibleDC(dc);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;void* bits=nullptr;auto bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);auto old=SelectObject(memory,bitmap);fill(memory,{0,0,width,height},bg);manualPaintCards(d,memory);
   for(HWND child=GetWindow(d.window,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){if(!IsWindowVisible(child))continue;RECT box;GetWindowRect(child,&box);MapWindowPoints(nullptr,d.window,(POINT*)&box,2);int saved=SaveDC(memory);IntersectClipRect(memory,box.left,box.top,box.right,box.bottom);SetViewportOrgEx(memory,box.left,box.top,nullptr);SendMessageW(child,WM_PRINT,(WPARAM)memory,PRF_CLIENT|PRF_NONCLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(memory,saved);}GdiFlush();std::vector<uint8_t> rgb(size_t(width)*height*3);auto data=(uint8_t*)bits;for(size_t i=0;i<size_t(width)*height;++i){rgb[i*3]=data[i*4+2];rgb[i*3+1]=data[i*4+1];rgb[i*3+2]=data[i*4];}SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(d.window,dc);writeBytes(renderTestDir/name,encodePng(width,height,rgb));};
  std::filesystem::create_directories(renderTestDir);
  d.centimetres=false;manualCentimetres=false;SendDlgItemMessageW(d.window,MANUAL_UNITS,CB_SETCURSEL,0,0);d.rows.assign(5,{});manualRebuild(d);
  for(size_t i=0;i<5;++i){SetWindowTextW(d.widgets[i].edits[0],L"5,0");SetWindowTextW(d.widgets[i].edits[1],L"2");SetWindowTextW(d.widgets[i].edits[2],(i%2?L"1,5":L"1"));}
  if(d.result.count!=5||d.result.mean!=12||d.result.minimum!=10||d.result.maximum!=15)throw std::runtime_error("Manual aggregate failed");
  if(auto scan=displayedVolume()){if(d.summary.find(num(*scan,3))==std::wstring::npos)throw std::runtime_error("Scan comparison missing");}
  capture(L"manual-calculator.png");
  auto savedRows=d.rows;d.rows={{L"5.05",L"2.31",L"1.36"},{L"5.02",L"2.3",L"1.35"},{},{},{}};manualRebuild(d);capture(L"manual-partial.png");
  if(std::abs(d.result.mean-15.72609)>1e-6)throw std::runtime_error("Partial measurements changed volume");
  bool partialTheme=lightTheme;applyTheme(true);RedrawWindow(d.window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);capture(L"manual-partial-light.png");applyTheme(partialTheme);d.rows=savedRows;manualRebuild(d);
  SendDlgItemMessageW(d.window,MANUAL_UNITS,CB_SETCURSEL,1,0);SendMessageW(d.window,WM_COMMAND,MAKEWPARAM(MANUAL_UNITS,CBN_SELCHANGE),0);
  if(!d.centimetres||std::abs(d.result.mean-12)>1e-10||manualVolume::dimension(d.rows[0][0])!=500)throw std::runtime_error("Centimetre conversion failed");
  capture(L"manual-centimetres.png");bool themeBefore=lightTheme;applyTheme(true);RedrawWindow(d.window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);capture(L"manual-light.png");applyTheme(themeBefore);
  SendDlgItemMessageW(d.window,MANUAL_UNITS,CB_SETCURSEL,0,0);SendMessageW(d.window,WM_COMMAND,MAKEWPARAM(MANUAL_UNITS,CBN_SELCHANGE),0);
  if(d.centimetres||std::abs(d.result.mean-12)>1e-10||manualVolume::dimension(d.rows[0][0])!=5)throw std::runtime_error("Metre conversion failed");
  auto roundtripSaved=d.rows;d.rows={{L"5.05",L"2.31",L"1.36"},{L"5",L"2.2",L"1.4"},{L"5.25",L"2.25",L"1.3"},{},{}};manualRebuild(d);auto roundtripRows=d.rows;double roundtripVolume=d.result.mean;
  for(int cycle=0;cycle<20;++cycle){
   SendDlgItemMessageW(d.window,MANUAL_UNITS,CB_SETCURSEL,1,0);SendMessageW(d.window,WM_COMMAND,MAKEWPARAM(MANUAL_UNITS,CBN_SELCHANGE),0);
   if(d.rows[0][0]!=L"505"||d.rows[0][1]!=L"231"||d.rows[0][2]!=L"136"||std::abs(d.result.mean-roundtripVolume)>1e-12)throw std::runtime_error("Rounded centimetre values or volume changed");
   SendDlgItemMessageW(d.window,MANUAL_UNITS,CB_SETCURSEL,0,0);SendMessageW(d.window,WM_COMMAND,MAKEWPARAM(MANUAL_UNITS,CBN_SELCHANGE),0);
   if(d.rows!=roundtripRows||std::abs(d.result.mean-roundtripVolume)>1e-12)throw std::runtime_error("Decimal roundtrip changed dimensions or volume");
  }
  capture(L"manual-roundtrip.png");d.rows=roundtripSaved;manualRebuild(d);
  SendMessageW(d.window,WM_COMMAND,MANUAL_ADD,0);if(d.rows.size()!=6||d.result.count!=5)throw std::runtime_error("Add blank row failed");
  SendMessageW(d.table,WM_COMMAND,MAKEWPARAM(2001,BN_CLICKED),(LPARAM)d.widgets[1].remove);if(d.rows.size()!=5||d.result.count!=4||d.result.mean!=11.25)throw std::runtime_error("Delete selected row failed");
  SetWindowTextW(d.widgets[0].edits[1],L"0");if(d.result.error.empty())throw std::runtime_error("Invalid dimension accepted");SetWindowTextW(d.widgets[0].edits[1],L"2");
  for(int i=0;i<8;++i)SendMessageW(d.window,WM_COMMAND,MANUAL_ADD,0);
  if(d.scroll<=0)throw std::runtime_error("Manual table did not scroll");
  capture(L"manual-scroll.png");
  std::filesystem::create_directories(renderTestDir);std::ofstream(renderTestDir/L"manual-ok.txt")<<"PASS comma decimals, five measurements, average/min/max, add/delete, invalid input and scrolling";
 }catch(const std::exception& e){std::ofstream(renderTestDir/L"manual-error.txt")<<e.what();}
}
