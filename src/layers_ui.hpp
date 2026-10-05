#pragma once
ScanLayers currentScanLayers(){
    if(scanLayers.enabled)return scanLayers;
    ScanLayers s;s.enabled=true;s.full=!cargoView&&model.kind==2;s.empty=overlayView||(!cargoView&&model.kind!=2&&baseDocument>=0);
    s.cargo=cargoView&&comparison.has_value();s.other=cargoView&&showContext;s.emptyWater=showWater;return s;
}
void applyScanLayers(ScanLayers layers){
    if(!lockedFrame)lockedFrame=currentViewBounds();
    scanLayers=layers;scanLayers.enabled=true;compileLists();redraw();
}
struct LayersDialog {ScanLayers before,initial;std::optional<ViewBounds> frame;bool done=false,accepted=false;int test=0;};
LRESULT CALLBACK LayersProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
    auto d=(LayersDialog*)GetWindowLongPtrW(w,GWLP_USERDATA);
    if(msg==WM_NCCREATE){d=(LayersDialog*)((CREATESTRUCTW*)lp)->lpCreateParams;SetWindowLongPtrW(w,GWLP_USERDATA,(LONG_PTR)d);}
    if(msg==WM_TIMER&&d->test){KillTimer(w,1);
        if(d->test==3){CheckDlgButton(w,310,BST_CHECKED);SendMessageW(w,WM_COMMAND,MAKEWPARAM(310,BN_CLICKED),0);SendMessageW(w,WM_COMMAND,IDOK,0);return 0;}
        CheckDlgButton(w,301,BST_CHECKED);CheckDlgButton(w,302,BST_UNCHECKED);CheckDlgButton(w,303,BST_UNCHECKED);CheckDlgButton(w,304,BST_CHECKED);CheckDlgButton(w,305,BST_UNCHECKED);CheckDlgButton(w,306,fullWater.available?BST_CHECKED:BST_UNCHECKED);
        SendMessageW(w,WM_COMMAND,MAKEWPARAM(301,BN_CLICKED),0);
        SendMessageW(w,WM_COMMAND,d->test==1?IDOK:IDCANCEL,0);return 0;
    }
    if(msg==WM_COMMAND){int id=LOWORD(wp);
        if(id>=301&&id<=310&&HIWORD(wp)==BN_CLICKED){ScanLayers s;s.enabled=true;
            s.full=IsDlgButtonChecked(w,301)==BST_CHECKED;s.empty=IsDlgButtonChecked(w,302)==BST_CHECKED;
            s.cargo=IsDlgButtonChecked(w,303)==BST_CHECKED;s.other=IsDlgButtonChecked(w,304)==BST_CHECKED;
            s.emptyWater=IsDlgButtonChecked(w,305)==BST_CHECKED;s.fullWater=IsDlgButtonChecked(w,306)==BST_CHECKED;
            s.tarp=IsDlgButtonChecked(w,310)==BST_CHECKED;
            s.label=IsDlgButtonChecked(w,307)==BST_CHECKED;grid=IsDlgButtonChecked(w,308)==BST_CHECKED;showZone=IsDlgButtonChecked(w,309)==BST_CHECKED;
            applyScanLayers(s);return 0;
        }
        if(id==IDOK){d->accepted=true;DestroyWindow(w);return 0;}
        if(id==IDCANCEL){DestroyWindow(w);return 0;}
    }
    if(msg==WM_CLOSE){DestroyWindow(w);return 0;}
    if(msg==WM_DESTROY){d->done=true;return 0;}
    return DefWindowProcW(w,msg,wp,lp);
}
void layerSettings(int test=0){
    if(activeDocument<0)return;
    LayersDialog d;d.test=test;d.before=scanLayers;d.initial=currentScanLayers();d.frame=lockedFrame;bool oldGrid=grid,oldZone=showZone;
    HINSTANCE inst=GetModuleHandleW(nullptr);WNDCLASSW wc{};wc.hInstance=inst;wc.lpfnWndProc=LayersProc;wc.lpszClassName=L"LaseScanLayers";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);RegisterClassW(&wc);
    RECT parent;GetWindowRect(mainWin,&parent);
    HWND w=CreateWindowExW(WS_EX_DLGMODALFRAME,L"LaseScanLayers",L"Слои просмотра",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,parent.left+100,parent.top+60,530,575,mainWin,nullptr,inst,&d);
    auto control=[&](const wchar_t* cls,const wchar_t* text,int id,int x,int y,int width,int height,DWORD style){auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,(HMENU)(INT_PTR)id,inst,nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)font,TRUE);return c;};
    control(L"STATIC",L"Слои включаются независимо. Камера сохраняется.",0,20,16,480,36,0);
    const wchar_t* names[]={L"Полный скан · цветные точки",L"Пустой кузов / Reference · голубые точки",L"Объём груза · цветные точки / поверхность",L"Остальные точки Full · серые",L"По воде: вместимость пустого кузова · синий",L"По воде: над грузом до бортов · синий",L"Показывать значение объёма груза",L"Сетка координат",L"Границы зоны интереса и подписи X, Y, Z",L"Виртуальный тент · только груз выше бортов"};
    bool values[]={d.initial.full,d.initial.empty,d.initial.cargo,d.initial.other,d.initial.emptyWater,d.initial.fullWater,d.initial.label,grid,showZone,d.initial.tarp};
    for(int i=0;i<10;i++){auto c=control(L"BUTTON",names[i],301+i,20,60+i*34,485,28,BS_AUTOCHECKBOX|WS_TABSTOP);CheckDlgButton(w,301+i,values[i]?BST_CHECKED:BST_UNCHECKED);
        bool available=i==1?baseDocument>=0:i==2||i==3?comparison.has_value():i==4?water.available:i==5?fullWater.available:i==9?tarp.available:true;EnableWindow(c,available);
    }
    control(L"STATIC",L"Синяя плоскость показывает уровень бортов без пропусков.\nОбъём воды над грузом учитывает только измеренные ячейки.",0,20,410,485,62,0);
    control(L"BUTTON",L"Применить",IDOK,240,485,125,34,BS_DEFPUSHBUTTON|WS_TABSTOP);control(L"BUTTON",L"Отмена",IDCANCEL,375,485,125,34,BS_PUSHBUTTON|WS_TABSTOP);
    if(test)SetTimer(w,1,50,nullptr);
    EnableWindow(mainWin,FALSE);skinSettings(w);ShowWindow(w,SW_SHOW);SetFocus(GetDlgItem(w,301));MSG msg;
    while(!d.done&&GetMessageW(&msg,nullptr,0,0)>0)if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    EnableWindow(mainWin,TRUE);SetActiveWindow(mainWin);
    if(d.accepted){if(!scanLayers.enabled)applyScanLayers(d.initial);}else{scanLayers=d.before;lockedFrame=d.frame;grid=oldGrid;showZone=oldZone;compileLists();redraw();}
}

