#pragma once
#include "logo.hpp"
// Native GDI controls: no UI runtime or assets required beside the executable.
COLORREF bg=RGB(239,241,243),panel=RGB(225,230,233),card=RGB(250,251,252),muted=RGB(76,94,106),ink=RGB(26,42,51),accent=RGB(0,112,38),border=RGB(173,184,190),activeBg=RGB(213,234,201),hoverBg=RGB(229,239,220);
void refreshCaptionTheme(HWND window){
 if(!window)return;
 // Windows 10 can retain the previous DWM caption until nonclient activation
 // is refreshed. These messages repaint the frame only; they neither activate
 // the window nor move keyboard focus. Restore the actual activation state.
 if(IsWindowVisible(window)&&!IsIconic(window)){
  BOOL active=GetActiveWindow()==window;
  SendMessageW(window,WM_NCACTIVATE,!active,0);
  SendMessageW(window,WM_NCACTIVATE,active,0);
 }
 RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_ALLCHILDREN|RDW_UPDATENOW);
 // Commit the updated nonclient surface before returning to the menu loop.
 if(HMODULE dwm=LoadLibraryW(L"dwmapi.dll")){using Flush=HRESULT(WINAPI*)();auto flush=(Flush)GetProcAddress(dwm,"DwmFlush");if(flush)flush();FreeLibrary(dwm);}
}
void applyTheme(bool light){
 lightTheme=light;bg=light?RGB(239,241,243):RGB(12,16,23);panel=light?RGB(225,230,233):RGB(18,24,33);card=light?RGB(250,251,252):RGB(26,34,46);muted=light?RGB(76,94,106):RGB(133,150,171);ink=light?RGB(26,42,51):RGB(229,237,246);accent=light?RGB(0,112,38):RGB(77,219,190);border=light?RGB(173,184,190):RGB(43,54,69);activeBg=light?RGB(213,234,201):RGB(30,63,62);hoverBg=light?RGB(229,239,220):RGB(36,47,62);
 if(HMODULE dwm=LoadLibraryW(L"dwmapi.dll")){using SetAttribute=HRESULT(WINAPI*)(HWND,DWORD,LPCVOID,DWORD);auto set=(SetAttribute)GetProcAddress(dwm,"DwmSetWindowAttribute");BOOL dark=!light;if(set&&FAILED(set(mainWin,20,&dark,sizeof(dark))))set(mainWin,19,&dark,sizeof(dark));if(set){set(mainWin,35,&panel,sizeof(panel));set(mainWin,36,&ink,sizeof(ink));}FreeLibrary(dwm);}
 redraw();refreshCaptionTheme(mainWin);
}

