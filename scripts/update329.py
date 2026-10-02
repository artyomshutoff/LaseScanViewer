from pathlib import Path
r=Path(__file__).resolve().parents[1]
def edit(name,old,new):
 p=r/name;s=p.read_text(encoding='utf-8');assert old in s,(name,old[:60]);p.write_text(s.replace(old,new),encoding='utf-8')
edit('src/main.cpp','button(LOAD_DATABASE,L"Загрузить SQ3",20,606,126,34);button(MANUAL_CALC,L"Обмеры кузова",154,606,126,34);','button(LOAD_DATABASE,L"База SQ3",20,606,78,34);button(MANUAL_CALC,L"Обмеры кузова",106,606,174,34);')
for name in ['src/main.cpp','src/v2.hpp','app.rc']:
 p=r/name;s=p.read_text(encoding='utf-8').replace('3.28.0','3.29.0').replace('3,28,0,0','3,29,0,0');p.write_text(s,encoding='utf-8')
edit('src/manual_volume_ui.hpp','std::vector<manualVolume::Row> manualMeasurements(5);','std::vector<manualVolume::Row> manualMeasurements(5);\nbool manualCentimetres=false;')
edit('src/manual_volume_ui.hpp','int scroll=0,test=0;','int scroll=0,test=0;bool centimetres=false;\n std::array<HWND,3> headings{};')
edit('src/manual_volume_ui.hpp','MANUAL_STATUS=712;','MANUAL_STATUS=712,MANUAL_UNITS=713;')
edit('src/manual_volume_ui.hpp','manualVolume::calculate({d.rows[i]})','manualVolume::calculate({d.rows[i]},d.centimetres?.01:1)')
edit('src/manual_volume_ui.hpp','manualVolume::calculate(d.rows);','manualVolume::calculate(d.rows,d.centimetres?.01:1);')
edit('src/manual_volume_ui.hpp','L": введите три положительных размера в метрах."','L": введите три положительных размера."')
edit('src/manual_volume_ui.hpp','summary=L"Средний объём: "+num(v.mean,3)+L" м³\\nМинимальный: "+num(v.minimum,3)+L" м³     Максимальный: "+num(v.maximum,3)+L" м³\\nСредние размеры (Д × Ш × В): "','summary=L"Средние размеры (Д × Ш × В): "')
edit('src/manual_volume_ui.hpp','for(auto& row:d.widgets)EnableWindow(row.remove,d.rows.size()>1);','for(auto& row:d.widgets)EnableWindow(row.remove,d.rows.size()>1);\n RECT cards{20,427,750,511};InvalidateRect(d.window,&cards,TRUE);')
edit('src/manual_volume_ui.hpp','void manualDialogSelfTest(ManualDialog& d);', '''void manualChangeUnits(ManualDialog& d,bool centimetres){
 manualUpdate(d);if(centimetres==d.centimetres)return;
 // Partial rows are allowed. Refuse the switch only for nonempty invalid fields.
 auto rows=d.rows;
 for(auto& row:rows)for(auto& text:row)if(!manualVolume::blank(text)){
  auto value=manualVolume::dimension(text);if(!value){SendDlgItemMessageW(d.window,MANUAL_UNITS,CB_SETCURSEL,d.centimetres,0);MessageBoxW(d.window,L"Исправьте некорректный размер перед сменой единиц.",L"Единицы измерения",MB_ICONINFORMATION);return;}
  double converted=*value*(centimetres?100:.01);if(!std::isfinite(converted)||converted<=0){SendDlgItemMessageW(d.window,MANUAL_UNITS,CB_SETCURSEL,d.centimetres,0);return;}
  // Fixed notation remains accepted by the input parser, including tiny values.
  std::wostringstream out;out.imbue(std::locale::classic());out<<std::fixed<<std::setprecision(std::max(0,16-int(std::floor(std::log10(converted)))))<<converted;text=out.str();
  if(text.find(L'.')!=std::wstring::npos){while(text.back()==L'0')text.pop_back();if(text.back()==L'.')text.pop_back();}
 }
 d.centimetres=centimetres;manualCentimetres=centimetres;d.rows=std::move(rows);
 const wchar_t* names[]={L"Длина",L"Ширина",L"Высота"};for(int j=0;j<3;++j)SetWindowTextW(d.headings[j],(std::wstring(names[j])+(centimetres?L", см":L", м")).c_str());
 manualRebuild(d);
}
void manualPaintCards(ManualDialog& d,HDC dc){
 const wchar_t* titles[]={L"СРЕДНИЙ ОБЪЁМ",L"МИНИМАЛЬНЫЙ",L"МАКСИМАЛЬНЫЙ"};double values[]={d.result.mean,d.result.minimum,d.result.maximum};
 for(int i=0;i<3;++i){int x=20+i*248;roundBox(dc,{x,427,x+234,511},i==0?activeBg:card,border);label(dc,{x+14,434,x+220,455},titles[i],muted,smallFont);label(dc,{x+14,459,x+220,499},d.result.count&&d.result.error.empty()?num(values[i],3)+L" м³":L"—",i==0?accent:ink,numberFont);}
}
void manualDialogSelfTest(ManualDialog& d);''')
edit('src/manual_volume_ui.hpp','if(id==MANUAL_ADD&&d->rows.size()<200)', 'if(id==MANUAL_UNITS&&HIWORD(wp)==CBN_SELCHANGE){manualChangeUnits(*d,SendMessageW(GetDlgItem(w,MANUAL_UNITS),CB_GETCURSEL,0,0)==1);return 0;}\n   if(id==MANUAL_ADD&&d->rows.size()<200)')
edit('src/manual_volume_ui.hpp','if(msg==WM_TIMER&&d->test)', 'if(msg==WM_PAINT){PAINTSTRUCT ps;auto dc=BeginPaint(w,&ps);manualPaintCards(*d,dc);EndPaint(w,&ps);return 0;}\n  if(msg==WM_TIMER&&d->test)')
edit('src/manual_volume_ui.hpp','d.rows=manualMeasurements;d.test=test;','d.rows=manualMeasurements;d.test=test;d.centimetres=manualCentimetres;')
edit('src/manual_volume_ui.hpp','manualControl(d.window,L"STATIC",L"Введите размеры в метрах. Для каждого обмера V = длина × ширина × высота.",0,20,14,730,28);','''auto title=manualControl(d.window,L"STATIC",L"Обмеры кузова",0,20,14,480,32);SendMessageW(title,WM_SETFONT,(WPARAM)numberFont,TRUE);
 manualControl(d.window,L"STATIC",L"Объём каждой строки = длина × ширина × высота",0,20,53,490,24);
 manualControl(d.window,L"STATIC",L"Единицы ввода",0,550,14,190,22);
 auto units=manualControl(d.window,L"COMBOBOX",L"",MANUAL_UNITS,550,40,200,150,CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL);
 SendMessageW(units,CB_ADDSTRING,0,(LPARAM)L"Метры (м)");SendMessageW(units,CB_ADDSTRING,0,(LPARAM)L"Сантиметры (см)");SendMessageW(units,CB_SETCURSEL,d.centimetres,0);''')
