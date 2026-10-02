#pragma once
#include <cwchar>
#include <locale>

std::string utf8(const std::wstring& s){int n=WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),nullptr,0,nullptr,nullptr);std::string out(n,0);WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),out.data(),n,nullptr,nullptr);return out;}
std::wstring num(double v,int precision=3){std::wostringstream s;s.imbue(std::locale::classic());s<<std::fixed<<std::setprecision(precision)<<v;return s.str();}
#include "control_ui.hpp"
#include "manual_volume_ui.hpp"

std::wstring waterSummary(){
    if(!water.available)return L"По бортам: "+widen(water.note);
    double k=comparison?std::pow(comparison->options.metresPerUnit,3):0;
    if(k<=0)return L"По бортам: выберите единицы координат";
    double load=(cargo.calibrated?cargo.calibratedVolume:cargo.volume);
    return L"По бортам (вода): "+num(water.volume*k,2)+L" м³   ·   Груз: "+num(load/water.volume*100,1)+L"%   ·   Разница: "+num((water.volume-load)*k,2)+L" м³";
}
void updateFileLists(){
    for(int id:{ACTIVE_FILE,BASE_FILE})SendMessageW(GetDlgItem(mainWin,id),CB_RESETCONTENT,0,0);
    SendMessageW(GetDlgItem(mainWin,BASE_FILE),CB_ADDSTRING,0,(LPARAM)L"Пустой кузов: Empty или Reference");
    for(const auto& d:documents){std::wstring name=(d.data.kind==2?L"[Full] ":d.data.kind==3?L"[Ref] ":L"[Empty] ")+d.path.filename().wstring()+L" · "+widen(d.data.label)+L" · набор "+std::to_wstring(d.data.selectedType);for(int id:{ACTIVE_FILE,BASE_FILE}){std::wstring entry=(id==ACTIVE_FILE?L"С грузом: ":L"Пустой: ")+name;SendMessageW(GetDlgItem(mainWin,id),CB_ADDSTRING,0,(LPARAM)entry.c_str());}}
    SendMessageW(GetDlgItem(mainWin,ACTIVE_FILE),CB_SETCURSEL,activeDocument,0);SendMessageW(GetDlgItem(mainWin,BASE_FILE),CB_SETCURSEL,baseDocument+1,0);
}
void selectDocument(int index){
    if(index<0||index>=int(documents.size())||busy)return;
    compareOptions.aligned=false;compareOptions.provisional=false;compareOptions.alignmentOverlap=-1;compareOptions.rimHeightAdjustment=0;compareOptions.bedHeightAdjustment=compareOptions.bedHeightSpread=0;compareOptions.bedHeightVariants=0;compareOptions.angle=compareOptions.dx=compareOptions.dy=compareOptions.dz=0;activeDocument=index;model=documents[index].data;loadedPath=documents[index].path;region={};selectRegion=false;invalidateComparison();compileLists();updateFileLists();reset();SetWindowTextW(mainWin,(std::wstring(applicationTitle)+L" — "+loadedPath.filename().wstring()).c_str());status(L"Активный скан: "+loadedPath.filename().wstring());
}
void acceptLoaded(Model next){
    size_t total=next.points.size();for(size_t i=0;i<documents.size();i++)if(!replacing||int(i)!=activeDocument)total+=documents[i].data.points.size();
    if(total>5000000)throw std::runtime_error("Session limit: 5 million points; close unused files");
    if(replacing&&activeDocument>=0){documents[activeDocument]={pendingPath,std::move(next)};selectDocument(activeDocument);}
    else {if(documents.size()>=12)throw std::runtime_error("Session limit: 12 files; close unused files");int old=activeDocument;documents.push_back({pendingPath,std::move(next)});selectDocument(int(documents.size())-1);if(baseDocument<0&&old>=0)baseDocument=old;}
    if(loadQueue.empty()){
        for(int f=int(documents.size())-1;f>=0;--f)if(documents[f].data.kind==2){int base=-1;for(int j=int(documents.size())-1;j>=0;--j)if(j!=f&&documents[j].data.kind==1&&documents[j].data.scan==documents[f].data.scan){base=j;break;}if(base<0&&documents.size()==2&&documents[1-f].data.kind!=2)base=1-f;if(base>=0){selectDocument(f);baseDocument=base;compileLists();break;}}
    }
    updateFileLists();
}
void enqueueFiles(const std::vector<std::filesystem::path>& paths){
    auto expanded=paths;
    if(paths.size()==1&&renderTestDir.empty()){auto name=paths.front().filename().wstring();auto at=name.find(L"_Full.bin");if(at!=std::wstring::npos){name.replace(at,9,L"_Empty.bin");auto empty=paths.front().parent_path()/name;bool opened=false;for(auto& d:documents)if(d.path==empty)opened=true;if(!opened&&std::filesystem::exists(empty))expanded.push_back(empty);}}
    for(auto p:expanded){if(loadQueue.size()+documents.size()>=12){status(L"Можно открыть до 12 файлов. Закройте лишние сканы.");break;}loadQueue.push_back(p);}
    if(!busy&&!loadQueue.empty()){auto p=loadQueue.front();loadQueue.pop_front();startLoad(p);}
}
void applyRegion(const Region& r){
    if(activeDocument<0)return;auto next=cropped(documents[activeDocument].data,r);model=std::move(next);region=r;invalidateComparison();compileLists();redraw();
}
void finishRegion(){
    selectRegion=false;
    if(std::abs(selectionEnd.x-selectionStart.x)<4||std::abs(selectionEnd.y-selectionStart.y)<4){status(L"Область не выбрана: протяните прямоугольник мышью.");redraw();return;}
    auto a=model.lo,b=model.hi;double span=std::max({b.x-a.x,b.y-a.y,b.z-a.z,1.f});
    auto world=[&](POINT p){return std::pair<double,double>{((2.*p.x/viewW-1)*orthoHeight*viewW/viewH-panX)*span/2+(a.x+b.x)/2,((1-2.*p.y/viewH)*orthoHeight-panY)*span/2+(a.y+b.y)/2};};
    auto p=world(selectionStart),q=world(selectionEnd);Region r{true,std::min(p.first,q.first),std::max(p.first,q.first),std::min(p.second,q.second),std::max(p.second,q.second),-1e12,1e12};
    try{applyRegion(r);status(L"Область применена. «Сброс ROI» возвращает все точки.");}catch(const std::exception& e){MessageBoxW(mainWin,widen(e.what()).c_str(),L"Область",MB_ICONINFORMATION);redraw();}
}
std::wstring comparisonSummary(){
    if(!comparison&&compareOptions.alignmentOverlap>=0&&!compareOptions.aligned)return L"Совмещение ненадёжно ("+num(compareOptions.alignmentOverlap*100,1)+L"%). Нажмите «Рассчитать» для предварительной оценки";
    if(!comparison)return region.enabled?L"Область выделена · "+grouped(model.points.size())+L" точек":L"Выберите активный и базовый сканы для сравнения";
    if(cargoView){double scale=comparison->options.metresPerUnit;if(cargo.calibrated)return std::wstring(comparison->options.provisional?L"Предварительная оценка: ":L"Оценка объёма: ")+num(cargo.calibratedVolume*std::pow(scale,3),2)+L" м³";return scale>0?(comparison->options.provisional?L"Предварительно: ":L"Объём груза: ")+num(cargo.volume*std::pow(scale,3),2)+L" м³"+(scale==.001?L" (при координатах в мм)":L"")+(comparison->options.provisional?L" · Совпадение "+num(comparison->options.alignmentOverlap*100,1)+L"%":L"  ·  Оценка по общей области"):L"Для отображения м³ выберите единицы в настройках";}
    auto& c=*comparison;double k=c.options.metresPerUnit>0?std::pow(c.options.metresPerUnit,3):1;std::wstring unit=c.options.metresPerUnit>0?L" м³":L" ед.³";
    return L"Добавлено "+num(c.positive*k)+unit+L"  ·  Убрано "+num(c.negative*k)+unit+L"  ·  Покрытие "+num(c.coverage()*100,1)+L"%";
}
void v2Layout(){
    RECT r;GetClientRect(mainWin,&r);int width=std::max(100,int(r.right)-340),half=(width-10)/2;
    MoveWindow(GetDlgItem(mainWin,ACTIVE_FILE),320,81,half,320,TRUE);MoveWindow(GetDlgItem(mainWin,BASE_FILE),330+half,81,half,320,TRUE);
    int x=320;for(auto pair:{std::pair<int,int>{COMPARE,100},{AUTO_ALIGN,112},{OVERLAY,96},{REGION_SELECT,78},{REGION_CLEAR,66},{OPTIONS,94},{PNG_SAVE,62},{REPORT_SAVE,76}}){MoveWindow(GetDlgItem(mainWin,pair.first),x,124,pair.second,34,TRUE);x+=pair.second+8;}
}
void createV2Controls(HINSTANCE inst){
    for(int id:{ACTIVE_FILE,BASE_FILE}){HWND b=CreateWindowW(L"COMBOBOX",L"",WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS|WS_VSCROLL,320,81,300,320,mainWin,(HMENU)(INT_PTR)id,inst,nullptr);SendMessageW(b,WM_SETFONT,(WPARAM)font,TRUE);SendMessageW(b,CB_SETDROPPEDWIDTH,520,0);SendMessageW(b,CB_SETITEMHEIGHT,WPARAM(-1),28);SetWindowSubclass(b,PickerSkinProc,1,0);}
    const std::pair<int,const wchar_t*> buttons[]={{COMPARE,L"Рассчитать"},{AUTO_ALIGN,L"Совместить"},{OVERLAY,L"Сравнить"},{REGION_SELECT,L"Область"},{REGION_CLEAR,L"Сброс"},{OPTIONS,L"Настройки"},{PNG_SAVE,L"PNG"},{REPORT_SAVE,L"Отчёт"}};
    for(auto [id,text]:buttons){auto b=CreateWindowW(L"BUTTON",text,WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,0,124,80,34,mainWin,(HMENU)(INT_PTR)id,inst,nullptr);SendMessageW(b,WM_SETFONT,(WPARAM)font,TRUE);SetWindowSubclass(b,ButtonSkinProc,1,0);}
    HWND tips=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP,0,0,0,0,mainWin,nullptr,inst,nullptr);SendMessageW(tips,TTM_SETMAXTIPWIDTH,0,360);
    for(auto [id,text]:std::initializer_list<std::pair<int,const wchar_t*>>{{GRID,L"Показать или скрыть границы зоны интереса, линии и подписи X, Y, Z. Камера и расчёт сохраняются."},{OPEN,L"Откройте Full. Файл Empty с тем же номером в этой папке загрузится автоматически. Можно выбрать несколько BIN."},{COMPARE,L"Совместить сканы и оценить объём. После расчёта проверьте, что выбран только груз."},{AUTO_ALIGN,L"Расширенный поиск 0–360° по скану и верхним частям кузова, затем точное уточнение. Может занять время; ход работы показан внизу. Проверьте совпадение бортов."},{OVERLAY,L"Оранжевый: Full. Голубой: Empty. Проверьте неподвижные части кузова."},{REGION_SELECT,L"Вид сверху: выделите кузов мышью. Восстановление пропусков можно отключить в настройках."},{OPTIONS,L"Слои просмотра: Full, Empty, груз, серый фон и две заливки по воде. Параметры расчёта доступны отдельным пунктом меню."},{CONTEXT_POINTS,L"Показать остальные точки исходного Full серыми, включая точки вне области расчёта. Груз остаётся цветным; м³ не меняются."},{CARGO,L"Показать выделенный груз или полную карту разности высот."},{REPORT_SAVE,L"Сохранить PDF или HTML с изображением, объёмом, покрытием и параметрами расчёта."}}){TOOLINFOW tool{};tool.cbSize=sizeof(tool);tool.uFlags=TTF_IDISHWND|TTF_SUBCLASS;tool.hwnd=mainWin;tool.uId=(UINT_PTR)GetDlgItem(mainWin,id);tool.lpszText=const_cast<wchar_t*>(text);SendMessageW(tips,TTM_ADDTOOLW,0,(LPARAM)&tool);}

}

