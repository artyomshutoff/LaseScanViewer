#include "../src/registration.hpp"
#include <iostream>
#include <cassert>
int main(){
 auto& r=compute::runtime();std::cout<<"GPU: "<<r.name<<" ready="<<r.ready<<" "<<r.reason<<'\n';if(!r.ready)return 2;
 std::vector<Point> points,queries;uint64_t state=37;auto random=[&]{state=state*6364136223846793005ULL+1;return float(int32_t(state>>32))*.000005f;};
 for(int i=0;i<3000;i++)points.push_back({random(),random(),random()});for(int i=0;i<5000;i++)queries.push_back({random(),random(),random()});
 points.push_back({1,2,3});points.push_back({1,2,3});queries.push_back({1,2,3});registration::Tree tree(points);
 compute::choose(compute::Backend::CPU);auto cpu=tree.nearestBatch(queries);auto before=compute::gpuBatches.load();compute::choose(compute::Backend::GPU);auto gpu=tree.nearestBatch(queries);
 assert(compute::gpuBatches.load()>before);for(size_t i=0;i<cpu.size();i++){assert(cpu[i].first==gpu[i].first);assert(cpu[i].second.x==gpu[i].second.x&&cpu[i].second.y==gpu[i].second.y&&cpu[i].second.z==gpu[i].second.z);}
 queries.resize(5);gpu=tree.nearestBatch(queries);assert(compute::gpuBatches.load()>before+1);
 r.ready=false;auto fallbacks=compute::cpuFallbacks.load();auto fallback=tree.nearestBatch(queries);assert(compute::cpuFallbacks.load()>fallbacks);for(size_t i=0;i<queries.size();i++)assert(fallback[i].first==cpu[i].first);r.ready=true;
 compute::choose(compute::Backend::CPU);std::cout<<"PASS 5001 exact nearest distances/points, ties, resized batch and CPU fallback\n";
}