edit('src/manual_volume_ui.hpp','for(int j=0;j<5;++j)manualControl(d.window,L"STATIC",titles[j],0,positions[j],50,j?140:34,24);','for(int j=0;j<5;++j){auto h=manualControl(d.window,L"STATIC",titles[j],0,positions[j],82,j?140:34,24);if(j>=1&&j<=3){d.headings[j-1]=h;if(d.centimetres)SetWindowTextW(h,(std::wstring(titles[j]).substr(0,std::wstring(titles[j]).size()-1)+L"см").c_str());}}')
edit('src/manual_volume_ui.hpp',',20,78,738,266,d.window',',20,110,738,234,d.window')
edit('src/manual_volume_ui.hpp','MANUAL_RESULTS,20,432,730,142','MANUAL_RESULTS,20,520,730,58')
edit('src/manual_volume_ui.hpp','d.rows.assign(5,{});manualRebuild(d);','d.centimetres=false;manualCentimetres=false;SendDlgItemMessageW(d.window,MANUAL_UNITS,CB_SETCURSEL,0,0);d.rows.assign(5,{});manualRebuild(d);')
edit('src/manual_volume_ui.hpp','capture(L"manual-calculator.png");','''capture(L"manual-calculator.png");
  SendDlgItemMessageW(d.window,MANUAL_UNITS,CB_SETCURSEL,1,0);SendMessageW(d.window,WM_COMMAND,MAKEWPARAM(MANUAL_UNITS,CBN_SELCHANGE),0);
  if(!d.centimetres||std::abs(d.result.mean-12)>1e-10||manualVolume::dimension(d.rows[0][0])!=500)throw std::runtime_error("Centimetre conversion failed");
  capture(L"manual-centimetres.png");bool themeBefore=lightTheme;applyTheme(true);RedrawWindow(d.window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);capture(L"manual-light.png");applyTheme(themeBefore);
  SendDlgItemMessageW(d.window,MANUAL_UNITS,CB_SETCURSEL,0,0);SendMessageW(d.window,WM_COMMAND,MAKEWPARAM(MANUAL_UNITS,CBN_SELCHANGE),0);
  if(d.centimetres||std::abs(d.result.mean-12)>1e-10||manualVolume::dimension(d.rows[0][0])!=5)throw std::runtime_error("Metre conversion failed");''')
edit('src/main.cpp','drawControlComparison();','drawBedDimensions();drawControlComparison();')
edit('src/v2_state.hpp','#include "water_fill.hpp"','#include "water_fill.hpp"\n#include "bed_dimensions.hpp"')
edit('src/v2_state.hpp','WaterFill water;bool showWater','BedDimensions bedDimensions;\nWaterFill water;bool showWater')
edit('src/v2_state.hpp','cargo={};water={};','cargo={};water={};bedDimensions={};')
edit('src/v2_state.hpp','void drawControlComparison();','void drawControlComparison();\nvoid drawBedDimensions();')
edit('src/v2.hpp','water=std::move(result.water);','water=std::move(result.water);bedDimensions=measureBed(water);')
print('Updated 3.29 UI')