struct SettingsDialog {double threshold=50;bool largest=true,clean=true;HWND w=nullptr;bool done=false,accepted=false;Region roi;CompareOptions options;};
LRESULT CALLBACK SettingsProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
    auto d=(SettingsDialog*)GetWindowLongPtrW(w,GWLP_USERDATA);
    if(msg==WM_NCCREATE){d=(SettingsDialog*)((CREATESTRUCTW*)lp)->lpCreateParams;SetWindowLongPtrW(w,GWLP_USERDATA,(LONG_PTR)d);}
    if(msg==WM_COMMAND&&LOWORD(wp)==IDOK){try{
        auto number=[&](int id){wchar_t b[128]{};GetDlgItemTextW(w,id,b,128);std::wstring s=b;std::replace(s.begin(),s.end(),L',',L'.');wchar_t* end=nullptr;double value=wcstod(s.c_str(),&end);while(end&&iswspace(*end))++end;if(end==s.c_str()||!end||*end||!std::isfinite(value))throw std::runtime_error("Enter valid numbers in all fields");return value;};
        CompareOptions o=d->options;o.step=number(201);int units=int(SendDlgItemMessageW(w,202,CB_GETCURSEL,0,0));o.metresPerUnit=units==1?.001:units==2?.01:units==3?1:0;
        o.calibratedEstimate=IsDlgButtonChecked(w,222)==BST_CHECKED;o.adaptiveGrid=IsDlgButtonChecked(w,221)==BST_CHECKED;o.reconstructGaps=IsDlgButtonChecked(w,220)==BST_CHECKED;o.estimator=int(SendDlgItemMessageW(w,203,CB_GETCURSEL,0,0));o.dx=number(204);o.dy=number(205);o.dz=number(206);o.angle=number(218);o.aligned=IsDlgButtonChecked(w,207)==BST_CHECKED;if(o.aligned&&!d->options.aligned&&o.alignmentOverlap>=0)o.provisional=true;
        Region roi{IsDlgButtonChecked(w,208)==BST_CHECKED,number(210),number(211),number(212),number(213),number(214),number(215)};roi.validate();if(o.step<=0)throw std::runtime_error("Cell size must be positive");
        if(roi.enabled&&activeDocument>=0){bool any=false;for(auto p:documents[activeDocument].data.points)if(roi.contains(p)){any=true;break;}if(!any)throw std::runtime_error("The region contains no active points");}
        double threshold=number(216);if(threshold<0)throw std::runtime_error("Порог должен быть неотрицательным");d->clean=IsDlgButtonChecked(w,219)==BST_CHECKED;d->threshold=threshold;d->largest=IsDlgButtonChecked(w,217)==BST_CHECKED;d->roi=roi;d->options=o;d->accepted=true;DestroyWindow(w);
    }catch(const std::exception& e){MessageBoxW(w,widen(e.what()).c_str(),L"Проверьте параметры",MB_ICONEXCLAMATION);}return 0;}
    if(msg==WM_CLOSE||(msg==WM_COMMAND&&LOWORD(wp)==IDCANCEL)){DestroyWindow(w);return 0;}
    if(msg==WM_DESTROY){d->done=true;return 0;}
    return DefWindowProcW(w,msg,wp,lp);
}
bool settings(){
    if(activeDocument<0)return false;SettingsDialog d;d.threshold=cargoThreshold;d.largest=cargoLargest;d.clean=cargoClean;d.options=compareOptions;d.roi=region.enabled?region:bounds(documents[activeDocument].data);if(!region.enabled){d.roi.enabled=false;d.roi.z0=-1e12;d.roi.z1=1e12;}
    HINSTANCE inst=GetModuleHandleW(nullptr);WNDCLASSW wc{};wc.hInstance=inst;wc.lpfnWndProc=SettingsProc;wc.lpszClassName=L"LaseV2Settings";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);RegisterClassW(&wc);
    RECT parent;GetWindowRect(mainWin,&parent);d.w=CreateWindowExW(WS_EX_DLGMODALFRAME,L"LaseV2Settings",L"Область и параметры сравнения",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,parent.left+80,parent.top+40,660,800,mainWin,nullptr,inst,&d);
    auto control=[&](const wchar_t* cls,const std::wstring& text,int id,int x,int y,int width,int height,DWORD style=0){HWND c=CreateWindowW(cls,text.c_str(),WS_CHILD|WS_VISIBLE|style,x,y,width,height,d.w,(HMENU)(INT_PTR)id,inst,nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)font,TRUE);return c;};
    auto edit=[&](const wchar_t* name,int id,double value,int y){control(L"STATIC",name,0,22,y+4,310,24);control(L"EDIT",num(value,6),id,350,y,260,28,WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL);};
    edit(L"Шаг сетки XY (в единицах файла)",201,d.options.step,20);
    control(L"STATIC",L"Физическая единица координат",0,22,65,310,24);auto unit=control(L"COMBOBOX",L"",202,350,60,260,180,CBS_DROPDOWNLIST|WS_TABSTOP);
    for(auto s:{L"Не подтверждена — объём в ед.³",L"Миллиметры (проверьте масштаб)",L"Сантиметры",L"Метры"})SendMessageW(unit,CB_ADDSTRING,0,(LPARAM)s);SendMessageW(unit,CB_SETCURSEL,d.options.metresPerUnit==.001?1:d.options.metresPerUnit==.01?2:d.options.metresPerUnit==1?3:0,0);
    control(L"STATIC",L"Высота поверхности в ячейке",0,22,105,310,24);auto method=control(L"COMBOBOX",L"",203,350,100,260,150,CBS_DROPDOWNLIST|WS_TABSTOP);for(auto s:{L"Максимальная высота",L"Средняя высота",L"Минимальная высота",L"Медиана — устойчиво к выбросам",L"Нижняя поверхность — проверено по n-gk"})SendMessageW(method,CB_ADDSTRING,0,(LPARAM)s);SendMessageW(method,CB_SETCURSEL,d.options.estimator,0);
    edit(L"Сдвиг базового скана по X",204,d.options.dx,140);edit(L"Сдвиг базового скана по Y",205,d.options.dy,177);edit(L"Сдвиг базового скана по Z",206,d.options.dz,214);
    control(L"BUTTON",L"Подтверждаю общие оси и совмещение сканов с учётом поворота и сдвига",207,22,255,600,30,BS_AUTOCHECKBOX|WS_TABSTOP);CheckDlgButton(d.w,207,d.options.aligned?BST_CHECKED:BST_UNCHECKED);
    control(L"BUTTON",L"Ограничить область расчёта и просмотра",208,22,299,580,30,BS_AUTOCHECKBOX|WS_TABSTOP);CheckDlgButton(d.w,208,d.roi.enabled?BST_CHECKED:BST_UNCHECKED);
    const wchar_t* names[]={L"X от",L"X до",L"Y от",L"Y до",L"Z от",L"Z до"};double values[]={d.roi.x0,d.roi.x1,d.roi.y0,d.roi.y1,d.roi.z0,d.roi.z1};
    for(int i=0;i<6;i++){int x=22+(i%2)*310,y=342+(i/2)*42;control(L"STATIC",names[i],0,x,y+4,60,26);control(L"EDIT",num(values[i],3),210+i,x+65,y,220,28,WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL);}
    control(L"STATIC",L"Порог ΔZ для груза (ед. файла)",0,22,473,310,26);control(L"EDIT",num(d.threshold,3),216,350,469,260,28,WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL);
    control(L"BUTTON",L"Только крупнейшая связная область груза",217,22,503,580,26,BS_AUTOCHECKBOX|WS_TABSTOP);CheckDlgButton(d.w,217,d.largest?BST_CHECKED:BST_UNCHECKED);
    edit(L"Поворот базы вокруг Z (градусы)",218,d.options.angle,537);
    control(L"BUTTON",L"Отсекать борта, выбросы и узкие полосы (проверяйте область)",219,22,576,600,28,BS_AUTOCHECKBOX|WS_TABSTOP);CheckDlgButton(d.w,219,d.clean?BST_CHECKED:BST_UNCHECKED);
    control(L"BUTTON",L"Восстанавливать короткие и замкнутые пропуски",220,22,611,600,28,BS_AUTOCHECKBOX|WS_TABSTOP);CheckDlgButton(d.w,220,d.options.reconstructGaps?BST_CHECKED:BST_UNCHECKED);
    control(L"BUTTON",L"Автоматически выбирать шаг по плотности и пропускам",221,22,642,600,28,BS_AUTOCHECKBOX|WS_TABSTOP);CheckDlgButton(d.w,221,d.options.adaptiveGrid?BST_CHECKED:BST_UNCHECKED);
    control(L"BUTTON",L"Уточнять оценку по контрольным измерениям",222,22,673,600,28,BS_AUTOCHECKBOX|WS_TABSTOP);CheckDlgButton(d.w,222,d.options.calibratedEstimate?BST_CHECKED:BST_UNCHECKED);
    control(L"BUTTON",L"Применить",IDOK,342,711,130,38,BS_DEFPUSHBUTTON|WS_TABSTOP);control(L"BUTTON",L"Отмена",IDCANCEL,482,711,130,38,BS_PUSHBUTTON|WS_TABSTOP);
    EnableWindow(mainWin,FALSE);ShowWindow(d.w,SW_SHOW);MSG msg;while(!d.done&&GetMessageW(&msg,nullptr,0,0)>0)if(!IsDialogMessageW(d.w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}EnableWindow(mainWin,TRUE);SetActiveWindow(mainWin);
    if(d.accepted){applyRegion(d.roi);compareOptions=d.options;cargoThreshold=d.threshold;cargoLargest=d.largest;cargoClean=d.clean;invalidateComparison();compileLists();redraw();}return d.accepted;
}
#include "layers_ui.hpp"
std::vector<uint8_t> capturePng(){
    render();glFinish();glReadBuffer(GL_FRONT);glPixelStorei(GL_PACK_ALIGNMENT,1);std::vector<uint8_t> bottom(size_t(viewW)*viewH*3),top(bottom.size());glReadPixels(0,0,viewW,viewH,GL_RGB,GL_UNSIGNED_BYTE,bottom.data());if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("Cannot capture OpenGL frame");
    for(int y=0;y<viewH;y++)std::copy_n(bottom.data()+size_t(viewH-1-y)*viewW*3,size_t(viewW)*3,top.data()+size_t(y)*viewW*3);return encodePng(viewW,viewH,top);
}
std::filesystem::path saveDialog(const wchar_t* filter,const wchar_t* ext,const std::wstring& name){wchar_t path[32768]{};wcsncpy(path,name.c_str(),32767);OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=mainWin;o.lpstrFilter=filter;o.lpstrFile=path;o.nMaxFile=32768;o.lpstrDefExt=ext;o.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;return GetSaveFileNameW(&o)?std::filesystem::path(path):std::filesystem::path{};}
std::string html(std::string s){std::string out;for(char c:s)switch(c){case '&':out+="&amp;";break;case '<':out+="&lt;";break;case '>':out+="&gt;";break;case '"':out+="&quot;";break;default:out+=c;}return out;}
std::vector<std::pair<std::string,std::string>> reportRows(){
    std::vector<std::pair<std::string,std::string>> rows;auto row=[&](const std::string& a,const std::string& b){rows.emplace_back(a,b);};
    row("Остальные точки Full",cargoView&&showContext?"Показаны серым; не добавляются к рассчитанному объёму":"Скрыты или обычный просмотр скана");row("Активный файл",utf8(loadedPath.wstring()));row("Тип / набор",std::string(model.kind==3?"Reference":model.kind==2?"Full":"Empty")+" / "+std::to_string(model.selectedType));row("Точек в области",std::to_string(model.points.size()));
    if(region.enabled){row("Область X",utf8(num(region.x0)+L" … "+num(region.x1)));row("Область Y",utf8(num(region.y0)+L" … "+num(region.y1)));row("Область Z",utf8(num(region.z0)+L" … "+num(region.z1)));}else row("Область","Все точки");
    if(comparison&&baseDocument>=0){auto& c=*comparison;row("Базовый файл",utf8(documents[baseDocument].path.wstring()));if(!c.warning.empty())row("Замечания",c.warning);if(c.sensitivityChecked){double scale=std::pow(c.options.metresPerUnit>0?c.options.metresPerUnit:1,3);row("Проверка пяти шагов и смещения сетки",utf8(num(c.sensitivityMin*scale,2)+L" … "+num(c.sensitivityMax*scale,2))+(c.options.metresPerUnit>0?" м³":" ед.³"));row("Значение диапазона","Чувствительность к шагу сетки, не доверительный интервал и не подтверждённая погрешность.");}row("Качество результата",c.options.provisional?"Предварительный: частичное совпадение. Проверьте борта кузова и область расчёта.":"Оценка по общей области");if(c.options.alignmentOverlap>=0)row("Совпадение при автовыравнивании",utf8(num(c.options.alignmentOverlap*100,1))+"%");row("Разность","Активный − базовый");if(cargo.calibrated)row("Уточнённая оценка",utf8(num(cargo.calibratedVolume*std::pow(c.options.metresPerUnit,3),4))+" м³");row("Статус калибровки",cargo.calibrationNote);row("Смысл калиброванной оценки","Отдельная статистическая поправка по геометрическим признакам. Геометрия, точки и PLY соответствуют исходному интегралу, а не калиброванной оценке.");row("Объём отображаемого груза",c.options.metresPerUnit>0?utf8(num(cargo.volume*std::pow(c.options.metresPerUnit,3),2))+" м³ (при выбранном масштабе)":"Единицы не заданы");row("Восстановление пропусков",c.options.reconstructGaps?"Короткие пропуски и ограниченные замкнутые участки с измеренной границей":"Выключено");row("Вклад восстановленных ячеек",utf8(num(cargo.reconstructedVolume*std::pow(c.options.metresPerUnit>0?c.options.metresPerUnit:1,3),3))+(c.options.metresPerUnit>0?" м³":" ед.³"));row("Восстановленная общая площадь",utf8(num(c.reconstructedArea))+" ед.²");row("Отсечение бортов",cargo.cleaned?"Включено":"Выключено");row("Отсечено ячеек",std::to_string(cargo.removedCells));row("Порог ΔZ груза",utf8(num(cargo.threshold)));row("Связные области",cargo.largest?"Крупнейшая по объёму":"Все положительные");row("Шаг сетки",utf8(num(c.options.step)));row("Автоматический шаг",c.adaptiveGridUsed?"Да: 0.75×, при вкладе восстановления >1% — 1.25×":"Нет");row("Заданный шаг",utf8(num(c.requestedStep)));row("Оценка высоты",c.options.estimator==0?"Максимум":c.options.estimator==1?"Среднее":c.options.estimator==2?"Минимум":c.options.estimator==4?"Нижняя поверхность: квантиль 10%, при <10 точках медиана; смешанные ячейки уточняются по соседям":"Медиана");row("Уточнение высоты по верхней части кузова",utf8(num(c.options.rimHeightAdjustment,3))+" ед. файла");row("Уточнение высоты по бортам кузова",utf8(num(c.options.bedHeightAdjustment,3))+" ед. файла");row("Опорные варианты кузова",std::to_string(c.options.bedHeightVariants)+" из 9");row("Разброс опорной высоты",utf8(num(c.options.bedHeightSpread,3))+" ед. файла (чувствительность, не доверительный интервал)");if(c.datumSensitivityChecked){double scale=std::pow(c.options.metresPerUnit>0?c.options.metresPerUnit:1,3);row("Чувствительность объёма к высоте базы",utf8(num(c.datumSensitivityMin*scale,3)+L" … "+num(c.datumSensitivityMax*scale,3))+(c.options.metresPerUnit>0?" м³":" ед.³"));}row("Поворот базы вокруг Z, градусы",utf8(num(c.options.angle,4)));row("Сдвиг базового X / Y / Z",utf8(num(c.options.dx)+L" / "+num(c.options.dy)+L" / "+num(c.options.dz)));row("Метры на единицу",c.options.metresPerUnit>0?utf8(num(c.options.metresPerUnit,6)):"Не подтверждено");double k=c.options.metresPerUnit>0?std::pow(c.options.metresPerUnit,3):1;std::wstring unit=c.options.metresPerUnit>0?L" м³":L" ед.³";row("Добавлено",utf8(num(c.positive*k)+unit));row("Убрано",utf8(num(c.negative*k)+unit));row("Разность объёмов",utf8(num((c.positive-c.negative)*k)+unit));row("Общих ячеек",std::to_string(c.cells.size()));row("Общая площадь",utf8(num(c.sharedArea))+" ед.²");row("Покрытие выбранного XY-прямоугольника",utf8(num(100*c.coverage(),2))+"%");row("Максимальная |ΔZ|",utf8(num(c.maxAbs)));row("Совмещение",c.options.aligned?"Совмещение принято для расчёта с указанным поворотом и сдвигом":"Ненадёжное совмещение: использован найденный вариант, результат предварительный");}
    else row("Сравнение","Не рассчитано");
    if(comparison){row("Виртуальный тент: груз выше бортов",tarp.available?utf8(num(tarp.volume*std::pow(comparison->options.metresPerUnit>0?comparison->options.metresPerUnit:1,3)))+(comparison->options.metresPerUnit>0?" м³":" ед.³"):tarp.note);row("Метод виртуального тента",tarp.note);row("Вода над грузом до бортов",fullWater.available?utf8(num(fullWater.volume*std::pow(comparison->options.metresPerUnit>0?comparison->options.metresPerUnit:1,3)))+(comparison->options.metresPerUnit>0?" м³":" ед.³"):fullWater.note);row("Метод воды над грузом",fullWater.note);row("Заполнение по бортам",water.available?utf8(waterSummary()):water.note);row("Метод заполнения",water.note);if(water.available){row("Уровень воды Z",utf8(num(water.level)));row("Восстановленные ячейки Empty",std::to_string(water.interpolated));row("Достоверность вместимости","Оценка по измеренным бортам и дну Empty. Пропуски, перекос кузова и неполное сканирование могут занижать вместимость; горизонталь принята по оси Z.");}}
    if(!controlDatabase.path.empty()){row("Контрольная база",utf8(controlDatabase.path.wstring()));for(auto& line:controlLines())row("Сверка TotalVolume",utf8(line));auto match=currentControl();if(match.record)row("Запись базы",std::to_string(match.record->id)+" / "+match.record->truck);}
    return rows;
}
#include "pdf_report.hpp"
void writeReport(const std::filesystem::path& path){
    std::ofstream out(path,std::ios::binary);if(!out)throw std::runtime_error("Cannot create report");out.imbue(std::locale::classic());out<<std::setprecision(12);
    out<<"<!doctype html><html lang='ru'><meta charset='utf-8'><title>LaseScanViewer — отчёт</title><style>body{font:16px system-ui;max-width:1100px;margin:40px auto;padding:0 24px;color:#203040}h1{font-size:30px}table{border-collapse:collapse;width:100%}td{padding:10px;border-bottom:1px solid #ddd}td:first-child{width:32%;color:#526579}img{max-width:100%;background:#000}small{color:#647080}</style><h1>LaseScanViewer · Отчёт</h1><table>";
    for(auto& item:reportRows())out<<"<tr><td>"<<html(item.first)<<"</td><td>"<<html(item.second)<<"</td></tr>";
    out<<"</table><p>Оценка 2.5D: сумма разностей высот, умноженных на площадь общей ячейки. Короткие пропуски восстанавливаются по соседям, небольшие замкнутые — итерационно по измеренной границе; их вклад указан отдельно. Большие пробелы и области без измерений обоих сканов не заполняются. Результат относится к измеренной и ограниченно восстановленной площади, зависит от шага сетки, выбора поверхности и совмещения. Это не калиброванное измерение полного объёма груза.</p><p>"<<(cargoView?"Показан только положительный объём выше базы, после порога и фильтра связных областей. Результат в м³ зависит от выбранного масштаба; единицы BIN требуют проверки.":overlayView?"Наложение: активный скан оранжевый, базовый голубой.":differenceView?"Карта разностей: красный — выше базы, синий — ниже, зелёный — около нуля, серый — нет пары.":"Цвет соответствует высоте Z.")<<"</p><img alt='Текущий вид' src='data:image/png;base64,"<<base64(capturePng())<<"'></html>";out.flush();if(!out)throw std::runtime_error("Report write failed");
}
void invalidateComparisonKeepingView(){auto frame=currentViewBounds();invalidateComparison();lockedFrame=frame;}
void calculateComparison(){
    if(busy)return;
    if(activeDocument<0||baseDocument<0||activeDocument==baseDocument)throw std::runtime_error("Select two different files: active and baseline");
    invalidateComparisonKeepingView();compileLists();volumeProgress=0;shownVolumeProgress=-1;
    volumeTask=std::async(std::launch::async,[a=documents[activeDocument].data,b=documents[baseDocument].data,r=region,o=compareOptions,t=cargoThreshold,l=cargoLargest,c=cargoClean]{
        ViewerVolumeResult result;result.volume=calculateVolume(a,b,r,o,t,l,c,[](int value){volumeProgress.store(value);});
        result.water=waterFill(b,result.volume.comparison,result.volume.cargo);result.fullWater=waterAboveLoad(a,result.volume.comparison,result.water);result.tarp=virtualTarp(result.volume.cargo,result.water,&result.volume.comparison);
        if(result.volume.comparison.region.enabled){auto whole=result.volume.comparison;whole.region={};result.dimensions=measureBed(waterFill(b,whole,result.volume.cargo));}
        else result.dimensions=measureBed(result.water);
        return result;
    });
    busy=true;EnableWindow(GetDlgItem(mainWin,OPEN),FALSE);SetTimer(mainWin,3,60,nullptr);status(L"Расчёт объёма: подготовка поверхностей…");redraw();
}
void finishComparison(){
    if(!volumeTask.valid())return;
    if(volumeTask.wait_for(std::chrono::seconds(0))!=std::future_status::ready){
        int value=volumeProgress.load();if(value!=shownVolumeProgress){shownVolumeProgress=value;
            status(L"Расчёт объёма: "+std::to_wstring(value)+L"% · "+(value<15?L"Поверхности и ограниченное восстановление пропусков":value<30?L"Адаптивная сетка и выделение груза":value<70?L"Проверка на пяти разрешениях":value<95?L"Проверка смещения сетки":L"Уточнение результата"));redraw();}
        return;
    }
    KillTimer(mainWin,3);busy=false;EnableWindow(GetDlgItem(mainWin,OPEN),TRUE);
    try{auto result=volumeTask.get();comparison=std::move(result.volume.comparison);cargo=std::move(result.volume.cargo);water=std::move(result.water);bedDimensions=result.dimensions;fullWater=std::move(result.fullWater);tarp=std::move(result.tarp);
        cargoView=true;differenceView=overlayView=mesh=false;compileLists();status(comparisonSummary());
    }catch(const std::exception& e){invalidateComparisonKeepingView();compileLists();status(L"Расчёт объёма не выполнен");
        if(!renderTestDir.empty())throw;
        MessageBoxW(mainWin,widen(e.what()).c_str(),L"Расчёт объёма",MB_ICONEXCLAMATION);
    }
    redraw();if(!loadQueue.empty()){auto p=loadQueue.front();loadQueue.pop_front();startLoad(p);}
}
void startAlignment(bool calculate){
    if(activeDocument<0||baseDocument<0||activeDocument==baseDocument)throw std::runtime_error("Выберите разные активный и базовый файлы");
    invalidateComparisonKeepingView();selectRegion=selecting=false;compareOptions.aligned=false;compareOptions.alignmentOverlap=-1;compareOptions.rimHeightAdjustment=0;compareOptions.bedHeightAdjustment=compareOptions.bedHeightSpread=0;compareOptions.bedHeightVariants=0;compileLists();calculateAfterAlignment=calculate;
    alignmentProgress=0;shownAlignmentProgress=-1;
    alignmentTask=std::async(std::launch::async,[a=documents[activeDocument].data,b=documents[baseDocument].data,o=compareOptions]{return registration::align(a,b,o,[](int value){alignmentProgress.store(value);});});
    busy=true;EnableWindow(GetDlgItem(mainWin,OPEN),FALSE);SetTimer(mainWin,2,80,nullptr);status(L"Точное совмещение: поиск поворота 0–360° и сдвига…");redraw();
}
void finishAlignment(){
    if(!alignmentTask.valid())return;
    if(alignmentTask.wait_for(std::chrono::seconds(0))!=std::future_status::ready){
        int value=alignmentProgress.load();
        if(value!=shownAlignmentProgress){shownAlignmentProgress=value;status(L"Точное совмещение: "+std::to_wstring(value)+L"% · "+(value<85?L"Поиск по всему скану и верхним частям кузова":value<98?L"Уточнение лучших вариантов на трёх разрешениях":L"Проверка совмещения по стенкам кузова"));redraw();}
        return;
    }
    KillTimer(mainWin,2);busy=false;EnableWindow(GetDlgItem(mainWin,OPEN),TRUE);
    try{auto result=alignmentTask.get();compareOptions=result.options;overlayView=true;cargoView=differenceView=false;compileLists();
        status(L"Поворот "+num(compareOptions.angle,2)+L"° · Совпадение "+num(result.overlap*100,1)+L"% · RMS "+num(result.rms,1)+L" ед. · "+(result.structureRefined?L"Уточнено по стенкам · ":L"")+(result.reliable?L"Проверьте наложение неподвижных частей":compareOptions.provisional?L"Частичное совпадение — расчёт предварительный":L"Низкое/неоднозначное совпадение — проверьте пару и настройки"));
        if(calculateAfterAlignment)calculateComparison();
    }catch(const std::exception& e){status(L"Автовыравнивание не выполнено");MessageBoxW(mainWin,widen(e.what()).c_str(),L"Выравнивание",MB_ICONEXCLAMATION);}redraw();if(!busy&&!loadQueue.empty()){auto p=loadQueue.front();loadQueue.pop_front();startLoad(p);}
}
void v2Command(int id,int notification){
    if(preferenceCommand(id))return;
try{
    if(busy)return;
    switch(id){
    case ACTIVE_FILE:if(notification==CBN_SELCHANGE)selectDocument(int(SendDlgItemMessageW(mainWin,id,CB_GETCURSEL,0,0)));break;
    case BASE_FILE:if(notification==CBN_SELCHANGE){compareOptions.aligned=false;compareOptions.provisional=false;compareOptions.alignmentOverlap=-1;compareOptions.rimHeightAdjustment=0;compareOptions.bedHeightAdjustment=compareOptions.bedHeightSpread=0;compareOptions.bedHeightVariants=0;compareOptions.angle=compareOptions.dx=compareOptions.dy=compareOptions.dz=0;baseDocument=int(SendDlgItemMessageW(mainWin,id,CB_GETCURSEL,0,0))-1;invalidateComparison();compileLists();redraw();}break;
    case CLOSE_FILE:if(activeDocument>=0){int removed=activeDocument;documents.erase(documents.begin()+removed);if(baseDocument==removed)baseDocument=-1;else if(baseDocument>removed)--baseDocument;invalidateComparison();if(documents.empty()){model={};region={};loadedPath.clear();activeDocument=-1;compileLists();updateFileLists();redraw();}else selectDocument(std::min(removed,int(documents.size())-1));}break;
    case WATER:scanLayers.enabled=false;if(comparison){if(!water.available){status(waterSummary());MessageBoxW(mainWin,waterSummary().c_str(),L"Заполнение по бортам",MB_ICONINFORMATION);break;}lockedFrame=currentViewBounds();showWater=!showWater;cargoView=true;differenceView=overlayView=false;compileLists();status(showWater?waterSummary():comparisonSummary());redraw();}break;
    case CONTEXT_POINTS:scanLayers.enabled=false;if(comparison){lockedFrame=currentViewBounds();showContext=!cargoView||!showContext;cargoView=true;differenceView=overlayView=false;compileLists();status(comparisonSummary());redraw();}break;
    case CARGO:scanLayers.enabled=false;if(comparison){cargoView=!cargoView;differenceView=!cargoView;overlayView=false;compileLists();status(comparisonSummary());}break;
    case OVERLAY:scanLayers.enabled=false;if(baseDocument<0||baseDocument==activeDocument)throw std::runtime_error("Select two different files");if(model.selectedType!=documents[baseDocument].data.selectedType)throw std::runtime_error("Select the same coordinate group");cargoView=false;overlayView=!overlayView;differenceView=false;mesh=false;compileLists();status(L"Наложение: активный — оранжевый, базовый — голубой. Поворот и сдвиг базы задаются в настройках.");redraw();break;
    case AUTO_ALIGN:startAlignment(false);break;
    case COMPARE:if(compareOptions.aligned||compareOptions.alignmentOverlap>=0)calculateComparison();else startAlignment(true);break;
    case REGION_SELECT:lockedFrame.reset();cargoView=false;overlayView=false;compileLists();selectRegion=true;selecting=false;yaw=pitch=0;zoom=1;panX=panY=0;status(L"Выделите область прямоугольником ЛКМ в виде сверху. Z можно задать в параметрах.");redraw();break;
    case REGION_CLEAR:applyRegion({});selectRegion=false;reset();status(L"Область сброшена. Показаны все точки.");break;
    case LOAD_DATABASE:openControlDatabase();break;
    case MANUAL_CALC:manualCalculator();break;
    case UNLOAD_DATABASE:controlDatabase={};redraw();status(L"Контрольная база отключена");break;
    case OPTIONS:settingsMenu();break;
    case PNG_SAVE:{auto p=saveDialog(L"Изображение PNG\0*.png\0",L"png",loadedPath.stem().wstring()+L"_view.png");if(!p.empty()){writeBytes(p,capturePng());status(L"PNG сохранён: "+p.wstring());}}break;
    case REPORT_SAVE:{if(model.points.empty())break;bool pdf=reportFormat==ReportFormat::PDF;auto p=saveDialog(pdf?L"Отчёт PDF\0*.pdf\0":L"Отчёт HTML\0*.html\0",pdf?L"pdf":L"html",loadedPath.stem().wstring()+(pdf?L"_report.pdf":L"_report.html"));if(!p.empty()){if(pdf)writePdfReport(p);else writeReport(p);status(L"Отчёт сохранён: "+p.wstring());}}break;
    }
}catch(const std::exception& e){MessageBoxW(mainWin,widen(e.what()).c_str(),L"LaseScanViewer",MB_ICONEXCLAMATION);}}