// Test capture includes the native caption; normal PNG export stays viewport-only.
COLORREF saveCaptionTest(const std::filesystem::path& path,bool refresh=true){
 if(!v2TestMode)return CLR_INVALID;bool wasVisible=IsWindowVisible(mainWin);if(!wasVisible)ShowWindow(mainWin,SW_SHOWNOACTIVATE);if(refresh){RedrawWindow(mainWin,nullptr,nullptr,RDW_INVALIDATE|RDW_FRAME|RDW_UPDATENOW);Sleep(100);}
 RECT r;GetWindowRect(mainWin,&r);int w=r.right-r.left,h=32;HDC dc=GetWindowDC(mainWin),mem=CreateCompatibleDC(dc);
 BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;void* bits=nullptr;
 auto bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);auto old=SelectObject(mem,bitmap);PrintWindow(mainWin,mem,2);GdiFlush();std::vector<uint8_t> rgb(size_t(w)*h*3);auto p=(uint8_t*)bits;
 COLORREF captionColor=GetPixel(mem,w/2,12);
 for(size_t i=0;i<size_t(w)*h;i++){rgb[i*3]=p[i*4+2];rgb[i*3+1]=p[i*4+1];rgb[i*3+2]=p[i*4];}SelectObject(mem,old);DeleteObject(bitmap);DeleteDC(mem);ReleaseDC(mainWin,dc);if(!wasVisible)ShowWindow(mainWin,SW_HIDE);writeBytes(path,encodePng(w,h,rgb));return captionColor;
}
HFONT smallFont,numberFont;
void fill(HDC dc,RECT r,COLORREF c){SetDCBrushColor(dc,c);FillRect(dc,&r,(HBRUSH)GetStockObject(DC_BRUSH));}
void roundBox(HDC dc,RECT r,COLORREF c,COLORREF border){SelectObject(dc,GetStockObject(DC_BRUSH));SelectObject(dc,GetStockObject(DC_PEN));SetDCBrushColor(dc,c);SetDCPenColor(dc,border);RoundRect(dc,r.left,r.top,r.right,r.bottom,4,4);}
void label(HDC dc,RECT r,const std::wstring& s,COLORREF c=ink,HFONT f=nullptr,UINT flags=DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS){SelectObject(dc,f?f:font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,c);DrawTextW(dc,s.c_str(),int(s.size()),&r,flags);}
std::wstring grouped(size_t n){auto s=std::to_wstring(n);for(int i=int(s.size())-3;i>0;i-=3)s.insert(i,L" ");return s;}
void drawButton(HDC dc,RECT r,int id,bool down=false,bool focus=false){
    RECT controlRect{};GetWindowRect(GetDlgItem(mainWin,id),&controlRect);MapWindowPoints(nullptr,mainWin,(POINT*)&controlRect,2);fill(dc,r,controlRect.left<300?panel:bg);
    bool active=!model.points.empty()&&((id==WATER&&(scanLayers.enabled?(scanLayers.emptyWater||scanLayers.fullWater):showWater))||(id==CONTEXT_POINTS&&(scanLayers.enabled?scanLayers.other:cargoView&&showContext))||(id==CARGO&&(scanLayers.enabled?scanLayers.cargo:cargoView))||(id==OVERLAY&&(scanLayers.enabled?scanLayers.full&&scanLayers.empty:overlayView))||(id==COMPARE&&differenceView)||(id==REGION_SELECT&&selectRegion)||(id==POINT_MODE&&!mesh)||(id==SURFACE&&mesh)||(id==GRID&&showZone)||(id==GROUP0&&model.selectedType==0&&!model.points.empty())||(id==GROUP1&&model.selectedType==1&&!model.points.empty()));
    bool enabled=IsWindowEnabled(GetDlgItem(mainWin,id));
    COLORREF base=id==OPEN?activeBg:active?activeBg:card;
    COLORREF selected=id==WATER?RGB(65,145,255):accent;if(id==WATER&&active)base=RGB(23,44,80);
    if(enabled&&GetPropW(GetDlgItem(mainWin,id),L"ScanHover")&&!active&&id!=OPEN)base=hoverBg;
    if(down)base=activeBg;
    roundBox(dc,r,enabled?base:panel,active?selected:border);
    wchar_t text[180]{};GetWindowTextW(GetDlgItem(mainWin,id),text,180);
    label(dc,{r.left+8,r.top,r.right-8,r.bottom},text,!enabled?RGB(77,89,106):id==OPEN?ink:active?selected:id==WATER?RGB(110,175,255):ink,id==LOAD_DATABASE?smallFont:font,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
    if(focus){InflateRect(&r,-4,-4);DrawFocusRect(dc,&r);}
}
void drawFilePicker(HDC dc,RECT r,int id){
    bool enabled=IsWindowEnabled(GetDlgItem(mainWin,id));fill(dc,r,bg);roundBox(dc,r,enabled?card:panel,GetFocus()==GetDlgItem(mainWin,id)?accent:border);
    wchar_t text[32768]{};GetWindowTextW(GetDlgItem(mainWin,id),text,32768);
    std::wstring value=text[0]?text:id==ACTIVE_FILE?L"С грузом: выберите Full":L"Пустой кузов: выберите Empty";
    label(dc,{r.left+12,r.top,r.right-32,r.bottom},value,enabled&&text[0]?ink:muted,font);
    SelectObject(dc,GetStockObject(DC_PEN));SetDCPenColor(dc,enabled?muted:RGB(77,89,106));int x=r.right-17,y=(r.top+r.bottom)/2;
    MoveToEx(dc,x-4,y-2,nullptr);LineTo(dc,x,y+2);LineTo(dc,x+4,y-2);
}
LRESULT CALLBACK PickerSkinProc(HWND w,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
    auto result=DefSubclassProc(w,msg,wp,lp);
    if(msg==WM_PAINT){HDC dc=GetDC(w);RECT r;GetClientRect(w,&r);drawFilePicker(dc,r,GetDlgCtrlID(w));ReleaseDC(w,dc);}
    if(msg==WM_SETFOCUS||msg==WM_KILLFOCUS||msg==WM_ENABLE)InvalidateRect(w,nullptr,FALSE);
    if(msg==WM_NCDESTROY)RemoveWindowSubclass(w,PickerSkinProc,id);
    (void)data;return result;
}
LRESULT CALLBACK ButtonSkinProc(HWND w,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
    if(msg==WM_MOUSEMOVE&&!GetPropW(w,L"ScanHover")){SetPropW(w,L"ScanHover",(HANDLE)1);TRACKMOUSEEVENT tracking{sizeof(tracking),TME_LEAVE,w,0};TrackMouseEvent(&tracking);InvalidateRect(w,nullptr,FALSE);}
    if(msg==WM_MOUSELEAVE){RemovePropW(w,L"ScanHover");InvalidateRect(w,nullptr,FALSE);}
    if(msg==WM_NCDESTROY){RemovePropW(w,L"ScanHover");RemoveWindowSubclass(w,ButtonSkinProc,id);}
    (void)data;return DefSubclassProc(w,msg,wp,lp);
}
void drawPickerItem(DRAWITEMSTRUCT* d){
    if(d->itemID==UINT(-1))return;bool selected=(d->itemState&ODS_SELECTED)!=0;fill(d->hDC,d->rcItem,selected?activeBg:card);
    LRESULT length=SendMessageW(d->hwndItem,CB_GETLBTEXTLEN,d->itemID,0);if(length<0||length>1000000)return;std::wstring text(size_t(length)+1,L'\0');SendMessageW(d->hwndItem,CB_GETLBTEXT,d->itemID,(LPARAM)text.data());text.resize(size_t(length));RECT r=d->rcItem;r.left+=12;r.right-=10;label(d->hDC,r,text,selected?accent:ink,font);
    if(d->itemState&ODS_FOCUS)DrawFocusRect(d->hDC,&d->rcItem);
}
void syncControls(){
    for(int id:{POINT_MODE,SURFACE,RESET,TOP,FRONT,SIDE,EXPORT,GRID,SMALL,LARGE})EnableWindow(GetDlgItem(mainWin,id),!model.points.empty()&&!busy);
    EnableWindow(GetDlgItem(mainWin,GROUP0),!busy&&(model.availableTypes&1));EnableWindow(GetDlgItem(mainWin,GROUP1),!busy&&(model.availableTypes&2));
    for(int id=OPEN;id<=MANUAL_CALC;id++)if(HWND b=GetDlgItem(mainWin,id))InvalidateRect(b,nullptr,FALSE);
    EnableWindow(GetDlgItem(mainWin,OPTIONS),!busy);EnableWindow(GetDlgItem(mainWin,LOAD_DATABASE),!busy);EnableWindow(GetDlgItem(mainWin,MANUAL_CALC),!busy);
    for(int id:{REGION_SELECT,REGION_CLEAR,PNG_SAVE,REPORT_SAVE,CLOSE_FILE})EnableWindow(GetDlgItem(mainWin,id),!busy&&!model.points.empty());
    EnableWindow(GetDlgItem(mainWin,AUTO_ALIGN),!busy&&documents.size()>1);
    EnableWindow(GetDlgItem(mainWin,CARGO),!busy&&comparison.has_value());
    EnableWindow(GetDlgItem(mainWin,CONTEXT_POINTS),!busy&&comparison.has_value());
    SetWindowTextW(GetDlgItem(mainWin,CARGO),cargoView&&showContext?L"Груз + фон":L"Только груз");
    EnableWindow(GetDlgItem(mainWin,COMPARE),!busy&&documents.size()>1);EnableWindow(GetDlgItem(mainWin,OVERLAY),!busy&&documents.size()>1);EnableWindow(GetDlgItem(mainWin,ACTIVE_FILE),!busy&&!documents.empty());EnableWindow(GetDlgItem(mainWin,BASE_FILE),!busy&&!documents.empty());
    if(viewWin)ShowWindow(viewWin,model.points.empty()?SW_HIDE:SW_SHOW);
}
void layout(){
    RECT r;GetClientRect(mainWin,&r);
    MoveWindow(viewWin,320,174,std::max(1L,r.right-340),std::max(1L,r.bottom-322),TRUE);v2Layout();
    MoveWindow(statusWin,24,r.bottom-29,std::max(1L,r.right-48),22,TRUE);
    MoveWindow(GetDlgItem(mainWin,EXPORT),20,r.bottom-82,260,40,TRUE);
}
void paintPanel(HDC dc){
    RECT r;GetClientRect(mainWin,&r);fill(dc,r,bg);fill(dc,{0,0,300,r.bottom-40},panel);fill(dc,{300,0,301,r.bottom-40},border);
    for(int top:{151,231,357,437})fill(dc,{16,top,284,top+1},border);
    drawBrandLogo(dc,{20,16,174,54});
    label(dc,{20,58,282,89},L"ScanViewer",ink,numberFont);
    label(dc,{20,154,280,175},L"ОТОБРАЖЕНИЕ",muted,smallFont);
    label(dc,{20,234,280,255},L"РАКУРС",muted,smallFont);
    label(dc,{20,360,280,381},L"НАБОР ДАННЫХ",muted,smallFont);
    label(dc,{20,440,280,461},L"ПАРАМЕТРЫ ВИДА",muted,smallFont);
    if(r.bottom>820)label(dc,{20,650,280,694},controlDatabase.path.empty()?L"Контрольная база не загружена":L"SQ3 · "+std::to_wstring(controlDatabase.ids.size())+L" записей",muted,smallFont);
    label(dc,{20,558,192,590},L"Размер точек · "+std::to_wstring(int(pointSize))+L" px",muted,smallFont);
    label(dc,{324,14,r.right-170,47},busy?L"Обработка сканов…":model.points.empty()?L"Просмотр 3D-сканов":loadedPath.filename().wstring(),ink,titleFont);
    label(dc,{325,48,r.right-180,74},model.points.empty()?L"Сравнение сканов и оценка объёма":L"Измерение "+std::to_wstring(model.scan)+L"   /   Набор "+std::to_wstring(model.selectedType)+L"   /   "+(scanLayers.enabled?L"Слои просмотра":cargoView?(showContext?L"Груз цветом · остальные точки серые":L"Только груз"):overlayView?L"Наложение":differenceView?L"Разность высот":mesh?L"Поверхность":L"Облако точек"),muted);
    roundBox(dc,{r.right-145,24,r.right-20,58},activeBg,border);
    label(dc,{r.right-145,24,r.right-20,58},busy?L"ОБРАБОТКА":comparison?(comparison->options.provisional?L"ПРОВЕРИТЬ":L"РАССЧИТАНО"):baseDocument>=0?L"ПАРА ГОТОВА":L"ОТКРОЙТЕ ПАРУ",accent,smallFont,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    if(model.points.empty()){
        int cx=(320+r.right-20)/2,cy=(174+r.bottom-148)/2;
        roundBox(dc,{cx-46,cy-125,cx+46,cy-33},card,RGB(44,69,78));
        for(int y=0;y<5;y++)for(int x=0;x<5;x++){int px=cx-24+x*12,py=cy-103+y*12;fill(dc,{px,py,px+3,py+3},(x+y)%3?accent:RGB(73,108,124));}
        label(dc,{320,cy-10,r.right-20,cy+30},L"Откройте скан с грузом",ink,titleFont,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        label(dc,{320,cy+40,r.right-20,cy+69},L"Перетащите Full сюда — Empty рядом загрузится автоматически",muted,font,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        label(dc,{320,cy+78,r.right-20,cy+104},L"Full · Empty · Reference    /    BIN",muted,smallFont,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }
    if(busy&&(volumeTask.valid()||alignmentTask.valid())){
        bool volume=volumeTask.valid();int progress=std::clamp(volume?volumeProgress.load():alignmentProgress.load(),0,100);
        label(dc,{324,r.bottom-140,r.right-20,r.bottom-110},(volume?L"Расчёт объёма · ":L"Точное совмещение · ")+std::to_wstring(progress)+L"%",accent,numberFont);
        RECT track{324,r.bottom-101,r.right-24,r.bottom-87};roundBox(dc,track,card,border);
        if(progress>0){RECT filled=track;filled.right=filled.left+int((track.right-track.left)*progress/100.);fill(dc,filled,accent);}
        std::wstring phase=volume?(progress<15?L"Построение поверхностей и проверка небольших пропусков":progress<30?L"Выбор шага по плотности и пропускам":progress<70?L"Проверка устойчивости на пяти разрешениях":progress<95?L"Проверка влияния смещения сетки":L"Уточнение результата"):
            progress<85?L"Проверка поворотов 0–360° и разных опорных поверхностей":L"Точное уточнение лучших вариантов";
        label(dc,{324,r.bottom-80,r.right-20,r.bottom-51},phase,muted,smallFont);
        fill(dc,{0,r.bottom-41,r.right,r.bottom-40},border);return;
    }
    if(model.points.empty()){
        label(dc,{324,r.bottom-78,r.right-20,r.bottom-52},L"Ctrl+O — открыть BIN   ·   Можно выбрать несколько файлов",muted,smallFont);
        fill(dc,{0,r.bottom-41,r.right,r.bottom-40},border);return;
    }
    label(dc,{324,r.bottom-140,r.right-20,r.bottom-110},comparisonSummary(),accent,cargoView?numberFont:font);
    int y=r.bottom-98;
    std::wstring details=model.points.empty()?L"Обработка локально. BIN не изменяется.":L"Точек: "+grouped(model.points.size())+L" · Масштаб координат проверьте в настройках";
    if(comparison){auto& c=*comparison;details=L"Шаг: "+std::to_wstring(int(c.options.step))+L" · Общая сетка: "+std::to_wstring(int(c.coverage()*100))+L"%";if(c.sensitivityChecked&&c.options.metresPerUnit>0){std::wostringstream text;text<<std::fixed<<std::setprecision(2)<<c.sensitivityMin*std::pow(c.options.metresPerUnit,3)<<L" - "<<c.sensitivityMax*std::pow(c.options.metresPerUnit,3);details+=L" · При разных шагах: "+text.str()+L" м³";}}
    label(dc,{324,y,r.right-300,y+25},details,muted,smallFont);
    std::wstring bottomHint=L"ЛКМ: вращать   Shift+ЛКМ: сдвиг   Колесо: к указателю   Двойной щелчок: общий вид";
    if(cargoView&&comparison&&cargo.reconstructedVolume>0){
        std::wostringstream estimate;double scale=comparison->options.metresPerUnit;
        estimate<<std::fixed<<std::setprecision(2)<<cargo.reconstructedVolume*std::pow(scale>0?scale:1,3);
        bottomHint=L"Восстановленные ячейки: "+estimate.str()+(scale>0?L" м³":L" ед.³")+L" · Открытые и двойные пропуски не заполнены · Настройки: можно отключить";
    }
    label(dc,{324,y+29,r.right-20,y+51},bottomHint,muted,smallFont);
    if(!model.points.empty()){
    label(dc,{r.right-240,y-3,r.right-20,y+17},overlayView?L"АКТИВНЫЙ / БАЗА":differenceView?L"ΔZ: СИНИЙ − / КРАСНЫЙ +":L"ВЫСОТА Z",muted,smallFont);
    Model scale;scale.lo.z=0;scale.hi.z=1;
    for(int i=0;i<220;i++){auto c=color(scale,{0,0,i/219.f});if(differenceView){float t=i/219.f*2-1,a=std::abs(t);c=t>=0?std::array<float,3>{.25f+.75f*a,.8f*(1-a),.65f*(1-a)}:std::array<float,3>{.25f*(1-a),.8f*(1-a),.65f+.35f*a};}if(overlayView)c=i<110?std::array<float,3>{1.f,.65f,.15f}:std::array<float,3>{.1f,.75f,1.f};fill(dc,{r.right-240+i,y+23,r.right-239+i,y+28},RGB(int(c[0]*255),int(c[1]*255),int(c[2]*255)));}
    }
    fill(dc,{0,r.bottom-41,r.right,r.bottom-40},border);
}
// Export the app's own paint output for visual regression review, without desktop capture.
void saveInterface(const std::filesystem::path& path){
    RECT r;GetClientRect(mainWin,&r);BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=r.right;bi.bmiHeader.biHeight=-r.bottom;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
    void* bits=nullptr;HDC dc=CreateCompatibleDC(nullptr);HBITMAP bmp=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,nullptr,0);auto old=SelectObject(dc,bmp);
    paintPanel(dc);
    for(int id=OPEN;id<=MANUAL_CALC;id++){HWND b=GetDlgItem(mainWin,id);if(!b)continue;RECT q;GetWindowRect(b,&q);MapWindowPoints(nullptr,mainWin,(POINT*)&q,2);if(id==ACTIVE_FILE||id==BASE_FILE)drawFilePicker(dc,q,id);else drawButton(dc,q,id);}
    wchar_t text[1024]{};GetWindowTextW(statusWin,text,1024);label(dc,{24,r.bottom-29,r.right-24,r.bottom-7},text,muted);
    if(!model.points.empty()){
        render();glFinish();glReadBuffer(GL_FRONT);glPixelStorei(GL_PACK_ALIGNMENT,4);int stride=(viewW*3+3)&~3;std::vector<unsigned char> pixels(size_t(stride)*viewH);glReadPixels(0,0,viewW,viewH,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
        for(int y=0;y<viewH;y++)for(int x=0;x<viewW;x++)std::swap(pixels[size_t(y)*stride+x*3],pixels[size_t(y)*stride+x*3+2]);
        BITMAPINFO v{};v.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);v.bmiHeader.biWidth=viewW;v.bmiHeader.biHeight=viewH;v.bmiHeader.biPlanes=1;v.bmiHeader.biBitCount=24;
        StretchDIBits(dc,320,174,viewW,viewH,0,0,viewW,viewH,pixels.data(),&v,DIB_RGB_COLORS,SRCCOPY);
    }
    GdiFlush();BITMAPFILEHEADER fh{};fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(BITMAPINFOHEADER);fh.bfSize=fh.bfOffBits+r.right*r.bottom*4;
    std::ofstream out(path,std::ios::binary);out.write((char*)&fh,sizeof(fh));out.write((char*)&bi.bmiHeader,sizeof(BITMAPINFOHEADER));out.write((char*)bits,size_t(r.right)*r.bottom*4);
    SelectObject(dc,old);DeleteObject(bmp);DeleteDC(dc);if(!out)throw std::runtime_error("Interface preview write failed");
}
