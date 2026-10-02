#pragma once
// Included inside registration namespace. Rescue XY/yaw using locally planar,
// steep structure shared by Full and Empty, excluding horizontal bed/material.
inline void closestK(const Tree& tree,int node,Point p,std::vector<std::pair<double,int>>& heap,size_t k){
 if(node<0)return;const auto& n=tree.nodes[node];double d=distance(p,n.p);if(heap.size()<k){heap.push_back({d,node});std::push_heap(heap.begin(),heap.end());}else if(d<heap.front().first){std::pop_heap(heap.begin(),heap.end());heap.back()={d,node};std::push_heap(heap.begin(),heap.end());}double split=coord(p,n.axis)-coord(n.p,n.axis);closestK(tree,split<0?n.left:n.right,p,heap,k);if(heap.size()<k||split*split<heap.front().first)closestK(tree,split<0?n.right:n.left,p,heap,k);
}
struct SurfacePoint {Point p,n;};
inline std::vector<SurfacePoint> steepSurfaces(const Model& m,double voxel){
 auto points=sample(m,voxel,16000);trimGround(points);Tree all(points);std::vector<SurfacePoint> out;std::vector<std::pair<double,int>> neighbours;neighbours.reserve(24);
 for(auto p:points){neighbours.clear();closestK(all,0,p,neighbours,20);if(neighbours.size()<20||neighbours.front().first>voxel*voxel*36)continue;double mean[3]={};for(auto h:neighbours)for(int d=0;d<3;d++)mean[d]+=coord(all.nodes[h.second].p,d)/neighbours.size();double cov[3][3]={},v[3][3]={{1,0,0},{0,1,0},{0,0,1}};
  for(auto h:neighbours){auto q=all.nodes[h.second].p;for(int i=0;i<3;i++)for(int j=0;j<3;j++)cov[i][j]+=(coord(q,i)-mean[i])*(coord(q,j)-mean[j]);}
  for(int iter=0;iter<18;iter++){int i=0,j=1;for(int x=0;x<3;x++)for(int y=x+1;y<3;y++)if(std::abs(cov[x][y])>std::abs(cov[i][j])){i=x;j=y;}if(std::abs(cov[i][j])<1e-9)break;double angle=.5*std::atan2(2*cov[i][j],cov[j][j]-cov[i][i]),c=std::cos(angle),s=std::sin(angle);double ii=c*c*cov[i][i]-2*c*s*cov[i][j]+s*s*cov[j][j],jj=s*s*cov[i][i]+2*c*s*cov[i][j]+c*c*cov[j][j];for(int x=0;x<3;x++)if(x!=i&&x!=j){double a=cov[x][i],b=cov[x][j];cov[x][i]=cov[i][x]=c*a-s*b;cov[x][j]=cov[j][x]=s*a+c*b;}cov[i][i]=ii;cov[j][j]=jj;cov[i][j]=cov[j][i]=0;for(int x=0;x<3;x++){double a=v[x][i],b=v[x][j];v[x][i]=c*a-s*b;v[x][j]=s*a+c*b;}}
  std::array<int,3> order{0,1,2};std::sort(order.begin(),order.end(),[&](int a,int b){return cov[a][a]<cov[b][b];});int smallest=order[0];if(cov[smallest][smallest]>.12*std::max(1.,cov[order[1]][order[1]])||std::abs(v[2][smallest])>.45)continue;out.push_back({p,{float(v[0][smallest]),float(v[1][smallest]),float(v[2][smallest])}});
 }return out;
}
struct SurfaceCloud {
 Tree tree;std::vector<Point> normals;
 static std::vector<Point> points(const std::vector<SurfacePoint>& data){std::vector<Point> out;for(auto s:data)out.push_back(s.p);return out;}
 explicit SurfaceCloud(const std::vector<SurfacePoint>& data):tree(points(data)){std::map<std::tuple<float,float,float>,Point> lookup;for(auto s:data)lookup[{s.p.x,s.p.y,s.p.z}]=s.n;for(auto n:tree.nodes)normals.push_back(lookup.at({n.p.x,n.p.y,n.p.z}));}
 static void nearestNode(const Tree& tree,int node,Point p,double& best,int& index){
  if(node<0)return;const auto& n=tree.nodes[node];double d=distance(p,n.p);
  if(index<0||d<best){best=d;index=node;}
  double split=coord(p,n.axis)-coord(n.p,n.axis);
  nearestNode(tree,split<0?n.left:n.right,p,best,index);
  if(split*split<best)nearestNode(tree,split<0?n.right:n.left,p,best,index);
 }
 int nearest(Point p)const{return tree.nearestIndex(p);}
};
struct StructureFit {double cost=1e30,coverage=0,rms=0;size_t anchors=0;bool spread=false;};
inline StructureFit structuralScore(const SurfaceCloud& a,const SurfaceCloud& b,const CompareOptions& o,double span){
 double cap=span*.025,sum=0,sq=0;size_t total=a.tree.nodes.size()+b.tree.nodes.size(),valid=0;PreparedBaseTransform transform(o);double rad=o.angle*3.141592653589793/180,c=std::cos(rad),s=std::sin(rad);Point lo{1e30f,1e30f,0},hi{-1e30f,-1e30f,0},allLo=lo,allHi=hi;
 for(int direction=0;direction<2;direction++){const auto& source=direction?a:b;const auto& target=direction?b:a;
  for(size_t i=0;i<source.tree.nodes.size();i++){auto original=source.tree.nodes[i].p;Point p=direction?Point{float(c*(original.x-o.dx)+s*(original.y-o.dy)),float(-s*(original.x-o.dx)+c*(original.y-o.dy)),float(original.z-o.dz)}:transform(original);auto n=source.normals[i];Point normal{float(c*n.x+(direction?s:-s)*n.y),float((direction?-s:s)*n.x+c*n.y),n.z};int j=target.nearest(p);auto q=target.tree.nodes[j].p,tn=target.normals[j];double dot=std::abs(normal.x*tn.x+normal.y*tn.y+normal.z*tn.z),d=distance(p,q);sum+=dot>.9?std::min(d,cap*cap):cap*cap;if(dot>.9&&d<cap*cap){++valid;sq+=d;if(!direction){lo.x=std::min(lo.x,original.x);lo.y=std::min(lo.y,original.y);hi.x=std::max(hi.x,original.x);hi.y=std::max(hi.y,original.y);}}if(!direction){allLo.x=std::min(allLo.x,original.x);allLo.y=std::min(allLo.y,original.y);allHi.x=std::max(allHi.x,original.x);allHi.y=std::max(allHi.y,original.y);}}
 }return {sum/std::max<size_t>(1,total),double(valid)/std::max<size_t>(1,total),valid?std::sqrt(sq/valid):cap,valid,valid>=150&&(hi.x-lo.x)>.7*(allHi.x-allLo.x)&&(hi.y-lo.y)>.6*(allHi.y-allLo.y)};
}
inline bool solveStructure(double matrix[3][4],double answer[3]){for(int i=0;i<3;i++){int pivot=i;for(int j=i+1;j<3;j++)if(std::abs(matrix[j][i])>std::abs(matrix[pivot][i]))pivot=j;if(std::abs(matrix[pivot][i])<1e-7)return false;for(int k=i;k<4;k++)std::swap(matrix[i][k],matrix[pivot][k]);double scale=matrix[i][i];for(int k=i;k<4;k++)matrix[i][k]/=scale;for(int j=0;j<3;j++)if(j!=i){double factor=matrix[j][i];for(int k=i;k<4;k++)matrix[j][k]-=factor*matrix[i][k];}}for(int i=0;i<3;i++)answer[i]=matrix[i][3];return true;}
inline CompareOptions fitStructure(const SurfaceCloud& a,const SurfaceCloud& b,CompareOptions o,double span){
 for(int iteration=0;iteration<45;iteration++){PreparedBaseTransform transform(o);double rad=o.angle*3.141592653589793/180,cs=std::cos(rad),sn=std::sin(rad);struct Pair{Point p,q,n;};std::vector<Pair> pairs;double cx=0,cy=0;
  for(size_t i=0;i<b.tree.nodes.size();i++){auto p=transform(b.tree.nodes[i].p);int j=a.nearest(p);auto q=a.tree.nodes[j].p,n=a.normals[j],bn=b.normals[i];double dot=std::abs((cs*bn.x-sn*bn.y)*n.x+(sn*bn.x+cs*bn.y)*n.y+bn.z*n.z);if(distance(p,q)>span*span*.000625||dot<.9)continue;pairs.push_back({p,q,n});cx+=p.x;cy+=p.y;}
  if(pairs.size()<100)break;cx/=pairs.size();cy/=pairs.size();double matrix[3][4]={};for(auto m:pairs){double residual=(m.p.x-m.q.x)*m.n.x+(m.p.y-m.q.y)*m.n.y+(m.p.z-m.q.z)*m.n.z,w=std::min(1.,span*.0023/std::max(1.,std::abs(residual)));double jac[]={m.n.x,m.n.y,(-(m.p.y-cy)*m.n.x+(m.p.x-cx)*m.n.y)/span};for(int i=0;i<3;i++){for(int j=0;j<3;j++)matrix[i][j]+=w*jac[i]*jac[j];matrix[i][3]-=w*jac[i]*residual;}}double step[3];if(!solveStructure(matrix,step))break;double da=std::clamp(step[2]/span,-.015,.015),c=std::cos(da),s=std::sin(da);double dx=std::clamp(step[0],-span*.02,span*.02),dy=std::clamp(step[1],-span*.02,span*.02),oldx=o.dx;o.dx=c*(oldx-cx)-s*(o.dy-cy)+cx+dx;o.dy=s*(oldx-cx)+c*(o.dy-cy)+cy+dy;o.angle+=da*180/3.141592653589793;if(std::abs(da)<1e-7&&std::hypot(dx,dy)<span*1e-6)break;
 }return o;
}
inline std::optional<CompareOptions> rescueStructure(const Model& active,const Model& base,const CompareOptions& original,double span){
 SurfaceCloud a(steepSurfaces(active,span/300)),b(steepSurfaces(base,span/300));if(a.tree.nodes.size()<150||b.tree.nodes.size()<150)return {};auto before=structuralScore(a,b,original,span),best=before;auto winner=original;bool xAxis=active.hi.x-active.lo.x>active.hi.y-active.lo.y;
 struct Candidate {CompareOptions pose;StructureFit score;};
 const std::array<double,5> localOffsets{0.,-.08,-.04,.04,.08};std::array<std::optional<Candidate>,5> local;
 parallelJobs(localOffsets.size(),[&](size_t i){auto seed=original;double offset=localOffsets[i];if(xAxis)seed.dx+=offset*span;else seed.dy+=offset*span;auto fitted=fitStructure(a,b,seed,span);if(std::abs(std::remainder(fitted.angle-original.angle,360.))>5||std::hypot(fitted.dx-original.dx,fitted.dy-original.dy)>span*.12)return;local[i]=Candidate{fitted,structuralScore(a,b,fitted,span)};});
 for(const auto& candidate:local)if(candidate&&candidate->score.spread&&candidate->score.cost<best.cost){best=candidate->score;winner=candidate->pose;}
 if(best.spread&&best.coverage>=before.coverage+.04&&best.coverage>before.coverage*1.3&&best.cost<before.cost*.94)return winner;
 // Long side walls admit a false longitudinal match after a reversed scan.
 // The local window cannot escape a displacement of several metres. Try a
 // wider window only after the established local rescue failed, and require
 // independent starting offsets to converge on the same distributed structure.
 std::vector<Candidate> broad;const std::array<double,8> broadOffsets{-.24,-.20,-.16,-.12,.12,.16,.20,.24};std::array<std::optional<Candidate>,8> broadResults;
 parallelJobs(broadOffsets.size(),[&](size_t i){double offset=broadOffsets[i];
  auto seed=original;if(xAxis)seed.dx+=offset*span;else seed.dy+=offset*span;
  auto fitted=fitStructure(a,b,seed,span);double movement=std::hypot(fitted.dx-original.dx,fitted.dy-original.dy);
  if(std::abs(std::remainder(fitted.angle-original.angle,360.))>5||movement<=span*.12||movement>span*.28)return;
  auto score=structuralScore(a,b,fitted,span);
  if(score.spread&&score.coverage>=before.coverage+.05&&score.coverage>before.coverage*1.12&&score.cost<before.cost*.93&&score.rms<before.rms*.97)broadResults[i]=Candidate{fitted,score};
 });
 for(const auto& candidate:broadResults)if(candidate)broad.push_back(*candidate);
 std::sort(broad.begin(),broad.end(),[](const Candidate& x,const Candidate& y){return x.score.cost<y.score.cost;});
 for(const auto& candidate:broad){size_t agreement=0;for(const auto& other:broad)if(std::abs(std::remainder(candidate.pose.angle-other.pose.angle,360.))<.5&&std::hypot(candidate.pose.dx-other.pose.dx,candidate.pose.dy-other.pose.dy)<span*.015)++agreement;if(agreement>=2)return candidate.pose;}
 return {};
}