void runCaptionRefreshTest(){try{
    std::filesystem::create_directories(renderTestDir);ShowWindow(mainWin,SW_SHOW);SetActiveWindow(mainWin);UpdateWindow(mainWin);
    RECT initial,initialClient;GetWindowRect(mainWin,&initial);GetClientRect(mainWin,&initialClient);auto active=GetActiveWindow();auto focus=GetFocus();auto resizeEvents=captionSizeEvents;
    float oldYaw=yaw,oldPitch=pitch,oldZoom=zoom,oldX=panX,oldY=panY;
    std::ofstream audit(renderTestDir/L"caption-colors.txt");
    for(int cycle=0;cycle<6;++cycle){bool light=(cycle%2)==1;v2Command(light?THEME_LIGHT:THEME_DARK);
        RECT current,client;GetWindowRect(mainWin,&current);GetClientRect(mainWin,&client);
        if(!EqualRect(&initial,&current)||!EqualRect(&initialClient,&client)||captionSizeEvents!=resizeEvents||GetActiveWindow()!=active||GetFocus()!=focus)throw std::runtime_error("Caption refresh resized window or changed focus");
        if(yaw!=oldYaw||pitch!=oldPitch||zoom!=oldZoom||panX!=oldX||panY!=oldY)throw std::runtime_error("Caption refresh changed camera");
        // Capture the composed native frame without a test repaint or resize.
        // GetWindowDC alone can expose an obsolete GDI surface under DWM.
        COLORREF color=saveCaptionTest(renderTestDir/(light?L"caption-immediate-light.png":L"caption-immediate-dark.png"),false);
        if(color==CLR_INVALID)throw std::runtime_error("Cannot read native caption pixel");
        int brightness=(int(GetRValue(color))+GetGValue(color)+GetBValue(color))/3;
        audit<<(light?"light ":"dark ")<<brightness<<" RGB "<<int(GetRValue(color))<<","<<int(GetGValue(color))<<","<<int(GetBValue(color))<<"\n";
        if((light&&brightness<170)||(!light&&brightness>100))throw std::runtime_error("Caption did not change immediately");
    }
    applyTheme(false);std::ofstream(renderTestDir/L"caption-ok.txt")<<"PASS 6 immediate native-caption switches; no resize messages, geometry, focus or camera changes";
}catch(const std::exception& e){std::ofstream(renderTestDir/L"caption-error.txt")<<e.what();}}

