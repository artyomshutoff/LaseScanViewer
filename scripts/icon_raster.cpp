#include <windows.h>
#include <cmath>
#include <fstream>
#include "../src/logo.hpp"
int main(){
    constexpr int size=1024;
    BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=size;bi.bmiHeader.biHeight=-size;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
    void* pixels=nullptr;HDC dc=CreateCompatibleDC(nullptr);HBITMAP bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&pixels,nullptr,0);if(!bitmap||!dc)return 1;auto previous=SelectObject(dc,bitmap);
    RECT r{0,0,size,size};SetDCBrushColor(dc,RGB(99,172,80));FillRect(dc,&r,(HBRUSH)GetStockObject(DC_BRUSH));
    drawBrandLogo(dc,{75,403,949,620},RGB(255,255,255));GdiFlush();
    BITMAPFILEHEADER h{};h.bfType=0x4d42;h.bfOffBits=sizeof(h)+sizeof(BITMAPINFOHEADER);h.bfSize=h.bfOffBits+size*size*4;
    std::ofstream out("build/icon314/icon.bmp",std::ios::binary);out.write((char*)&h,sizeof(h));out.write((char*)&bi.bmiHeader,sizeof(bi.bmiHeader));out.write((char*)pixels,size*size*4);
    SelectObject(dc,previous);DeleteObject(bitmap);DeleteDC(dc);return out?0:1;
}
