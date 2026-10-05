#pragma once
struct ComputeDialog {bool done=false,accepted=false;compute::Backend selected=compute::Backend::CPU;int test=0;};
LRESULT CALLBACK ComputeProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 auto d=(ComputeDialog*)GetWindowLongPtrW(w,GWLP_USERDATA);if(msg==WM_NCCREATE){d=(ComputeDialog*)((CREATESTRUCTW*)lp)->lpCreateParams;SetWindowLongPtrW(w,GWLP_USERDATA,(LONG_PTR)d);}
 if(msg==WM_TIMER&&d->test){KillTimer(w,1);CheckRadioButton(w,401,402,d->test!=3&&compute::runtime().ready?402:401);SendMessageW(w,WM_COMMAND,d->test==2?IDCANCEL:IDOK,0);return 0;}
 if(msg==WM_COMMAND){if(LOWORD(wp)==IDOK){d->selected=IsDlgButtonChecked(w,402)==BST_CHECKED?compute::Backend::GPU:compute::Backend::CPU;d->accepted=true;DestroyWindow(w);return 0;}if(LOWORD(wp)==IDCANCEL){DestroyWindow(w);return 0;}}
 if(msg==WM_CLOSE){DestroyWindow(w);return 0;}if(msg==WM_DESTROY){d->done=true;return 0;}return DefWindowProcW(w,msg,wp,lp);
}
void computeSettings(int test=0){
 if(busy)return;auto& gpu=compute::runtime();ComputeDialog d;d.selected=compute::selected.load();d.test=test;
 HINSTANCE inst=GetModuleHandleW(nullptr);WNDCLASSW wc{};wc.hInstance=inst;wc.lpfnWndProc=ComputeProc;wc.lpszClassName=L"LaseComputeSettings";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);RegisterClassW(&wc);
 RECT parent;GetWindowRect(mainWin,&parent);auto w=CreateWindowExW(WS_EX_DLGMODALFRAME,L"LaseComputeSettings",L"Устройство вычислений",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,parent.left+100,parent.top+60,650,390,mainWin,nullptr,inst,&d);
 auto control=[&](const wchar_t* cls,const std::wstring& text,int id,int x,int y,int width,int height,DWORD style=0){auto c=CreateWindowW(cls,text.c_str(),WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,(HMENU)(INT_PTR)id,inst,nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)font,TRUE);return c;};
 control(L"BUTTON",L"CPU · все вычисления на процессоре",401,20,20,590,30,BS_AUTORADIOBUTTON|WS_TABSTOP|WS_GROUP);
 auto choice=control(L"BUTTON",L"GPU + CPU · поиск соответствий на видеокарте",402,20,62,590,30,BS_AUTORADIOBUTTON|WS_TABSTOP);EnableWindow(choice,gpu.ready);
 CheckRadioButton(w,401,402,d.selected==compute::Backend::GPU&&gpu.ready?402:401);
 control(L"STATIC",gpu.ready?L"Видеокарта: "+widen(gpu.name):L"GPU недоступен: "+widen(gpu.reason),0,24,106,590,52);
 control(L"STATIC",L"GPU помогает совмещению сканов. Интегрирование объёма и точное\nуточнение остаются на CPU. GPU использует двойную точность.\nНа небольших облаках CPU может быть быстрее.\nПри ошибке драйвера поиск продолжится на CPU.\nВыбор применяется к следующему совмещению.",0,24,164,590,112);
 control(L"BUTTON",L"Применить",IDOK,340,300,130,36,BS_DEFPUSHBUTTON|WS_TABSTOP);control(L"BUTTON",L"Отмена",IDCANCEL,480,300,130,36,BS_PUSHBUTTON|WS_TABSTOP);
 if(test)SetTimer(w,1,50,nullptr);EnableWindow(mainWin,FALSE);skinSettings(w);ShowWindow(w,SW_SHOW);MSG msg;while(!d.done&&GetMessageW(&msg,nullptr,0,0)>0)if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}EnableWindow(mainWin,TRUE);SetActiveWindow(mainWin);
 if(d.accepted){compute::choose(d.selected);status(d.selected==compute::Backend::GPU?L"Вычисления: GPU + CPU · "+widen(gpu.name):L"Вычисления: CPU");redraw();}
}