void runV2Test(){try{
    wchar_t title[1024];GetWindowTextW(mainWin,title,1024);if(GetMenu(mainWin)||std::wstring(title).find(L"[Version: 3.30.2]")==std::wstring::npos)throw std::runtime_error("Menu or version title failed");
    if(!std::filesystem::exists(preferencesFile())&&lightTheme)throw std::runtime_error("Default theme must be dark");
    std::filesystem::create_directories(renderTestDir);
    if(documents.size()!=2)throw std::runtime_error("Multi-file queue did not load two documents");
    if(activeDocument<0||baseDocument<0||documents[activeDocument].data.kind!=2||documents[baseDocument].data.kind==2)throw std::runtime_error("Automatic Full/Empty roles failed");
    selectDocument(0);baseDocument=1;updateFileLists();size_t originalCount=documents[0].data.points.size();
    overlayView=true;compileLists();saveInterface(renderTestDir/L"overlay.bmp");overlayView=false;
    yaw=pitch=0;zoom=1;panX=panY=0;render();selectRegion=true;selectionStart={viewW/5,viewH/5};selectionEnd={viewW*4/5,viewH*4/5};finishRegion();if(!region.enabled)throw std::runtime_error("Mouse region selection failed");applyRegion({});reset();
    yaw=37;pitch=28;zoom=1.6f;panX=.17f;panY=-.11f;render();auto initialFrame=currentViewBounds();float initialScale=orthoHeight;
    auto verifyCamera=[&]{render();auto frame=currentViewBounds();if(yaw!=37||pitch!=28||zoom!=1.6f||panX!=.17f||panY!=-.11f||orthoHeight!=initialScale||frame.lo.x!=initialFrame.lo.x||frame.lo.y!=initialFrame.lo.y||frame.lo.z!=initialFrame.lo.z||frame.hi.x!=initialFrame.hi.x||frame.hi.y!=initialFrame.hi.y||frame.hi.z!=initialFrame.hi.z)throw std::runtime_error("Alignment/calculation changed camera or frame");};
    startAlignment(true);render();verifyCamera();while(busy){finishAlignment();finishComparison();Sleep(10);}verifyCamera();
    if(!comparison)throw std::runtime_error("Automatic volume calculation did not run");
    if(!compareOptions.aligned){
        if(!comparison->options.provisional||comparison->options.aligned||comparison->warning.empty())throw std::runtime_error("Unreliable alignment lost its warning");
        v2Command(COMPARE);if(alignmentTask.valid()||!volumeTask.valid())throw std::runtime_error("Repeat calculation reran alignment");
        while(busy){finishComparison();Sleep(10);}
        if(!comparison||!comparison->options.provisional)throw std::runtime_error("Repeated preliminary volume failed");
    }
    cargoView=false;overlayView=true;compileLists();saveInterface(renderTestDir/L"aligned.bmp");overlayView=false;
    render();float fixedScale=orthoHeight,oldYaw=yaw,oldPitch=pitch;
    SendMessageW(viewWin,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(300,250));
    if(GetCursor()!=LoadCursorW(nullptr,IDC_SIZEALL))throw std::runtime_error("Orbit cursor missing");
    SendMessageW(viewWin,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(480,340));
    SendMessageW(viewWin,WM_SETCURSOR,(WPARAM)viewWin,MAKELPARAM(HTCLIENT,WM_MOUSEMOVE));
    if(GetCursor()!=LoadCursorW(nullptr,IDC_SIZEALL))throw std::runtime_error("Drag cursor reset during movement");
    SendMessageW(viewWin,WM_LBUTTONUP,0,MAKELPARAM(480,340));render();
    if(GetCursor()!=LoadCursorW(nullptr,IDC_ARROW))throw std::runtime_error("Drag cursor not released");
    SendMessageW(viewWin,WM_RBUTTONDOWN,MK_RBUTTON,MAKELPARAM(300,250));
    if(GetCursor()!=LoadCursorW(nullptr,IDC_SIZEALL))throw std::runtime_error("Pan cursor missing");
    SendMessageW(viewWin,WM_CANCELMODE,0,0);
    if(dragging||GetCapture()==viewWin||GetCursor()!=LoadCursorW(nullptr,IDC_ARROW))throw std::runtime_error("Cancelled drag retained capture/cursor");
    if(yaw==oldYaw||pitch>=oldPitch||std::abs(orthoHeight-fixedScale)>1e-5)throw std::runtime_error("Orbit changed scale or failed to rotate");
    POINT wheel{viewW*3/4,viewH/3};float sx=(2.f*wheel.x/viewW-1)*orthoHeight*viewW/viewH,sy=(1-2.f*wheel.y/viewH)*orthoHeight;
    float anchorX=sx-panX,anchorY=sy-panY;ClientToScreen(viewWin,&wheel);SendMessageW(viewWin,WM_MOUSEWHEEL,MAKEWPARAM(0,WHEEL_DELTA),MAKELPARAM(wheel.x,wheel.y));render();
    if(std::abs(sx/1.12f-panX-anchorX)>1e-4||std::abs(sy/1.12f-panY-anchorY)>1e-4)throw std::runtime_error("Cursor zoom anchor moved");
    reset();yaw=37;pitch=28;zoom=1.6f;panX=.17f;panY=-.11f;render();initialFrame=currentViewBounds();initialScale=orthoHeight;calculateComparison();verifyCamera();bool progressSaved=false;while(busy){if(!progressSaved&&volumeProgress.load()>0&&volumeProgress.load()<100){saveInterface(renderTestDir/L"volume-progress.bmp");progressSaved=true;}finishComparison();Sleep(10);}verifyCamera();if(cargo.cells==0)throw std::runtime_error("Full scan cargo filter removed everything");saveInterface(renderTestDir/L"full-cargo.bmp");
    if(water.available){auto before=currentViewBounds();float scale=orthoHeight,oldYaw=yaw,oldPitch=pitch,oldZoom=zoom,oldPanX=panX,oldPanY=panY;double volume=cargo.volume;
        v2Command(WATER);render();auto after=currentViewBounds();if(!showWater||scale!=orthoHeight||oldYaw!=yaw||oldPitch!=pitch||oldZoom!=zoom||oldPanX!=panX||oldPanY!=panY||before.lo.x!=after.lo.x||cargo.volume!=volume)throw std::runtime_error("Water view changed camera or cargo");
        saveInterface(renderTestDir/L"water.bmp");writeBytes(renderTestDir/L"water.png",capturePng());writeReport(renderTestDir/L"water-report.html");v2Command(WATER);if(showWater)throw std::runtime_error("Water toggle failed");}
    {auto before=currentViewBounds();float scale=orthoHeight,oldYaw=yaw,oldPitch=pitch,oldZoom=zoom,oldX=panX,oldY=panY;double volume=cargo.volume;
        if(water.available&&(!fullWater.available||fullWater.volume<0||fullWater.volume>water.volume+1))throw std::runtime_error("Invalid water above load");
        layerSettings(1);render();auto after=currentViewBounds();
        if(!scanLayers.enabled||!scanLayers.full||scanLayers.cargo||!scanLayers.other||scanLayers.fullWater!=fullWater.available)throw std::runtime_error("Layer dialog failed to apply");
        if(scale!=orthoHeight||oldYaw!=yaw||oldPitch!=pitch||oldZoom!=zoom||oldX!=panX||oldY!=panY||before.lo.x!=after.lo.x||cargo.volume!=volume||!comparison)throw std::runtime_error("Layers changed camera or calculation");
        saveInterface(renderTestDir/L"layers-full-water.bmp");writeBytes(renderTestDir/L"layers-full-water.png",capturePng());writeReport(renderTestDir/L"layers-report.html");
        if(water.available){if(!tarp.available||tarp.volume<0||tarp.volume>cargo.volume+1)throw std::runtime_error("Invalid virtual tarp");
            auto keep=scanLayers;layerSettings(3);auto t=scanLayers;if(!t.tarp||t.full!=keep.full||t.empty!=keep.empty||t.cargo!=keep.cargo||t.other!=keep.other||t.emptyWater!=keep.emptyWater||t.fullWater!=keep.fullWater)throw std::runtime_error("Tarp disabled other layers");render();writeBytes(renderTestDir/L"virtual-tarp.png",capturePng());
            bool oldMesh=mesh;mesh=true;render();writeBytes(renderTestDir/L"virtual-tarp-surface.png",capturePng());mesh=oldMesh;
            if(cargo.volume!=volume||oldYaw!=yaw||oldPitch!=pitch||scale!=orthoHeight)throw std::runtime_error("Tarp changed total or camera");
            t.tarp=false;applyScanLayers(t);render();applyScanLayers(keep);
        }
        auto calcBefore=displayedVolume();auto db=documents[activeDocument].path.parent_path()/L"TVM_Measurement_Data_Base.sq3";
        if(std::filesystem::exists(db)){loadControlDatabase(db);auto match=currentControl();if(!match.record||controlLines().size()<3)throw std::runtime_error("Database comparison failed");if(displayedVolume()!=calcBefore||cargo.volume!=volume||oldYaw!=yaw||oldPitch!=pitch||scale!=orthoHeight)throw std::runtime_error("Database changed calculated result or camera");}
        v2Command(THEME_LIGHT);v2Command(FORMAT_PDF);if(!lightTheme||reportFormat!=ReportFormat::PDF)throw std::runtime_error("Preferences failed");saveCaptionTest(renderTestDir/L"caption-light.png");saveInterface(renderTestDir/L"theme-light.bmp");writeBytes(renderTestDir/L"theme-light.png",capturePng());writePdfReport(renderTestDir/L"report.pdf");writeReport(renderTestDir/L"control-report.html");
        v2Command(THEME_DARK);v2Command(FORMAT_HTML);if(lightTheme||reportFormat!=ReportFormat::HTML||displayedVolume()!=calcBefore||cargo.volume!=volume||oldYaw!=yaw||oldPitch!=pitch||scale!=orthoHeight)throw std::runtime_error("Theme changed result or camera");saveCaptionTest(renderTestDir/L"caption-dark.png");saveInterface(renderTestDir/L"theme-dark.bmp");writeBytes(renderTestDir/L"theme-dark.png",capturePng());
        computeSettings(1);auto chosen=compute::selected.load();
        if(chosen!=(compute::runtime().ready?compute::Backend::GPU:compute::Backend::CPU))throw std::runtime_error("Compute dialog apply failed");
        if(scale!=orthoHeight||oldYaw!=yaw||oldPitch!=pitch||oldZoom!=zoom||oldX!=panX||oldY!=panY||cargo.volume!=volume||!comparison)throw std::runtime_error("Compute dialog changed view or volume");
        computeSettings(2);if(compute::selected.load()!=chosen)throw std::runtime_error("Compute dialog cancel failed");
        computeSettings(3);if(compute::selected.load()!=compute::Backend::CPU)throw std::runtime_error("Compute dialog CPU selection failed");
        auto saved=scanLayers;layerSettings(2);if(scanLayers.full!=saved.full||scanLayers.cargo!=saved.cargo||scanLayers.fullWater!=saved.fullWater)throw std::runtime_error("Layer dialog cancel lost state");
        ScanLayers level;level.enabled=true;level.cargo=false;level.other=true;level.fullWater=fullWater.available;applyScanLayers(level);render();saveInterface(renderTestDir/L"water-level.bmp");writeBytes(renderTestDir/L"water-level.png",capturePng());
        bool zoneBefore=showZone,gridBefore=grid;float zoneYaw=yaw,zonePitch=pitch,zoneZoom=zoom,zoneX=panX,zoneY=panY;
        SendMessageW(mainWin,WM_COMMAND,GRID,0);render();if(showZone==zoneBefore)throw std::runtime_error("Zone button failed to toggle");
        SendMessageW(mainWin,WM_COMMAND,GRID,0);render();if(showZone!=zoneBefore||grid!=gridBefore||yaw!=zoneYaw||pitch!=zonePitch||zoom!=zoneZoom||panX!=zoneX||panY!=zoneY||cargo.volume!=volume)throw std::runtime_error("Zone button changed view or volume");
        ScanLayers s;s.enabled=true;s.cargo=true;s.other=true;s.empty=true;s.emptyWater=water.available;s.fullWater=fullWater.available;applyScanLayers(s);render();saveInterface(renderTestDir/L"layers-all.bmp");writeBytes(renderTestDir/L"layers-all.png",capturePng());
        s.full=s.empty=s.cargo=s.other=s.emptyWater=s.fullWater=s.label=false;applyScanLayers(s);render();if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("Empty layers rendering failed");
        scanLayers.enabled=false;compileLists();render();
    }

    auto r=bounds(documents[0].data);double dx=(r.x1-r.x0)*.15,dy=(r.y1-r.y0)*.15;r.x0+=dx;r.x1-=dx;r.y0+=dy;r.y1-=dy;r.z0=-1e12;r.z1=1e12;
    applyRegion(r);if(documents[0].data.points.size()!=originalCount||model.points.size()>=originalCount)throw std::runtime_error("ROI isolation failed");
    compareOptions.aligned=true;compareOptions.metresPerUnit=.001;calculateComparison();while(busy){finishComparison();Sleep(10);}
    if(!comparison||comparison->cells.empty()||!std::isfinite(comparison->positive))throw std::runtime_error("Comparison failed");
    mesh=true;saveInterface(renderTestDir/L"cargo-surface.bmp");mesh=false;saveInterface(renderTestDir/L"cargo-points.bmp");if(cargo.cloud.points.empty())throw std::runtime_error("Empty cargo point cloud");exportPly(cargo.cloud,renderTestDir/L"cargo-points.ply",false);saveInterface(renderTestDir/L"comparison.bmp");writeBytes(renderTestDir/L"view.png",capturePng());writeReport(renderTestDir/L"report.html");exportPly(model,renderTestDir/L"selection.ply",false);exportPly(cargo.geometry,renderTestDir/L"cargo.ply",true);if(!cargoView||cargo.cells==0||cargo.volume<=0)throw std::runtime_error("Cargo extraction failed");
    if(water.available&&!bedDimensions.available)throw std::runtime_error("Bed dimensions missing");
    std::ofstream(renderTestDir/L"bed-dimensions.txt")<<bedDimensions.length<<" "<<bedDimensions.width<<" "<<bedDimensions.height;
    {HDC dc=GetDC(mainWin);auto old=SelectObject(dc,font);wchar_t caption[80];GetWindowTextW(GetDlgItem(mainWin,MANUAL_CALC),caption,80);SIZE size;GetTextExtentPoint32W(dc,caption,int(wcslen(caption)),&size);RECT box;GetClientRect(GetDlgItem(mainWin,MANUAL_CALC),&box);SelectObject(dc,old);ReleaseDC(mainWin,dc);if(size.cx>box.right-16)throw std::runtime_error("Manual button label clipped");}
    auto manualBefore=displayedVolume();auto manualFrame=currentViewBounds();float manualYaw=yaw,manualPitch=pitch,manualZoom=zoom;manualCalculator(1);
    if(!std::filesystem::exists(renderTestDir/L"manual-ok.txt")||std::filesystem::exists(renderTestDir/L"manual-error.txt")||displayedVolume()!=manualBefore||yaw!=manualYaw||pitch!=manualPitch||zoom!=manualZoom||currentViewBounds().lo.x!=manualFrame.lo.x)throw std::runtime_error("Manual calculator failed or changed the scan");
    auto savedVolume=cargo.volume;auto savedPoints=cargo.cloud.points.size();yaw=37;pitch=28;zoom=1.6f;panX=.17f;panY=-.11f;render();auto before=currentViewBounds();float scaleBefore=orthoHeight;
    v2Command(CONTEXT_POINTS);render();auto after=currentViewBounds();if(yaw!=37||pitch!=28||zoom!=1.6f||panX!=.17f||panY!=-.11f||orthoHeight!=scaleBefore||before.lo.x!=after.lo.x||before.lo.y!=after.lo.y||before.lo.z!=after.lo.z||before.hi.x!=after.hi.x||before.hi.y!=after.hi.y||before.hi.z!=after.hi.z)throw std::runtime_error("Showing context changed camera");saveInterface(renderTestDir/L"cargo-context.bmp");writeBytes(renderTestDir/L"cargo-context.png",capturePng());writeReport(renderTestDir/L"context-report.html");mesh=true;saveInterface(renderTestDir/L"cargo-context-surface.bmp");mesh=false;v2Command(CONTEXT_POINTS);render();if(orthoHeight!=scaleBefore||zoom!=1.6f||panX!=.17f||panY!=-.11f||yaw!=37||pitch!=28)throw std::runtime_error("Hiding context changed camera");reset();if(cargo.volume!=savedVolume||cargo.cloud.points.size()!=savedPoints)throw std::runtime_error("Context view changed cargo analysis");
    auto summary=comparisonSummary();selectDocument(1);if(comparison||region.enabled||compareOptions.aligned)throw std::runtime_error("Stale analysis after switching files");selectDocument(0);if(model.points.size()!=originalCount)throw std::runtime_error("Original scan was modified");
    std::ofstream out(renderTestDir/L"v2-ok.txt");out<<utf8(summary)<<"\nPASS: multiple files, overlay, ROI, signed volume, PNG, HTML, PLY, state invalidation and original preservation\n";
}catch(const std::exception& e){std::ofstream(renderTestDir/L"v2-error.txt")<<e.what();}}
