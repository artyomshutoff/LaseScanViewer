#include "model.hpp"
#include <iostream>
int wmain(int argc,wchar_t** argv){try{if(argc<2)return 2;auto m=readModel(std::filesystem::path(argv[1]));triangulate(m);std::cout<<"profiles="<<m.profiles.size()<<" points="<<m.points.size()<<" faces="<<m.faces.size()<<" rejected="<<m.rejected<<"\n";std::cout<<"min="<<m.lo.x<<","<<m.lo.y<<","<<m.lo.z<<" max="<<m.hi.x<<","<<m.hi.y<<","<<m.hi.z<<"\n";if(argc>2)exportPly(m,std::filesystem::path(argv[2]),true);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
