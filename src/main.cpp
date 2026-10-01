#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <gl/GL.h>
#include <future>
#include <sstream>
#include <iomanip>
#include "model.hpp"

HWND mainWin,viewWin,statusWin; HDC glDC; HGLRC glRC; HFONT font,titleFont;
Model model; std::filesystem::path loadedPath,pendingPath; std::future<Model> loading;
std::filesystem::path renderTestDir;
bool busy=false,mesh=false,grid=false; float yaw=160,pitch=45,zoom=1,panX=0,panY=0,pointSize=1,orthoHeight=1;
POINT mouse{}; int dragging=0; GLuint pointList=0,faceList=0,textList=0,bigTextList=0; int viewW=1,viewH=1;
constexpr wchar_t applicationTitle[]=L"LaseScanViewer [Version: 3.26.0]";
constexpr int OPEN=101,POINT_MODE=102,SURFACE=103,RESET=104,TOP=105,FRONT=106,SIDE=107,EXPORT=108,GRID=109,SMALL=110,LARGE=111,GROUP0=112,GROUP1=113;
#include "v2_state.hpp"
std::wstring widen(const std::string& s){int n=MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),nullptr,0);std::wstring w(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),w.data(),n);return w;}
void status(const std::wstring& s){SetWindowTextW(statusWin,s.c_str());}
void syncControls();
void redraw(){syncControls();InvalidateRect(viewWin,nullptr,FALSE);InvalidateRect(mainWin,nullptr,FALSE);}
ViewBounds currentViewBounds(){if(lockedFrame)return *lockedFrame;const Model& frame=cargoView&&showContext&&activeDocument>=0?documents[activeDocument].data:cargoView?cargo.geometry:model;return {frame.lo,frame.hi,!frame.points.empty()};}
void reset(){lockedFrame.reset();yaw=160;pitch=45;zoom=1;panX=panY=0;redraw();}
#include "zone_preview.hpp"
void compileLists(){
    const Model& scene=cargoView?cargo.geometry:model;
    wglMakeCurrent(glDC,glRC);if(pointList)glDeleteLists(pointList,1);if(faceList)glDeleteLists(faceList,1);
    auto vertex=[&scene](uint32_t i){auto p=scene.points[i];auto c=!cargoView&&differenceView&&comparison?differenceColor(*comparison,p):overlayView?std::array<float,3>{1.f,.65f,.15f}:color(scene,p);glColor3fv(c.data());glVertex3f(p.x,p.y,p.z);};
    pointList=glGenLists(1);glNewList(pointList,GL_COMPILE);glBegin(GL_POINTS);if(cargoView){for(auto p:cargo.cloud.points){auto c=color(scene,p);glColor3fv(c.data());glVertex3f(p.x,p.y,p.z);}}else for(uint32_t i=0;i<scene.points.size();i++)vertex(i);glEnd();glEndList();
    if(baseList)glDeleteLists(baseList,1);baseList=glGenLists(1);glNewList(baseList,GL_COMPILE);glBegin(GL_POINTS);glColor3f(.1f,.75f,1.f);
    if(baseDocument>=0&&baseDocument<int(documents.size()))for(auto p:documents[baseDocument].data.points){p=transformBase(p,compareOptions);if(region.contains(p))glVertex3f(p.x,p.y,p.z);}glEnd();glEndList();
    if(contextList)glDeleteLists(contextList,1);contextList=glGenLists(1);glNewList(contextList,GL_COMPILE);
    if(((cargoView&&showContext)||scanLayers.enabled)&&activeDocument>=0){
        std::set<std::tuple<float,float,float>> colored;for(auto p:cargo.cloud.points)colored.emplace(p.x,p.y,p.z);
        const auto& full=documents[activeDocument].data;glBegin(GL_POINTS);
        for(auto p:full.points)if(!colored.count({p.x,p.y,p.z})){float g=.32f+.38f*std::clamp((p.z-full.lo.z)/std::max(1.f,full.hi.z-full.lo.z),0.f,1.f);glColor3f(g,g,g);glVertex3f(p.x,p.y,p.z);}glEnd();
    }glEndList();
    if(waterList)glDeleteLists(waterList,1);waterList=glGenLists(1);glNewList(waterList,GL_COMPILE);
    auto preview=waterPreviewSurface(water,comparison?(comparison->requestedStep>0?comparison->requestedStep:comparison->options.step):compareOptions.step);
    glBegin(GL_TRIANGLES);for(auto face:water.geometry.faces){if(face[0]%8>=4&&face[1]%8>=4&&face[2]%8>=4)continue;for(auto i:face){auto p=water.geometry.points[i];glVertex3f(p.x,p.y,p.z);}}
    for(auto f:preview.faces)for(auto i:f){auto p=preview.points[i];glVertex3f(p.x,p.y,p.z);}glEnd();glEndList();
    if(fullWaterList)glDeleteLists(fullWaterList,1);fullWaterList=glGenLists(1);glNewList(fullWaterList,GL_COMPILE);glBegin(GL_TRIANGLES);
    for(auto f:preview.faces)for(auto i:f){auto p=preview.points[i];glVertex3f(p.x,p.y,p.z);}glEnd();glEndList();
    if(fullLayerList)glDeleteLists(fullLayerList,1);fullLayerList=glGenLists(1);glNewList(fullLayerList,GL_COMPILE);glBegin(GL_POINTS);
    for(auto p:model.points){auto c=color(model,p);glColor3fv(c.data());glVertex3f(p.x,p.y,p.z);}glEnd();glEndList();
    if(cargoLayerList)glDeleteLists(cargoLayerList,1);cargoLayerList=glGenLists(1);glNewList(cargoLayerList,GL_COMPILE);glBegin(GL_POINTS);
    for(auto p:cargo.cloud.points){auto c=color(cargo.geometry,p);glColor3fv(c.data());glVertex3f(p.x,p.y,p.z);}glEnd();glEndList();
    if(cargoLayerFaces)glDeleteLists(cargoLayerFaces,1);cargoLayerFaces=glGenLists(1);glNewList(cargoLayerFaces,GL_COMPILE);glBegin(GL_TRIANGLES);
    for(auto f:cargo.geometry.faces)for(auto i:f){auto p=cargo.geometry.points[i];auto c=color(cargo.geometry,p);glColor3fv(c.data());glVertex3f(p.x,p.y,p.z);}glEnd();glEndList();
    if(tarpPoints)glDeleteLists(tarpPoints,1);tarpPoints=glGenLists(1);glNewList(tarpPoints,GL_COMPILE);glBegin(GL_POINTS);
    for(auto p:tarp.cloud.points){auto c=color(cargo.geometry,p);glColor3fv(c.data());glVertex3f(p.x,p.y,p.z);}glEnd();glEndList();
    if(tarpFaces)glDeleteLists(tarpFaces,1);tarpFaces=glGenLists(1);glNewList(tarpFaces,GL_COMPILE);glBegin(GL_TRIANGLES);
    for(auto f:tarp.geometry.faces)for(auto i:f){auto p=tarp.geometry.points[i];auto c=color(cargo.geometry,p);glColor3fv(c.data());glVertex3f(p.x,p.y,p.z);}glEnd();glEndList();
    faceList=glGenLists(1);glNewList(faceList,GL_COMPILE);glBegin(GL_TRIANGLES);for(auto t:scene.faces)for(auto i:t)vertex(i);glEnd();glEndList();
}
void render(){
    const bool volumeLabel=cargoView||(scanLayers.enabled&&comparison.has_value());
    const Model& scene=cargoView?cargo.geometry:model;
    auto frame=currentViewBounds();
    if(!glRC)return; wglMakeCurrent(glDC,glRC);glViewport(0,0,viewW,viewH);
    glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glDisable(GL_POINT_SMOOTH);glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);glMatrixMode(GL_PROJECTION);glLoadIdentity();
    float aspect=float(viewW)/std::max(1,viewH),s=1.15f/zoom;
    if(frame.valid){
        auto a=frame.lo,b=frame.hi;float span=std::max({b.x-a.x,b.y-a.y,b.z-a.z,1.f});
        // Fixed bounding sphere: turning the model no longer changes its scale.
        float radius=std::sqrt((b.x-a.x)*(b.x-a.x)+(b.y-a.y)*(b.y-a.y)+(b.z-a.z)*(b.z-a.z))/span;
        s=std::max(radius,.01f)*1.08f/std::min(aspect,1.f)/zoom;
    }
    orthoHeight=s;glOrtho(-s*aspect,s*aspect,-s,s,-20,20);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();glTranslatef(panX,panY,0);glRotatef(-pitch,1,0,0);glRotatef(yaw,0,0,1);
    if(frame.valid){
        auto a=frame.lo,b=frame.hi;float span=std::max({b.x-a.x,b.y-a.y,b.z-a.z,1.f});
        glScalef(2/span,2/span,2/span);glTranslatef(-(a.x+b.x)/2,-(a.y+b.y)/2,-(a.z+b.z)/2);
        if(grid&&(!cargoView||scanLayers.enabled)){glColor3f(.13f,.19f,.25f);glBegin(GL_LINES);float step=std::pow(10.f,std::floor(std::log10(span/10)));
            for(float x=std::floor(a.x/step)*step;x<=b.x+step;x+=step){glVertex3f(x,a.y-step,a.z-20);glVertex3f(x,b.y+step,a.z-20);}
            for(float y=std::floor(a.y/step)*step;y<=b.y+step;y+=step){glVertex3f(a.x-step,y,a.z-20);glVertex3f(b.x+step,y,a.z-20);}glEnd();
            glLineWidth(2);glBegin(GL_LINES);glColor3f(1,.35f,.3f);glVertex3f(a.x,a.y,a.z-10);glVertex3f(a.x+span*.14f,a.y,a.z-10);glColor3f(.4f,1,.5f);glVertex3f(a.x,a.y,a.z-10);glVertex3f(a.x,a.y+span*.14f,a.z-10);glColor3f(.4f,.6f,1);glVertex3f(a.x,a.y,a.z-10);glVertex3f(a.x,a.y,a.z+span*.14f);glEnd();glLineWidth(1);
        }
        if(scanLayers.enabled){
            glPointSize(pointSize);
            if(scanLayers.cargo)glCallList(mesh?cargoLayerFaces:cargoLayerList);
            if(scanLayers.full)glCallList(fullLayerList);
            if(scanLayers.other&&!scanLayers.full)glCallList(contextList);
            if(scanLayers.empty)glCallList(baseList);
            glDisable(GL_BLEND);glDepthMask(GL_TRUE);
            if(scanLayers.emptyWater&&water.available){glColor3f(0.f,0.f,1.f);glCallList(waterList);}
            if(scanLayers.fullWater&&fullWater.available){glColor3f(0.f,0.f,1.f);glCallList(fullWaterList);}
            if(scanLayers.tarp&&tarp.available)glCallList(mesh?tarpFaces:tarpPoints);
            glDepthMask(GL_TRUE);glDisable(GL_BLEND);
        }else{
        glPointSize(pointSize);if(cargoView&&showContext)glCallList(contextList);glCallList(mesh?faceList:pointList);
        if(showWater&&water.available){glDisable(GL_BLEND);glDepthMask(GL_TRUE);glColor3f(0.f,0.f,1.f);glCallList(waterList);glDepthMask(GL_TRUE);glDisable(GL_BLEND);}
        if(mesh&&!cargoView){glPointSize(1);glCallList(pointList);}if(overlayView&&!cargoView){glPointSize(pointSize);glCallList(baseList);}
        }
        drawZonePreview();
    }
    if((!scene.points.empty()||cargoView) && textList&&(!scanLayers.enabled||scanLayers.label)){
        glDisable(GL_DEPTH_TEST);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,viewW,0,viewH,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
        std::wstring label=L"Измерение: "+std::to_wstring(model.scan)+L"    Точек: "+std::to_wstring(cargoView?cargo.cloud.points.size():scene.points.size());
        if(volumeLabel){double scale=comparison?comparison->options.metresPerUnit:0;std::wostringstream value;value<<std::fixed<<std::setprecision(2)<<(cargo.calibrated?cargo.calibratedVolume:cargo.volume)*std::pow(scale,3);label=cargo.calibrated?std::wstring(comparison&&comparison->options.provisional?L"Предварительная оценка: ":L"Оценка объёма: ")+value.str()+L" м³":scale>0?std::wstring(comparison&&comparison->options.provisional?L"Предварительно: ":L"Объём груза: ")+value.str()+(scale==.001?L" м³ (если мм)":L" м³ (оценка)"):L"Выберите единицы координат в настройках";}
        glColor3f(.39f,.78f,.36f);glRasterPos2i(14,viewH-(volumeLabel?42:20));glListBase(volumeLabel&&bigTextList?bigTextList:textList);glCallLists(GLsizei(label.size()),GL_UNSIGNED_SHORT,label.data());
    }
    if((scanLayers.enabled?scanLayers.emptyWater:showWater)&&water.available&&textList){glDisable(GL_DEPTH_TEST);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,viewW,0,viewH,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
        auto text=waterSummary();glColor3f(.25f,.60f,1.f);glRasterPos2i(14,viewH-70);glListBase(textList);glCallLists(GLsizei(text.size()),GL_UNSIGNED_SHORT,text.data());}
    if(scanLayers.enabled&&scanLayers.fullWater&&fullWater.available&&textList){glDisable(GL_DEPTH_TEST);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,viewW,0,viewH,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
        std::wostringstream label;label<<L"Вода над грузом до бортов: "<<std::fixed<<std::setprecision(2)<<fullWater.volume*std::pow(compareOptions.metresPerUnit>0?compareOptions.metresPerUnit:1,3)<<(compareOptions.metresPerUnit>0?L" м³":L" ед.³");
        auto text=label.str();glColor3f(.3f,.7f,1.f);glRasterPos2i(14,viewH-94);glListBase(textList);glCallLists(GLsizei(text.size()),GL_UNSIGNED_SHORT,text.data());}
    if(scanLayers.enabled&&scanLayers.tarp&&tarp.available&&textList&&scanLayers.label){glDisable(GL_DEPTH_TEST);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,viewW,0,viewH,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();std::wostringstream label;label<<L"Тент · над бортами: "<<std::fixed<<std::setprecision(2)<<tarp.volume*std::pow(compareOptions.metresPerUnit>0?compareOptions.metresPerUnit:1,3)<<(compareOptions.metresPerUnit>0?L" м³":L" ед.³");auto text=label.str();glColor3f(.39f,.78f,.36f);glRasterPos2i(14,viewH-118);glListBase(textList);glCallLists(GLsizei(text.size()),GL_UNSIGNED_SHORT,text.data());}
    if(selecting){glDisable(GL_DEPTH_TEST);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,viewW,viewH,0,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();glColor3f(1,.8f,.2f);glBegin(GL_LINE_LOOP);glVertex2i(selectionStart.x,selectionStart.y);glVertex2i(selectionEnd.x,selectionStart.y);glVertex2i(selectionEnd.x,selectionEnd.y);glVertex2i(selectionStart.x,selectionEnd.y);glEnd();}
    drawControlComparison();
    SwapBuffers(glDC);
}
void saveFrame(const std::filesystem::path& path){
    render();glFinish();glReadBuffer(GL_FRONT);glPixelStorei(GL_PACK_ALIGNMENT,4);
    int stride=(viewW*3+3)&~3;std::vector<unsigned char> pixels(size_t(stride)*viewH);
    glReadPixels(0,0,viewW,viewH,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("OpenGL rendering check failed");
    size_t colored=0;for(int y=0;y<viewH;y++)for(int x=0;x<viewW;x++){auto p=&pixels[size_t(y)*stride+x*3];if(p[0]>80||p[1]>80||p[2]>80)colored++;std::swap(p[0],p[2]);}
    if(colored<100)throw std::runtime_error("Rendered image is empty");
    BITMAPFILEHEADER fh{};fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(BITMAPINFOHEADER);fh.bfSize=fh.bfOffBits+DWORD(pixels.size());
    BITMAPINFOHEADER ih{};ih.biSize=sizeof(ih);ih.biWidth=viewW;ih.biHeight=viewH;ih.biPlanes=1;ih.biBitCount=24;ih.biSizeImage=DWORD(pixels.size());
    std::ofstream out(path,std::ios::binary);out.write((char*)&fh,sizeof(fh));out.write((char*)&ih,sizeof(ih));out.write((char*)pixels.data(),pixels.size());if(!out)throw std::runtime_error("Render image write failed");
}
void startLoad(std::filesystem::path p,int preferredType=-1){
    if(busy)return;busy=true;status(L"Чтение BIN и построение поверхности…");EnableWindow(GetDlgItem(mainWin,OPEN),FALSE);redraw();
    replacing=preferredType>=0;pendingPath=p;loading=std::async(std::launch::async,[p,preferredType]{auto m=readModel(p,preferredType);triangulate(m);return m;});SetTimer(mainWin,1,50,nullptr);
}
void openFile(){std::vector<wchar_t> p(65536);OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=mainWin;o.lpstrFilter=L"LASE BIN (*.bin)\0*.bin\0Все файлы\0*.*\0";o.lpstrFile=p.data();o.nMaxFile=DWORD(p.size());o.lpstrTitle=L"Открыть сканы (можно выбрать несколько)";o.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_ALLOWMULTISELECT|OFN_EXPLORER;
 if(GetOpenFileNameW(&o)){std::filesystem::path first=p.data();const wchar_t* item=p.data()+wcslen(p.data())+1;std::vector<std::filesystem::path> paths;if(!*item)paths.push_back(first);else while(*item){paths.push_back(first/item);item+=wcslen(item)+1;}enqueueFiles(paths);}}
void saveFile(){
    if(busy||model.points.empty())return;wchar_t p[32768]{};std::wstring name=loadedPath.stem().wstring()+(mesh?L"_surface.ply":L"_points.ply");wcsncpy(p,name.c_str(),32767);
    OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=mainWin;o.lpstrFilter=L"PLY (*.ply)\0*.ply\0";o.lpstrFile=p;o.nMaxFile=32768;o.lpstrDefExt=L"ply";o.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
    if(GetSaveFileNameW(&o)){try{exportPly(cargoView?(mesh?cargo.geometry:cargo.cloud):model,p,mesh);status(L"Сохранено: "+std::wstring(p));}catch(const std::exception& e){MessageBoxW(mainWin,widen(e.what()).c_str(),L"Ошибка экспорта",MB_ICONERROR);}}
}
void updateViewCursor(){SetCursor(LoadCursorW(nullptr,dragging?IDC_SIZEALL:(selectRegion?IDC_CROSS:IDC_ARROW)));}
LRESULT CALLBACK ViewProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){
    case WM_SETCURSOR:if(dragging||LOWORD(lp)==HTCLIENT){updateViewCursor();return TRUE;}break;
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:{PAINTSTRUCT ps;BeginPaint(w,&ps);render();EndPaint(w,&ps);return 0;}
    case WM_SIZE:viewW=std::max(1,int(LOWORD(lp)));viewH=std::max(1,int(HIWORD(lp)));return 0;
    case WM_LBUTTONDOWN:case WM_RBUTTONDOWN:case WM_MBUTTONDOWN:if(selectRegion&&msg==WM_LBUTTONDOWN){SetFocus(w);selecting=true;selectionStart=selectionEnd={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};SetCapture(w);return 0;}dragging=msg==WM_LBUTTONDOWN&&!(wp&MK_SHIFT)?1:2;mouse={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};SetCapture(w);SetFocus(w);updateViewCursor();return 0;
    case WM_LBUTTONUP:case WM_RBUTTONUP:case WM_MBUTTONUP:if(selecting){selecting=false;ReleaseCapture();finishRegion();return 0;}dragging=0;ReleaseCapture();updateViewCursor();return 0;
    case WM_CAPTURECHANGED:dragging=0;selecting=false;updateViewCursor();return 0;
    case WM_CANCELMODE:dragging=0;selecting=false;if(GetCapture()==w)ReleaseCapture();updateViewCursor();return 0;
    case WM_MOUSEMOVE:if(dragging)updateViewCursor();if(selecting){selectionEnd={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};InvalidateRect(w,nullptr,FALSE);return 0;}if(dragging){int x=GET_X_LPARAM(lp),y=GET_Y_LPARAM(lp);if(dragging==1){float sensitivity=180.f/std::max(300,std::min(viewW,viewH));yaw=std::remainder(yaw+(x-mouse.x)*sensitivity,360.f);pitch=std::clamp(pitch-(y-mouse.y)*sensitivity,-89.5f,89.5f);}else{panX+=(x-mouse.x)*2.f*orthoHeight/viewH;panY-=(y-mouse.y)*2.f*orthoHeight/viewH;}mouse={x,y};InvalidateRect(w,nullptr,FALSE);}return 0;
    case WM_LBUTTONDBLCLK:reset();return 0;
    case WM_MOUSEWHEEL:{POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};ScreenToClient(w,&p);float old=zoom;zoom=std::clamp(zoom*std::pow(1.12f,GET_WHEEL_DELTA_WPARAM(wp)/120.f),.1f,50.f);float factor=old/zoom;panX+=(2.f*p.x/viewW-1)*orthoHeight*viewW/viewH*(factor-1);panY+=(1-2.f*p.y/viewH)*orthoHeight*(factor-1);InvalidateRect(w,nullptr,FALSE);return 0;}
    case WM_KEYDOWN:SendMessageW(mainWin,msg,wp,lp);return 0;
    }
    return DefWindowProcW(w,msg,wp,lp);
}
#include "ui.hpp"
#include "v2.hpp"
void command(int id){
    switch(id){case GROUP0:if(!loadedPath.empty()&&(model.availableTypes&1))startLoad(loadedPath,0);break;case GROUP1:if(!loadedPath.empty()&&(model.availableTypes&2))startLoad(loadedPath,1);break;case OPEN:openFile();break;case POINT_MODE:mesh=false;break;case SURFACE:mesh=true;break;case RESET:reset();break;case TOP:yaw=0;pitch=0;panX=panY=0;zoom=1;break;case FRONT:yaw=0;pitch=90;panX=panY=0;zoom=1;break;case SIDE:yaw=90;pitch=90;panX=panY=0;zoom=1;break;case EXPORT:saveFile();break;case GRID:showZone=!showZone;break;case SMALL:pointSize=std::max(1.f,pointSize-1);break;case LARGE:pointSize=std::min(8.f,pointSize+1);break;}
    redraw();
}
LRESULT CALLBACK MainProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){
    case WM_GETMINMAXINFO:reinterpret_cast<MINMAXINFO*>(lp)->ptMinTrackSize={1120,760};return 0;
    case WM_SIZE:layout();redraw();return 0;
    case WM_MEASUREITEM:{auto item=reinterpret_cast<MEASUREITEMSTRUCT*>(lp);if(item->CtlType==ODT_COMBOBOX){item->itemHeight=32;return TRUE;}break;}
    case WM_DRAWITEM:{auto d=reinterpret_cast<DRAWITEMSTRUCT*>(lp);if(d->CtlType==ODT_COMBOBOX){drawPickerItem(d);return TRUE;}drawButton(d->hDC,d->rcItem,int(d->CtlID),(d->itemState&ODS_SELECTED)!=0,(d->itemState&ODS_FOCUS)!=0);return TRUE;}
    case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);paintPanel(dc);EndPaint(w,&ps);return 0;}
    case WM_CTLCOLORLISTBOX:SetTextColor((HDC)wp,ink);SetBkColor((HDC)wp,panel);SetDCBrushColor((HDC)wp,panel);return (LRESULT)GetStockObject(DC_BRUSH);
    case WM_CTLCOLORSTATIC:SetTextColor((HDC)wp,muted);SetBkColor((HDC)wp,bg);SetDCBrushColor((HDC)wp,bg);return (LRESULT)GetStockObject(DC_BRUSH);
    case WM_COMMAND:if(LOWORD(wp)>=ACTIVE_FILE){v2Command(LOWORD(wp),HIWORD(wp));return 0;}command(LOWORD(wp));return 0;
    case WM_KEYDOWN:if(wp==VK_ESCAPE){selecting=selectRegion=false;ReleaseCapture();redraw();}else if(wp=='O'&&(GetKeyState(VK_CONTROL)&0x8000))openFile();else if(wp=='S'&&(GetKeyState(VK_CONTROL)&0x8000))saveFile();else if(wp=='R'||wp==VK_HOME)reset();else if(wp=='1')command(POINT_MODE);else if(wp=='2')command(SURFACE);return 0;
    case WM_TIMER:if(wp==3){finishComparison();return 0;}if(wp==2){finishAlignment();return 0;}if(busy && loading.wait_for(std::chrono::seconds(0))==std::future_status::ready){KillTimer(w,1);busy=false;EnableWindow(GetDlgItem(w,OPEN),TRUE);try{auto next=loading.get();acceptLoaded(std::move(next));if(!renderTestDir.empty()&&loadQueue.empty())PostMessageW(w,v2TestMode?WM_APP+2:WM_APP+1,0,0);SetWindowTextW(w,(std::wstring(applicationTitle)+L" — "+loadedPath.filename().wstring()).c_str());status(L"Скан открыт   ·   "+grouped(model.points.size())+L" точек   ·   Геометрия в исходных координатах");}catch(const std::exception& e){status(L"Не удалось открыть BIN. Выберите файл поддерживаемого формата.");MessageBoxW(w,(L"Не удалось прочитать файл.\nПоддерживается проверенная структура LASE Full/Empty/Reference.\n\n"+widen(e.what())).c_str(),L"Ошибка чтения",MB_ICONERROR);}redraw();if(!loadQueue.empty()){auto p=loadQueue.front();loadQueue.pop_front();startLoad(p);}}return 0;
    case WM_APP+2:runV2Test();DestroyWindow(w);return 0;
    case WM_APP+1:try{std::filesystem::create_directories(renderTestDir);mesh=false;syncControls();saveInterface(renderTestDir/L"interface.bmp");saveFrame(renderTestDir/L"points.bmp");mesh=true;saveFrame(renderTestDir/L"surface.bmp");exportPly(model,renderTestDir/L"model.ply",true);Model saved=std::move(model);auto savedDocuments=std::move(documents);documents.clear();updateFileLists();model=Model{};mesh=false;status(L"Готово   ·   Откройте BIN-файл или перетащите его в окно");syncControls();saveInterface(renderTestDir/L"empty.bmp");model=std::move(saved);documents=std::move(savedDocuments);updateFileLists();syncControls();SetWindowPos(w,nullptr,0,0,1120,780,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);saveInterface(renderTestDir/L"compact.bmp");std::ofstream(renderTestDir/L"render-ok.txt")<<"OpenGL points and surface rendered successfully";}catch(const std::exception& e){std::ofstream(renderTestDir/L"render-error.txt")<<e.what();}DestroyWindow(w);return 0;
    case WM_DROPFILES:{HDROP d=(HDROP)wp;wchar_t p[32768];std::vector<std::filesystem::path> paths;for(UINT i=0;i<DragQueryFileW(d,0xffffffff,nullptr,0);i++)if(DragQueryFileW(d,i,p,32768))paths.emplace_back(p);DragFinish(d);enqueueFiles(paths);return 0;}
    case WM_DESTROY:if(glRC){wglMakeCurrent(nullptr,nullptr);wglDeleteContext(glRC);ReleaseDC(viewWin,glDC);}DeleteObject(font);DeleteObject(titleFont);DeleteObject(smallFont);DeleteObject(numberFont);PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(w,msg,wp,lp);
}
int WINAPI wWinMain(HINSTANCE inst,HINSTANCE,PWSTR,int show){
    SetProcessDPIAware();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES};InitCommonControlsEx(&common);
    WNDCLASSW wc{};wc.hInstance=inst;wc.lpfnWndProc=MainProc;wc.lpszClassName=L"Lase3DMain";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hIcon=(HICON)LoadImageW(inst,MAKEINTRESOURCEW(101),IMAGE_ICON,GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),LR_DEFAULTCOLOR);RegisterClassW(&wc);
    wc.lpfnWndProc=ViewProc;wc.lpszClassName=L"Lase3DView";wc.style=CS_OWNDC|CS_DBLCLKS;RegisterClassW(&wc);
    mainWin=CreateWindowExW(WS_EX_ACCEPTFILES,L"Lase3DMain",applicationTitle,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,1420,940,nullptr,nullptr,inst,nullptr);
    SendMessageW(mainWin,WM_SETICON,ICON_BIG,(LPARAM)wc.hIcon);
    SendMessageW(mainWin,WM_SETICON,ICON_SMALL,(LPARAM)LoadImageW(inst,MAKEINTRESOURCEW(101),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR));
    if(HMODULE dwm=LoadLibraryW(L"dwmapi.dll")){using SetAttribute=HRESULT(WINAPI*)(HWND,DWORD,LPCVOID,DWORD);auto set=(SetAttribute)GetProcAddress(dwm,"DwmSetWindowAttribute");BOOL dark=TRUE;if(set&&FAILED(set(mainWin,20,&dark,sizeof(dark))))set(mainWin,19,&dark,sizeof(dark));FreeLibrary(dwm);}
    font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");titleFont=CreateFontW(-27,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    smallFont=CreateFontW(-13,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");numberFont=CreateFontW(-22,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    auto button=[&](int id,const wchar_t* title,int x,int y,int width,int height=38){HWND b=CreateWindowW(L"BUTTON",title,WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,x,y,width,height,mainWin,(HMENU)(INT_PTR)id,inst,nullptr);SendMessageW(b,WM_SETFONT,(WPARAM)font,TRUE);SetWindowSubclass(b,ButtonSkinProc,1,0);};
    button(OPEN,L"+   Открыть Full / пару",20,99,216,44);button(CLOSE_FILE,L"×",244,99,36,44);
    button(POINT_MODE,L"Точки",20,182,126);button(SURFACE,L"Поверхность",154,182,126);
    button(TOP,L"Сверху",20,262,81);button(FRONT,L"Спереди",109,262,82);button(SIDE,L"Сбоку",199,262,81);
    button(RESET,L"Общий вид   ·   R",20,308,260);
    button(GROUP1,L"Набор 1",20,388,126);button(GROUP0,L"Набор 0",154,388,126);
    button(GRID,L"Зона интереса",20,468,126);button(CARGO,L"Только груз",154,468,126);
    button(CONTEXT_POINTS,L"Остальные точки",20,514,260,36);
    button(SMALL,L"−",200,558,36,32);button(LARGE,L"+",244,558,36,32);
    button(EXPORT,L"Экспортировать PLY",20,800,260,40);
    button(LOAD_DATABASE,L"Загрузить базу SQ3",20,606,260,34);
    createV2Controls(inst);
    viewWin=CreateWindowW(L"Lase3DView",L"3D",WS_CHILD|WS_CLIPSIBLINGS,320,88,1000,750,mainWin,nullptr,inst,nullptr);
    statusWin=CreateWindowW(L"STATIC",L"Готово   ·   Откройте BIN-файл или перетащите его в окно",WS_CHILD|WS_VISIBLE,12,780,1250,24,mainWin,nullptr,inst,nullptr);SendMessageW(statusWin,WM_SETFONT,(WPARAM)font,TRUE);
    glDC=GetDC(viewWin);PIXELFORMATDESCRIPTOR pd{};pd.nSize=sizeof(pd);pd.nVersion=1;pd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;pd.iPixelType=PFD_TYPE_RGBA;pd.cColorBits=24;pd.cDepthBits=24;
    int pf=ChoosePixelFormat(glDC,&pd);if(!pf||!SetPixelFormat(glDC,pf,&pd)||!(glRC=wglCreateContext(glDC))){MessageBoxW(mainWin,L"Не удалось создать контекст OpenGL.",L"Ошибка",MB_ICONERROR);return 1;}
    wglMakeCurrent(glDC,glRC);HFONT overlayFont=CreateFontW(-15,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,ANSI_CHARSET,0,0,NONANTIALIASED_QUALITY,0,L"Arial");HGDIOBJ previousFont=SelectObject(glDC,overlayFont);textList=glGenLists(1280);if(!wglUseFontBitmapsW(glDC,0,1280,textList)){glDeleteLists(textList,1280);textList=0;}SelectObject(glDC,previousFont);DeleteObject(overlayFont);HFONT volumeFont=CreateFontW(-30,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,ANSI_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Segoe UI");previousFont=SelectObject(glDC,volumeFont);bigTextList=glGenLists(1280);if(!wglUseFontBitmapsW(glDC,0,1280,bigTextList)){glDeleteLists(bigTextList,1280);bigTextList=0;}SelectObject(glDC,previousFont);DeleteObject(volumeFont);
    loadPreferences();applyTheme(lightTheme);
    ShowWindow(mainWin,show);RECT rc;GetClientRect(mainWin,&rc);SendMessageW(mainWin,WM_SIZE,0,MAKELPARAM(rc.right,rc.bottom));UpdateWindow(mainWin);
    int argc=0;LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);if(argc>3&&std::wstring(argv[1])==L"--pair-test"){v2TestMode=true;enqueueFiles({argv[2]});renderTestDir=argv[3];}else if(argc>4&&std::wstring(argv[1])==L"--v2-test"){v2TestMode=true;renderTestDir=argv[4];enqueueFiles({argv[2],argv[3]});}else if(argc>3 && std::wstring(argv[1])==L"--render-test"){renderTestDir=argv[3];startLoad(argv[2]);}else if(argc>1)enqueueFiles({argv[1]});LocalFree(argv);
    MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){if(msg.message==WM_KEYDOWN&&(msg.wParam==VK_ESCAPE||msg.wParam=='R'||msg.wParam==VK_HOME||msg.wParam=='1'||msg.wParam=='2'||((msg.wParam=='O'||msg.wParam=='S')&&(GetKeyState(VK_CONTROL)&0x8000)))){SendMessageW(mainWin,msg.message,msg.wParam,msg.lParam);continue;}if(!IsDialogMessageW(mainWin,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}return 0;
}
