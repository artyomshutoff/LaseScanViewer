from pathlib import Path
r=Path(__file__).resolve().parents[1]
p=r/'src/manual_volume_ui.hpp';s=p.read_text(encoding='utf-8')
s=s.replace('std::array<HWND,3> headings{};','''std::array<HWND,3> headings{};
 int lineHeight=24,rowHeight=40,tableY=120,tableHeight=200,statsY=380,dimensionsY=474,compareY=510,compareHeight=116,footerY=640,clientHeight=692;
 std::wstring summary;''')
s=s.replace('MANUAL_UNITS=713;','MANUAL_UNITS=713,MANUAL_INFO=714;')
s=s.replace('int total=int(d.widgets.size())*38;','int total=int(d.widgets.size())*d.rowHeight;')
s=s.replace('int y=int(i)*38-d.scroll;','int y=int(i)*d.rowHeight-d.scroll;')
s=s.replace('MoveWindow(row.number,8,y+3,34,30,TRUE);','MoveWindow(row.number,8,y+3,34,d.rowHeight-8,TRUE);')
s=s.replace('y+2,140,30,TRUE','y+2,140,d.rowHeight-8,TRUE').replace('y+3,128,30,TRUE','y+3,128,d.rowHeight-8,TRUE').replace('y+2,64,30,TRUE','y+2,64,d.rowHeight-8,TRUE')
s=s.replace('RECT cards{20,427,750,511};','d.summary=summary;RECT cards{20,d.statsY,750,d.footerY};')
s=s.replace('SetWindowTextW(GetDlgItem(d.window,MANUAL_RESULTS),summary.c_str());','')
s=s.replace('text=L"Заполнено: "+std::to_wstring(v.count)+L" из "+std::to_wstring(d.rows.size())+L" обмеров. Пустые строки не учитываются.";','text=L"Заполнено "+std::to_wstring(v.count)+L" из "+std::to_wstring(d.rows.size())+L" · пустые строки не учитываются";')
s=s.replace('int top=index*38;','int top=index*d->rowHeight;').replace('top+38','top+d->rowHeight').replace('*114','*d->rowHeight*3').replace('d->scroll-=38','d->scroll-=d->rowHeight').replace('d->scroll+=38','d->scroll+=d->rowHeight').replace('int(d->rows.size())*38','int(d->rows.size())*d->rowHeight')
s=s.replace('auto title=manualControl(d.window', 'auto title=manualControl(d.window') # creation is replaced below
a=s.index('void manualPaintCards(');b=s.index('void manualDialogSelfTest(ManualDialog& d);',a)
s=s[:a]+'''void manualPaintCards(ManualDialog& d,HDC dc){
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
 label(dc,{166,d.footerY,602,d.footerY+42},L"Обмеры: прямоугольный кузов.\\nПо скану: объём груза, а не вместимость кузова.",muted,smallFont,DT_LEFT|DT_WORDBREAK);
}
void manualMeasureLayout(ManualDialog& d){
 HDC dc=GetDC(mainWin);auto previous=SelectObject(dc,font);TEXTMETRICW metric{};GetTextMetricsW(dc,&metric);SelectObject(dc,previous);ReleaseDC(mainWin,dc);
 d.lineHeight=std::max(24,int(metric.tmHeight)+4);d.rowHeight=d.lineHeight+16;d.tableHeight=d.rowHeight*5;
 d.statsY=d.tableY+d.tableHeight+60;d.dimensionsY=d.statsY+2*d.lineHeight+46;
 d.compareY=d.dimensionsY+d.lineHeight+12;d.compareHeight=3*d.lineHeight+44;d.footerY=d.compareY+d.compareHeight+14;d.clientHeight=d.footerY+52;
}
''' +s[b:]
s=s.replace('if(id==MANUAL_UNITS&&HIWORD(wp)', '''if(id==MANUAL_INFO){MessageBoxW(w,L"Объём обмера = длина × ширина × высота.\\nСредний объём — среднее объёмов заполненных строк.\\n\\nРазница = объём по скану − средний объём обмеров.\\nПроцент = разница / средний объём обмеров × 100.\\n\\nОбмеры оценивают прямоугольный кузов. Объём груза по скану может отличаться от его вместимости.",L"Как считается сравнение",MB_ICONINFORMATION);return 0;}
   if(id==MANUAL_UNITS&&HIWORD(wp)''')
