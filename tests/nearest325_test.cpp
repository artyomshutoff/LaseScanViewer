#include "../src/registration.hpp"
#include <random>
#include <cassert>
#include <iostream>
int main(){std::mt19937 rng(42);std::uniform_real_distribution<float> rand(-10000,10000);std::vector<registration::SurfacePoint> points;for(int i=0;i<8000;i++)points.push_back({{rand(rng),rand(rng),rand(rng)},{0,1,0}});registration::SurfaceCloud cloud(points);std::vector<Point> query;for(int i=0;i<50000;i++)query.push_back({rand(rng),rand(rng),rand(rng)});std::vector<int> old;
 auto start=std::chrono::steady_clock::now();for(auto p:query){std::vector<std::pair<double,int>> h;registration::closestK(cloud.tree,0,p,h,1);old.push_back(h[0].second);}double before=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
 start=std::chrono::steady_clock::now();for(size_t i=0;i<query.size();i++)assert(cloud.nearest(query[i])==old[i]);double after=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();std::cout<<"PASS 50000 exact nearest indices: "<<before<<" s -> "<<after<<" s\n";
}