#include "preferences_ui.hpp"
struct SettingsHome {bool done=false;int action=0,test=0;};
void captureSettingsHome(HWND w){
 RECT r;GetClientRect(w,&r);HDC dc=GetDC(w),mem=CreateCompatibleDC(dc);BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=r.right;bi.bmiHeader.biHeight=-r.bottom;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;void* bits;auto bmp=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,nullptr,0);auto old=SelectObject(mem,bmp);PrintWindow(w,mem,PW_CLIENTONLY);BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(bi.bmiHeader);header.bfSize=header.bfOffBits+r.right*r.bottom*4;std::ofstream file(renderTestDir/(lightTheme?L"settings-light.bmp":L"settings-dark.bmp"),std::ios::binary);file.write((char*)&header,sizeof(header));file.write((char*)&bi.bmiHeader,sizeof(bi.bmiHeader));file.write((char*)bits,r.right*r.bottom*4);SelectObject(mem,old);DeleteObject(bmp);DeleteDC(mem);ReleaseDC(w,dc);
}
LRESULT CALLBACK SettingsHomeProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 auto d=(SettingsHome*)GetWindowLongPtrW(w,GWLP_USERDATA);if(msg==WM_NCCREATE){d=(SettingsHome*)((CREATESTRUCTW*)lp)->lpCreateParams;SetWindowLongPtrW(w,GWLP_USERDATA,(LONG_PTR)d);}
 if(msg==WM_TIMER&&d->test){KillTimer(w,1);captureSettingsHome(w);DestroyWindow(w);return 0;}
 if(msg==WM_COMMAND){int id=LOWORD(wp);if(id==600&&HIWORD(wp)==CBN_SELCHANGE){preferenceCommand(SendDlgItemMessageW(w,600,CB_GETCURSEL,0,0)==0?THEME_DARK:THEME_LIGHT);skinSettings(w);return 0;}if(id==601&&HIWORD(wp)==CBN_SELCHANGE){preferenceCommand(SendDlgItemMessageW(w,601,CB_GETCURSEL,0,0)==0?FORMAT_HTML:FORMAT_PDF);return 0;}if(id==600||id==601)return 0;if(id==IDCANCEL||id==IDOK){DestroyWindow(w);return 0;}d->action=id;DestroyWindow(w);return 0;}
 if(msg==WM_CLOSE){DestroyWindow(w);return 0;}if(msg==WM_DESTROY){d->done=true;return 0;}return DefWindowProcW(w,msg,wp,lp);
}
void settingsMenu(int test=0){
 do{
 SettingsHome d;d.test=test;auto inst=GetModuleHandleW(nullptr);WNDCLASSW wc{};wc.hInstance=inst;wc.lpfnWndProc=SettingsHomeProc;wc.lpszClassName=L"LaseSettingsHome";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&wc);RECT r;GetWindowRect(mainWin,&r);
 auto w=CreateWindowExW(WS_EX_DLGMODALFRAME,L"LaseSettingsHome",L"Настройки",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,r.left+80,r.top+40,700,456,mainWin,nullptr,inst,&d);
 auto add=[&](const wchar_t* cls,const wchar_t* text,int id,int x,int y,int width,int height,DWORD style=0,HFONT f=nullptr){auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,(HMENU)(INT_PTR)id,inst,nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)(f?f:font),TRUE);return c;};
 add(L"STATIC",L"Оформление и работа программы",0,22,16,620,28,0,numberFont);
 add(L"STATIC",L"Тема интерфейса",0,22,56,260,22);add(L"STATIC",L"Формат сохраняемого отчёта",0,354,56,300,22);
 auto theme=add(L"COMBOBOX",L"",600,22,82,300,150,CBS_DROPDOWNLIST|WS_TABSTOP);for(auto text:{L"Тёмная",L"Светлая"})SendMessageW(theme,CB_ADDSTRING,0,(LPARAM)text);SendMessageW(theme,CB_SETCURSEL,lightTheme?1:0,0);
 auto format=add(L"COMBOBOX",L"",601,354,82,300,150,CBS_DROPDOWNLIST|WS_TABSTOP);for(auto text:{L"HTML — открыть в браузере",L"PDF — печать и отправка"})SendMessageW(format,CB_ADDSTRING,0,(LPARAM)text);SendMessageW(format,CB_SETCURSEL,reportFormat==ReportFormat::PDF?1:0,0);
 add(L"STATIC",L"Тема меняется сразу. Окно 3D-просмотра остаётся чёрным.",0,22,121,630,22,0,smallFont);
 struct Row{int id;const wchar_t* name;const wchar_t* help;};
 int y=164;for(auto row:{Row{702,L"Расчёт и совмещение",L"Единицы BIN, точность сетки, границы кузова и ручная поправка совмещения."},Row{LOAD_DATABASE,L"Загрузить базу SQ3",L"Показывает контрольный TotalVolume и отклонение. Не меняет рассчитанный объём."},Row{704,L"Порт веб-версии",L"Выберите адрес для браузера. Новый порт применяется после перезапуска веб-версии."}}){auto b=add(L"BUTTON",row.name,row.id,22,y,218,36,WS_TABSTOP|BS_PUSHBUTTON);if(row.id==701||row.id==702)EnableWindow(b,activeDocument>=0);add(L"STATIC",row.help,0,258,y,396,48,0,smallFont);y+=60;}
 auto unload=add(L"BUTTON",L"Отключить базу SQ3",UNLOAD_DATABASE,22,360,218,34,WS_TABSTOP);EnableWindow(unload,!controlDatabase.path.empty());add(L"BUTTON",L"Готово",IDOK,534,360,120,34,WS_TABSTOP|BS_DEFPUSHBUTTON);
 skinSettings(w);EnableWindow(mainWin,FALSE);ShowWindow(w,SW_SHOW);SetFocus(theme);if(test)SetTimer(w,1,100,nullptr);MSG msg;while(!d.done&&GetMessageW(&msg,nullptr,0,0)>0)if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}EnableWindow(mainWin,TRUE);SetActiveWindow(mainWin);
 if(!d.action||test)return;
 if(d.action==701)layerSettings();else if(d.action==702)settings();else if(d.action==704)webPortSettings();else v2Command(d.action);
 }while(true);
}
