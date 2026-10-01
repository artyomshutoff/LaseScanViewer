#pragma once
#include <vector>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <stdexcept>
// Lossless PNG encoder with stored DEFLATE blocks. No external runtime DLLs.
inline void be32(std::vector<uint8_t>& b,uint32_t n){for(int s=24;s>=0;s-=8)b.push_back(uint8_t(n>>s));}
inline uint32_t crc32(const uint8_t* p,size_t n){uint32_t c=~0u;for(size_t i=0;i<n;i++){c^=p[i];for(int j=0;j<8;j++)c=(c>>1)^((c&1)?0xedb88320u:0);}return ~c;}
inline void chunk(std::vector<uint8_t>& png,const char* type,const std::vector<uint8_t>& data){be32(png,uint32_t(data.size()));size_t start=png.size();png.insert(png.end(),type,type+4);png.insert(png.end(),data.begin(),data.end());be32(png,crc32(png.data()+start,4+data.size()));}
inline std::vector<uint8_t> encodePng(int w,int h,const std::vector<uint8_t>& rgb){
    if(w<1||h<1||rgb.size()!=size_t(w)*h*3)throw std::runtime_error("Invalid PNG dimensions");
    std::vector<uint8_t> raw;raw.reserve(size_t(h)*(1+size_t(w)*3));for(int y=0;y<h;y++){raw.push_back(0);raw.insert(raw.end(),rgb.begin()+size_t(y)*w*3,rgb.begin()+size_t(y+1)*w*3);}
    std::vector<uint8_t> z{0x78,0x01};uint32_t a=1,b=0;for(auto v:raw){a=(a+v)%65521;b=(b+a)%65521;}
    for(size_t p=0;p<raw.size();){uint16_t n=uint16_t(std::min<size_t>(65535,raw.size()-p));z.push_back(p+n==raw.size()?1:0);z.push_back(uint8_t(n));z.push_back(uint8_t(n>>8));uint16_t inv=uint16_t(~n);z.push_back(uint8_t(inv));z.push_back(uint8_t(inv>>8));z.insert(z.end(),raw.begin()+p,raw.begin()+p+n);p+=n;}be32(z,(b<<16)|a);
    std::vector<uint8_t> png{137,80,78,71,13,10,26,10},ih;be32(ih,w);be32(ih,h);ih.insert(ih.end(),{8,2,0,0,0});chunk(png,"IHDR",ih);chunk(png,"IDAT",z);chunk(png,"IEND",{});return png;
}
inline void writeBytes(const std::filesystem::path& path,const std::vector<uint8_t>& data){std::ofstream f(path,std::ios::binary);if(!f.write((const char*)data.data(),data.size()))throw std::runtime_error("Cannot write output file");}
inline std::string base64(const std::vector<uint8_t>& b){static const char* t="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";std::string s;for(size_t i=0;i<b.size();i+=3){uint32_t v=uint32_t(b[i])<<16;if(i+1<b.size())v|=uint32_t(b[i+1])<<8;if(i+2<b.size())v|=b[i+2];s+=t[(v>>18)&63];s+=t[(v>>12)&63];s+=i+1<b.size()?t[(v>>6)&63]:'=';s+=i+2<b.size()?t[v&63]:'=';}return s;}
