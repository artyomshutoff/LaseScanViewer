#pragma once
#include <wincodec.h>
// Portable PDF 1.4 writer: lossless, rasterized GDI pages preserve Cyrillic fonts.
// No printer, browser, Python or PDF runtime is required by the executable.
namespace reportPdf {
struct Page {int width=1240,height=1754;std::vector<uint8_t> rgb;};
template<class T> struct Com {T* p=nullptr;~Com(){if(p)p->Release();}T** address(){return &p;}T* operator->(){return p;}};
inline std::vector<uint8_t> compressed(const Page& page){
 struct Init {HRESULT hr=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);~Init(){if(SUCCEEDED(hr))CoUninitialize();}} init;
 auto check=[](HRESULT hr){if(FAILED(hr))throw std::runtime_error("PDF compression failed: "+std::to_string(hr));};Com<IWICImagingFactory> factory;Com<IStream> stream;Com<IWICBitmapEncoder> encoder;Com<IWICBitmapFrameEncode> frame;
 check(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_IWICImagingFactory,(void**)factory.address()));check(CreateStreamOnHGlobal(nullptr,TRUE,stream.address()));check(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,encoder.address()));check(encoder->Initialize(stream.p,WICBitmapEncoderNoCache));check(encoder->CreateNewFrame(frame.address(),nullptr));check(frame->Initialize(nullptr));check(frame->SetSize(page.width,page.height));WICPixelFormatGUID format=GUID_WICPixelFormat24bppRGB;check(frame->SetPixelFormat(&format));std::vector<uint8_t> pixels=page.rgb;if(IsEqualGUID(format,GUID_WICPixelFormat24bppBGR)){for(size_t i=0;i<pixels.size();i+=3)std::swap(pixels[i],pixels[i+2]);}else if(!IsEqualGUID(format,GUID_WICPixelFormat24bppRGB))throw std::runtime_error("PDF PNG pixel format mismatch");check(frame->WritePixels(page.height,page.width*3,UINT(pixels.size()),pixels.data()));check(frame->Commit());check(encoder->Commit());
 HGLOBAL handle=nullptr;check(GetHGlobalFromStream(stream.p,&handle));auto* data=(const uint8_t*)GlobalLock(handle);if(!data)throw std::runtime_error("Cannot access PDF compressed image");size_t size=GlobalSize(handle);std::vector<uint8_t> bytes(data,data+size);GlobalUnlock(handle);
 auto number=[&](size_t p){return uint32_t(bytes[p])<<24|uint32_t(bytes[p+1])<<16|uint32_t(bytes[p+2])<<8|bytes[p+3];};std::vector<uint8_t> result;
 for(size_t p=8;p+12<=bytes.size();){uint32_t n=number(p);if(n>bytes.size()-p-12)break;std::string type((const char*)bytes.data()+p+4,4);if(type=="IHDR"&&(n!=13||bytes[p+16]!=8||bytes[p+17]!=2||bytes[p+20]!=0))throw std::runtime_error("Unsupported PNG layout in PDF");if(type=="IDAT")result.insert(result.end(),bytes.begin()+p+8,bytes.begin()+p+8+n);if(type=="IEND")break;p+=n+12;}
 if(result.empty())throw std::runtime_error("PDF compressed image missing");return result;
}
inline void save(const std::filesystem::path& path,const std::vector<Page>& pages){
 std::vector<std::vector<uint8_t>> encoded;for(const auto& page:pages)encoded.push_back(compressed(page));
 std::ofstream out(path,std::ios::binary);if(!out)throw std::runtime_error("Cannot create PDF");out.imbue(std::locale::classic());out<<"%PDF-1.4\n%\xE2\xE3\xCF\xD3\n";std::vector<std::streamoff> offsets(3+pages.size()*3);
 auto object=[&](int n,const std::string& content){offsets[n]=out.tellp();out<<n<<" 0 obj\n"<<content<<"\nendobj\n";};
 object(1,"<< /Type /Catalog /Pages 2 0 R >>");std::string kids="[";for(size_t i=0;i<pages.size();i++)kids+=std::to_string(3+i*3)+" 0 R ";kids+="]";object(2,"<< /Type /Pages /Count "+std::to_string(pages.size())+" /Kids "+kids+" >>");
 for(size_t i=0;i<pages.size();i++){const auto& p=pages[i];int n=3+int(i)*3;object(n,"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595.28 841.89] /Resources << /XObject << /Im "+std::to_string(n+2)+" 0 R >> >> /Contents "+std::to_string(n+1)+" 0 R >>");std::string commands="q 595.28 0 0 841.89 0 0 cm /Im Do Q\n";object(n+1,"<< /Length "+std::to_string(commands.size())+" >>\nstream\n"+commands+"endstream");
  const auto& image=encoded[i];offsets[n+2]=out.tellp();out<<n+2<<" 0 obj\n<< /Type /XObject /Subtype /Image /Width "<<p.width<<" /Height "<<p.height<<" /ColorSpace /DeviceRGB /BitsPerComponent 8 /Length "<<image.size()<<" /Filter /FlateDecode /DecodeParms << /Predictor 15 /Colors 3 /BitsPerComponent 8 /Columns "<<p.width<<" >> >>\nstream\n";out.write((const char*)image.data(),image.size());out<<"\nendstream\nendobj\n";
 }
 auto xref=out.tellp();out<<"xref\n0 "<<offsets.size()<<"\n0000000000 65535 f \n";for(size_t i=1;i<offsets.size();i++)out<<std::setw(10)<<std::setfill('0')<<offsets[i]<<" 00000 n \n";out<<"trailer\n<< /Size "<<offsets.size()<<" /Root 1 0 R >>\nstartxref\n"<<xref<<"\n%%EOF\n";out.flush();if(!out)throw std::runtime_error("PDF write failed");
}
struct Canvas {
 HDC dc=CreateCompatibleDC(nullptr);HBITMAP bitmap=nullptr;HGDIOBJ old=nullptr;HFONT normal=nullptr,title=nullptr;uint8_t* bits=nullptr;
 Canvas(){BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=1240;bi.bmiHeader.biHeight=-1754;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,(void**)&bits,nullptr,0);if(!bitmap)throw std::runtime_error("Cannot allocate PDF page");old=SelectObject(dc,bitmap);normal=CreateFontW(-26,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Segoe UI");title=CreateFontW(-43,0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Segoe UI");SetBkMode(dc,TRANSPARENT);clear();}
 ~Canvas(){SelectObject(dc,old);DeleteObject(bitmap);DeleteObject(normal);DeleteObject(title);DeleteDC(dc);}
 void clear(){RECT r{0,0,1240,1754};FillRect(dc,&r,(HBRUSH)GetStockObject(WHITE_BRUSH));SelectObject(dc,normal);SetTextColor(dc,RGB(30,46,56));}
 int text(const std::wstring& s,RECT r,bool heading=false,bool measure=false){SelectObject(dc,heading?title:normal);DrawTextW(dc,s.data(),int(s.size()),&r,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX|(measure?DT_CALCRECT:0));return r.bottom-r.top;}
 Page page(){GdiFlush();Page p;p.rgb.resize(size_t(1240)*1754*3);for(size_t i=0;i<p.rgb.size()/3;i++){p.rgb[i*3]=bits[i*4+2];p.rgb[i*3+1]=bits[i*4+1];p.rgb[i*3+2]=bits[i*4];}return p;}
};
}
void writePdfReport(const std::filesystem::path& path){
 auto rows=reportRows();reportPdf::Canvas c;std::vector<reportPdf::Page> pages;int y=0;
 auto header=[&]{c.text(L"LaseScanViewer | Отчёт",{80,68,1160,130},true);c.text(L"Измерение "+std::to_wstring(model.scan)+L" · "+loadedPath.filename().wstring(),{80,145,1160,245});y=260;};
 auto finish=[&]{c.text(L"LaseScanViewer · "+std::to_wstring(pages.size()+1)+L" | Масштаб координат задан в настройках",{80,1660,1160,1715});pages.push_back(c.page());c.clear();header();};header();
 c.text(comparisonSummary(),{80,y,1160,y+160});y+=145;for(const auto& line:controlLines()){c.text(line,{80,y,1160,y+42});y+=42;}
 render();glFinish();glReadBuffer(GL_FRONT);glPixelStorei(GL_PACK_ALIGNMENT,1);std::vector<uint8_t> image(size_t(viewW)*viewH*3);glReadPixels(0,0,viewW,viewH,GL_RGB,GL_UNSIGNED_BYTE,image.data());if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("PDF preview capture failed");for(size_t i=0;i<image.size();i+=3)std::swap(image[i],image[i+2]);
 // DIB RGB uses bottom-up rows and a four-byte stride.
 int stride=(viewW*3+3)&~3;std::vector<uint8_t> padded(size_t(stride)*viewH);for(int row=0;row<viewH;row++)std::copy_n(image.data()+size_t(row)*viewW*3,size_t(viewW)*3,padded.data()+size_t(row)*stride);
 BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=viewW;bi.bmiHeader.biHeight=viewH;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=24;int h=std::min(860,1580-y);int w=1080;if(double(viewW)/viewH<double(w)/h)w=int(double(h)*viewW/viewH);else h=int(double(w)*viewH/viewW);SetStretchBltMode(c.dc,HALFTONE);StretchDIBits(c.dc,80+(1080-w)/2,y,w,h,0,0,viewW,viewH,padded.data(),&bi,DIB_RGB_COLORS,SRCCOPY);finish();
 c.text(L"Параметры и результаты",{80,y,1160,y+70},true);y+=86;
 for(auto& row:rows){auto a=widen(row.first),b=widen(row.second);int height=std::max(c.text(a,{80,0,425,0},false,true),c.text(b,{455,0,1160,0},false,true))+28;if(y+height>1610)finish();c.text(a,{80,y,425,y+height-20});c.text(b,{455,y,1160,y+height-20});y+=height;SelectObject(c.dc,GetStockObject(DC_PEN));SetDCPenColor(c.dc,RGB(210,218,222));MoveToEx(c.dc,80,y-10,nullptr);LineTo(c.dc,1160,y-10);}
 if(y+130>1610)finish();
 c.text(L"Оценка по сканам зависит от масштаба, совмещения и выбранной области. Сверка с базой не изменяет вычисленный объём.",{80,y+24,1160,1640});finish();reportPdf::save(path,pages);
}
