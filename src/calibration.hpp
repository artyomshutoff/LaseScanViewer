#pragma once
#include "calibration_features.hpp"
#include "kernel_estimate.hpp"
#include <functional>

// An explicitly separate supervised estimate. It never changes the geometric
// integral, registration, mesh, point classification or exported PLY geometry.
// No file names, measurement IDs, truck labels or database access are involved.
inline void calibrateVolume(const Model& active,const Model& base,const Comparison& c,
                        Cargo& cargo,bool manualRegion,const std::function<void(int)>& progress={}){
    cargo.calibrated=false;cargo.calibratedVolume=0;
    auto reject=[&](const char* reason){cargo.calibrationNote=reason;};
    if(!c.options.calibratedEstimate){reject("Калибровка отключена");return;}
    if(c.refinementRejected){reject("Сетка неустойчива: показан геометрический объём без модельной поправки");return;}
    if(manualRegion||active.kind!=2||base.kind!=1||active.selectedType!=0||base.selectedType!=0||
       c.options.estimator!=4||!c.options.reconstructGaps||!c.adaptiveGridUsed||
       std::abs(c.requestedStep-100)>1e-8||std::abs(c.options.metresPerUnit-.001)>1e-10||
       cargo.threshold!=50||!cargo.largest||!cargo.cleaned){
        reject("Уточнение по контрольным данным неприменимо к выбранному формату, области или настройкам");return;
    }
    double volume=cargo.volume/1e9;
    auto features=volumeFeatures(active,base,c,cargo,progress);
    static_assert(features.size()==volumeKernelModel::featureCount,"Feature/model mismatch");
    double correction=0;
    if(!kernelCorrection(features,volume,correction)){reject("Геометрия, объём или поправка вне диапазона контрольных данных");return;}
    cargo.calibratedVolume=cargo.volume*(1+correction);cargo.calibrated=true;
    cargo.calibrationNote="Нелинейная оценка по 190 геометрическим признакам. Все 271 контрольная пара использованы при обучении: среднее отклонение 0.555%. При исключении целых автомобилей — 1.184%; проверка использована для выбора модели. Независимого испытания нет. Это оценка, а не гарантированная погрешность. Геометрия и PLY сохраняют исходный масштаб.";
}
