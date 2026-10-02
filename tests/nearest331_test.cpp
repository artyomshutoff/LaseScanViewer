#include "../src/registration.hpp"
#include <cassert>
#include <iostream>
void legacy(const registration::Tree&t,int i,Point p,double&best,Point&q){if(i<0)return;const auto&n=t.nodes[i];double d=registration::distance(p,n.p);if(d<best){best=d;q=n.p;}double split=registration::coord(p,n.axis)-registration::coord(n.p,n.axis);legacy(t,split<0?n.left:n.right,p,best,q);if(split*split<best)legacy(t,split<0?n.right:n.left,p,best,q);}
int main(){uint64_t state=1337;auto random=[&]{state=state*6364136223846793005ULL+1;return float(int32_t(state>>32))*1e-5f;};
 std::vector<Point> points;for(int i=0;i<16000;++i)points.push_back({random(),random(),random()});for(int i=0;i<100;++i)points.push_back(points[i]);registration::Tree tree(points);
 for(int i=0;i<100000;++i){Point p=i<16000?points[i]:Point{random(),random(),random()};double d=std::numeric_limits<double>::max();Point q{};legacy(tree,0,p,d,q);auto found=tree.nearest(p);assert(found.first==d&&found.second.x==q.x&&found.second.y==q.y&&found.second.z==q.z);double best=std::numeric_limits<double>::infinity();int oldIndex=-1;registration::SurfaceCloud::nearestNode(tree,0,p,best,oldIndex);assert(tree.nearestIndex(p)==oldIndex);}
 registration::Tree tied({{-1,0,0},{1,0,0},{0,-1,0},{0,1,0}});double d=std::numeric_limits<double>::max();Point q{};legacy(tied,0,{0,0,0},d,q);auto found=tied.nearest({0,0,0});assert(found.first==d&&found.second.x==q.x&&found.second.y==q.y&&found.second.z==q.z);
 std::cout<<"PASS 100000 exact nearest-neighbour queries, duplicates and ties\n";
}