s=s.replace('RECT parent;GetWindowRect(mainWin,&parent);RECT area{0,0,770,650};','manualMeasureLayout(d);RECT parent;GetWindowRect(mainWin,&parent);RECT area{0,0,770,d.clientHeight};')
s=s.replace('parent.left+50,parent.top+25,area.right-area.left,area.bottom-area.top','parent.left+std::max(0L,(parent.right-parent.left-(area.right-area.left))/2),parent.top+std::max(0L,(parent.bottom-parent.top-(area.bottom-area.top))/2),area.right-area.left,area.bottom-area.top')
a=s.index(' auto title=manualControl');b=s.index(' manualRebuild(d);EnableWindow',a)
s=s[:a]+''' auto units=manualControl(d.window,L"COMBOBOX",L"",MANUAL_UNITS,550,42,200,150,CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS|WS_TABSTOP|WS_VSCROLL);
 SendMessageW(units,CB_SETITEMHEIGHT,WPARAM(-1),d.lineHeight+6);SendMessageW(units,CB_SETITEMHEIGHT,0,d.lineHeight+6);
 SendMessageW(units,CB_ADDSTRING,0,(LPARAM)L"Метры (м)");SendMessageW(units,CB_ADDSTRING,0,(LPARAM)L"Сантиметры (см)");SendMessageW(units,CB_SETCURSEL,d.centimetres,0);
 const wchar_t* titles[]={L"№",L"Длина, м",L"Ширина, м",L"Высота, м",L"Объём, м³"};int positions[]={28,70,224,378,536};
 for(int j=0;j<5;++j){auto h=manualControl(d.window,L"STATIC",titles[j],0,positions[j],d.tableY-d.lineHeight-4,j?140:34,d.lineHeight);if(j>=1&&j<=3){d.headings[j-1]=h;if(d.centimetres)SetWindowTextW(h,(std::wstring(titles[j]).substr(0,std::wstring(titles[j]).size()-1)+L"см").c_str());}}
 d.table=CreateWindowExW(WS_EX_CONTROLPARENT,L"LaseManualVolumeTable",L"",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_VSCROLL,20,d.tableY,738,d.tableHeight,d.window,nullptr,inst,&d);
 manualControl(d.window,L"BUTTON",L"+ Добавить обмер",MANUAL_ADD,20,d.tableY+d.tableHeight+12,190,36,BS_OWNERDRAW|WS_TABSTOP);
 auto status=manualControl(d.window,L"STATIC",L"",MANUAL_STATUS,230,d.tableY+d.tableHeight+18,520,d.lineHeight+4,SS_LEFTNOWORDWRAP);SendMessageW(status,WM_SETFONT,(WPARAM)smallFont,TRUE);
 manualControl(d.window,L"BUTTON",L"Как считается",MANUAL_INFO,20,d.footerY,128,36,BS_OWNERDRAW|WS_TABSTOP);
 manualControl(d.window,L"BUTTON",L"Закрыть",IDOK,626,d.footerY,124,36,BS_OWNERDRAW|WS_TABSTOP);
''' +s[b:]
s=s.replace('wchar_t summary[2048]{};GetWindowTextW(GetDlgItem(d.window,MANUAL_RESULTS),summary,2048);if(std::wstring(summary).find(num(*scan,3))','if(d.summary.find(num(*scan,3))')
# Both parent and table deliver owner-draw notifications.
s=s.replace('bool enabled=IsWindowEnabled(item->hwndItem);auto r=item->rcItem;', '''bool enabled=IsWindowEnabled(item->hwndItem);auto r=item->rcItem;
 if(item->CtlType==ODT_COMBOBOX){fill(item->hDC,r,(item->itemState&ODS_SELECTED)?activeBg:card);wchar_t text[80]{};if(item->itemID!=UINT(-1))SendMessageW(item->hwndItem,CB_GETLBTEXT,item->itemID,(LPARAM)text);r.left+=10;label(item->hDC,r,text,ink,font);return;}''')
s=s.replace('msg==WM_CTLCOLORSTATIC)return manualColors','msg==WM_CTLCOLORSTATIC||msg==WM_CTLCOLORLISTBOX)return manualColors')
s=s.replace('msg==WM_CTLCOLOREDIT?card:bg','(msg==WM_CTLCOLOREDIT||msg==WM_CTLCOLORLISTBOX)?card:bg')
s=s.replace('if(msg==WM_PAINT){PAINTSTRUCT','if(msg==WM_MEASUREITEM){auto item=(MEASUREITEMSTRUCT*)lp;if(item->CtlType==ODT_COMBOBOX){item->itemHeight=d->lineHeight+6;return TRUE;}}\n  if(msg==WM_PAINT){PAINTSTRUCT')
# Snapshot the problematic partially filled scenario in both themes.
s=s.replace('capture(L"manual-calculator.png");','''capture(L"manual-calculator.png");
  auto savedRows=d.rows;d.rows={{L"5.05",L"2.31",L"1.36"},{L"5.02",L"2.3",L"1.35"},{},{},{}};manualRebuild(d);capture(L"manual-partial.png");
  if(std::abs(d.result.mean-15.72609)>1e-6)throw std::runtime_error("Partial measurements changed volume");
  bool partialTheme=lightTheme;applyTheme(true);RedrawWindow(d.window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);capture(L"manual-partial-light.png");applyTheme(partialTheme);d.rows=savedRows;manualRebuild(d);''')
p.write_text(s,encoding='utf-8')
for name in ['src/main.cpp','src/v2.hpp','app.rc','README.md','README-LaseScanViewer.md']:
 p=r/name;s=p.read_text(encoding='utf-8').replace('3.29.0','3.30.0').replace('3,29,0,0','3,30,0,0');p.write_text(s,encoding='utf-8')
print('Updated manual calculator layout 3.30')
