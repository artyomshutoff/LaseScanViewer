#pragma once
#include "calibration_kernel_model.hpp"
#include <array>
#include <cmath>
#include <algorithm>
#include "simd_distance.hpp"
// Smooth regularized regression in normalized geometric descriptor space.
// Centres contain descriptors, not scan identities or reference volumes.
inline bool kernelCorrection(const std::array<double,190>& features,double volume,double& correction){
    using namespace volumeKernelModel;
    correction=0;
    if(!std::isfinite(volume)||volume<volumeMin||volume>volumeMax)return false;
    std::array<double,190> normalized{};
    for(size_t i=0;i<features.size();++i){
        double margin=std::max(.005,(featureMax[i]-featureMin[i])*.25);
        if(!std::isfinite(features[i])||features[i]<featureMin[i]-margin||features[i]>featureMax[i]+margin)return false;
        normalized[i]=(features[i]-mean[i])/scale[i];
    }
    double result=bias;
    for(int row=0;row<centreCount;++row){
        double distance=descriptorDistance190(normalized.data(),centres+row*featureCount);
        result+=weights[row]*std::exp(-distance/(2*width*width*featureCount));
    }
    if(!std::isfinite(result)||std::abs(result)>.15)return false;
    correction=result;return true;
}
