#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#define CL_TARGET_OPENCL_VERSION 120
#include "vendor/CL/cl.h"
#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include "model.hpp"
namespace compute {
enum class Backend {CPU,GPU};
inline std::atomic<Backend> selected{Backend::CPU};
inline std::atomic<uint64_t> gpuBatches{0},cpuFallbacks{0};
inline void choose(Backend b){selected.store(b);}
inline const char* source=R"CL(
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#pragma OPENCL FP_CONTRACT OFF
typedef struct {float x,y,z;int left,right,axis;} Node;
typedef struct {float x,y,z;} Point;
__kernel void nearest(__global const Node* tree,__global const Point* query,
                      __global double* distances,__global Point* points,int count){
 int id=get_global_id(0);if(id>=count)return;Point p=query[id],q={0,0,0};
 double best=1.7976931348623157e308;int nodes[64];double limits[64];int top=1;nodes[0]=0;limits[0]=-1;
 while(top){--top;int n=nodes[top];double limit=limits[top];if(n<0||(limit>=0&&!(limit<best)))continue;
  Node a=tree[n];float fx=p.x-a.x,fy=p.y-a.y,fz=p.z-a.z;double x=fx,y=fy,z=fz;
  double d=(x*x+y*y)+z*z;if(d<best){best=d;q.x=a.x;q.y=a.y;q.z=a.z;}
  double split=a.axis==0?(double)p.x-(double)a.x:a.axis==1?(double)p.y-(double)a.y:(double)p.z-(double)a.z;
  int near=split<0?a.left:a.right,far=split<0?a.right:a.left;
  if(far>=0){nodes[top]=far;limits[top++]=split*split;}
  if(near>=0){nodes[top]=near;limits[top++]=-1;}
 }
 distances[id]=best;points[id]=q;
}
)CL";
struct Runtime {
    HMODULE dll=nullptr;cl_device_id device=nullptr;cl_context context=nullptr;cl_command_queue queue=nullptr;cl_program program=nullptr;cl_kernel kernel=nullptr;
    std::mutex lock;std::string name,reason;bool ready=false;
#define GPU_API(name) decltype(&::name) name=nullptr;
    GPU_API(clGetPlatformIDs) GPU_API(clGetDeviceIDs) GPU_API(clGetDeviceInfo) GPU_API(clCreateContext) GPU_API(clCreateCommandQueue)
    GPU_API(clCreateProgramWithSource) GPU_API(clBuildProgram) GPU_API(clGetProgramBuildInfo) GPU_API(clCreateKernel) GPU_API(clCreateBuffer)
    GPU_API(clSetKernelArg) GPU_API(clEnqueueWriteBuffer) GPU_API(clEnqueueNDRangeKernel) GPU_API(clEnqueueReadBuffer)
    GPU_API(clReleaseMemObject) GPU_API(clReleaseKernel) GPU_API(clReleaseProgram) GPU_API(clReleaseCommandQueue) GPU_API(clReleaseContext)
#undef GPU_API
    void check(cl_int code){if(code!=CL_SUCCESS)throw std::runtime_error("OpenCL error "+std::to_string(code));}
    Runtime(){try{
        dll=LoadLibraryExW(L"OpenCL.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!dll)throw std::runtime_error("OpenCL driver unavailable");
#define GPU_LOAD(name) name=(decltype(name))GetProcAddress(dll,#name);if(!name)throw std::runtime_error("Missing " #name);
        GPU_LOAD(clGetPlatformIDs) GPU_LOAD(clGetDeviceIDs) GPU_LOAD(clGetDeviceInfo) GPU_LOAD(clCreateContext) GPU_LOAD(clCreateCommandQueue)
        GPU_LOAD(clCreateProgramWithSource) GPU_LOAD(clBuildProgram) GPU_LOAD(clGetProgramBuildInfo) GPU_LOAD(clCreateKernel) GPU_LOAD(clCreateBuffer)
        GPU_LOAD(clSetKernelArg) GPU_LOAD(clEnqueueWriteBuffer) GPU_LOAD(clEnqueueNDRangeKernel) GPU_LOAD(clEnqueueReadBuffer)
        GPU_LOAD(clReleaseMemObject) GPU_LOAD(clReleaseKernel) GPU_LOAD(clReleaseProgram) GPU_LOAD(clReleaseCommandQueue) GPU_LOAD(clReleaseContext)
#undef GPU_LOAD
        cl_uint count=0;check(clGetPlatformIDs(0,nullptr,&count));std::vector<cl_platform_id> platforms(count);check(clGetPlatformIDs(count,platforms.data(),nullptr));
        cl_ulong best=0;
        for(auto p:platforms){cl_uint n=0;if(clGetDeviceIDs(p,CL_DEVICE_TYPE_GPU,0,nullptr,&n)!=CL_SUCCESS)continue;std::vector<cl_device_id> devices(n);check(clGetDeviceIDs(p,CL_DEVICE_TYPE_GPU,n,devices.data(),nullptr));
            for(auto d:devices){cl_device_fp_config fp=0;cl_bool available=0;cl_uint units=0;cl_ulong memory=0;
                clGetDeviceInfo(d,CL_DEVICE_DOUBLE_FP_CONFIG,sizeof(fp),&fp,nullptr);clGetDeviceInfo(d,CL_DEVICE_AVAILABLE,sizeof(available),&available,nullptr);
                clGetDeviceInfo(d,CL_DEVICE_MAX_COMPUTE_UNITS,sizeof(units),&units,nullptr);clGetDeviceInfo(d,CL_DEVICE_GLOBAL_MEM_SIZE,sizeof(memory),&memory,nullptr);
                cl_ulong rank=cl_ulong(units)*1000000000ULL+memory;if(available&&fp&&rank>best){best=rank;device=d;}
            }
        }
        if(!device)throw std::runtime_error("No available GPU with double precision");
        size_t size=0;check(clGetDeviceInfo(device,CL_DEVICE_NAME,0,nullptr,&size));name.resize(size);check(clGetDeviceInfo(device,CL_DEVICE_NAME,size,name.data(),nullptr));while(!name.empty()&&name.back()==0)name.pop_back();
        cl_int error=0;context=clCreateContext(nullptr,1,&device,nullptr,nullptr,&error);check(error);queue=clCreateCommandQueue(context,device,0,&error);check(error);
        program=clCreateProgramWithSource(context,1,&source,nullptr,&error);check(error);error=clBuildProgram(program,1,&device,"",nullptr,nullptr);
        if(error!=CL_SUCCESS){size_t n=0;clGetProgramBuildInfo(program,device,CL_PROGRAM_BUILD_LOG,0,nullptr,&n);std::string log(n,0);clGetProgramBuildInfo(program,device,CL_PROGRAM_BUILD_LOG,n,log.data(),nullptr);throw std::runtime_error("GPU kernel build failed: "+log);}
        kernel=clCreateKernel(program,"nearest",&error);check(error);ready=true;
    }catch(const std::exception& e){reason=e.what();}}
    ~Runtime(){if(kernel)clReleaseKernel(kernel);if(program)clReleaseProgram(program);if(queue)clReleaseCommandQueue(queue);if(context)clReleaseContext(context);if(dll)FreeLibrary(dll);}
};
inline Runtime& runtime(){static Runtime r;return r;}
struct TreeBuffers {
    cl_mem nodes=nullptr,queries=nullptr,distances=nullptr,points=nullptr;size_t capacity=0;
    ~TreeBuffers(){auto& r=runtime();for(auto m:{nodes,queries,distances,points})if(m)r.clReleaseMemObject(m);}
};
template<class Node> inline bool nearestGPU(const std::vector<Node>& nodes,std::shared_ptr<TreeBuffers>& buffers,const std::vector<Point>& query,std::vector<std::pair<double,Point>>& out){
    if(selected.load()!=Backend::GPU||nodes.empty()||query.empty())return false;
    static_assert(sizeof(Node)==24&&sizeof(Point)==12,"GPU ABI mismatch");auto& r=runtime();std::lock_guard<std::mutex> guard(r.lock);
    if(!r.ready){++cpuFallbacks;return false;}
    try{
        cl_int error=0;
        if(!buffers){buffers=std::make_shared<TreeBuffers>();buffers->nodes=r.clCreateBuffer(r.context,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,nodes.size()*sizeof(Node),(void*)nodes.data(),&error);r.check(error);}
        if(query.size()>buffers->capacity){for(auto m:{buffers->queries,buffers->distances,buffers->points})if(m)r.clReleaseMemObject(m);buffers->queries=buffers->distances=buffers->points=nullptr;buffers->capacity=0;
            buffers->queries=r.clCreateBuffer(r.context,CL_MEM_READ_ONLY,query.size()*sizeof(Point),nullptr,&error);r.check(error);
            buffers->distances=r.clCreateBuffer(r.context,CL_MEM_WRITE_ONLY,query.size()*sizeof(double),nullptr,&error);r.check(error);
            buffers->points=r.clCreateBuffer(r.context,CL_MEM_WRITE_ONLY,query.size()*sizeof(Point),nullptr,&error);r.check(error);buffers->capacity=query.size();
        }
        r.check(r.clEnqueueWriteBuffer(r.queue,buffers->queries,CL_TRUE,0,query.size()*sizeof(Point),query.data(),0,nullptr,nullptr));
        cl_mem arguments[]={buffers->nodes,buffers->queries,buffers->distances,buffers->points};for(cl_uint i=0;i<4;i++)r.check(r.clSetKernelArg(r.kernel,i,sizeof(cl_mem),&arguments[i]));int count=int(query.size());r.check(r.clSetKernelArg(r.kernel,4,sizeof(count),&count));size_t global=query.size();r.check(r.clEnqueueNDRangeKernel(r.queue,r.kernel,1,nullptr,&global,nullptr,0,nullptr,nullptr));
        std::vector<double> distances(query.size());std::vector<Point> points(query.size());r.check(r.clEnqueueReadBuffer(r.queue,buffers->distances,CL_TRUE,0,distances.size()*sizeof(double),distances.data(),0,nullptr,nullptr));r.check(r.clEnqueueReadBuffer(r.queue,buffers->points,CL_TRUE,0,points.size()*sizeof(Point),points.data(),0,nullptr,nullptr));
        out.resize(query.size());for(size_t i=0;i<query.size();i++)out[i]={distances[i],points[i]};++gpuBatches;return true;
    }catch(const std::exception& e){r.ready=false;r.reason=e.what();buffers.reset();++cpuFallbacks;return false;}
}
}
