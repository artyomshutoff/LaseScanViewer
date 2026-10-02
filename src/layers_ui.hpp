#pragma once
ScanLayers currentScanLayers(){
    if(scanLayers.enabled)return scanLayers;
    ScanLayers s;s.enabled=true;s.full=!cargoView;s.empty=overlayView;
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
    EnableWindow(mainWin,FALSE);ShowWindow(w,SW_SHOW);SetFocus(GetDlgItem(w,301));MSG msg;
    while(!d.done&&GetMessageW(&msg,nullptr,0,0)>0)if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    EnableWindow(mainWin,TRUE);SetActiveWindow(mainWin);
    if(d.accepted){if(!scanLayers.enabled)applyScanLayers(d.initial);}else{scanLayers=d.before;lockedFrame=d.frame;grid=oldGrid;showZone=oldZone;compileLists();redraw();}
}
#include "compute_ui.hpp"
#include "preferences_ui.hpp"
void settingsMenu(){
    HMENU menu=CreatePopupMenu();AppendMenuW(menu,MF_STRING|(activeDocument<0?MF_GRAYED:0),1,L"Слои просмотра…");AppendMenuW(menu,MF_STRING|(activeDocument<0?MF_GRAYED:0),2,L"Параметры расчёта…");AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING,3,L"Устройство вычислений · CPU / GPU…");
    AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING,MANUAL_CALC,L"Обмеры кузова · ручной калькулятор…");AppendMenuW(menu,MF_POPUP,(UINT_PTR)themeMenu(),L"Тема интерфейса");AppendMenuW(menu,MF_POPUP,(UINT_PTR)formatMenu(),L"Формат отчёта");AppendMenuW(menu,MF_STRING,LOAD_DATABASE,L"Загрузить контрольную базу SQ3…");AppendMenuW(menu,MF_STRING|(controlDatabase.path.empty()?MF_GRAYED:0),UNLOAD_DATABASE,L"Отключить контрольную базу");
    RECT r;GetWindowRect(mainControl(OPTIONS),&r);int selected=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_LEFTALIGN|TPM_TOPALIGN,r.left,r.bottom,0,mainWin,nullptr);DestroyMenu(menu);
    if(selected==1)layerSettings();else if(selected==2)settings();else if(selected==3)computeSettings();else if(selected>=LOAD_DATABASE)v2Command(selected);
}
