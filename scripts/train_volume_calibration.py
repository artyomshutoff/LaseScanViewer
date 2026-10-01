"""Reproducible ridge calibration on geometric anchors, never on IDs at inference.

Inputs: 89 n-gk and 10 dmu aligned pairs. TotalVolume is read only for training.
Validation holds out n-gk vehicles in five folds and ALL dmu as a sixth fold.
All these data were used in development; this is not a blind accuracy claim.
"""
from pathlib import Path
import csv, hashlib, json, sqlite3
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
RIDGE = 30.0
KEYS = [(str(d), str(c), str(q)) for d in (60,100,160,250)
        for c in (.8,.9,.95,.98) for q in (.5,.9,1)]

def fit(x, y):
    mean = x.mean(0)
    scale = np.maximum(x.std(0), .001)
    z = np.column_stack([np.ones(len(x)), (x-mean)/scale])
    penalty = np.eye(z.shape[1])*RIDGE
    penalty[0,0] = 0
    w = np.linalg.solve(z.T@z+penalty, z.T@y)
    coef = w[1:]/scale
    return float(w[0]-mean@coef), coef

def main():
    samples, features = [], []
    for dataset, raw_file, feature_file, count in [
        ('n-gk','build/ngk-full-341-retest.csv','build/research35/rim.csv',89),
        ('dmu','build/dmu-full-351.csv','build/research36/dmu-rim.csv',10)]:
        db_path = ROOT/dataset/'TVM_Measurement_Data_Base.sq3'
        db = sqlite3.connect(db_path.as_uri()+'?mode=ro', uri=True)
        observed = {(int(z['id']),z['divisor'],z['cut'],z['quantile']):z
                    for z in csv.DictReader((ROOT/feature_file).open())}
        raw_rows = sorted(csv.DictReader((ROOT/raw_file).open(encoding='utf-8')),
                          key=lambda z:int(z['id']))
        assert len(raw_rows)==count
        for z in raw_rows:
            ident = int(z['id'])
            truck, target, status = db.execute(
                'SELECT TruckID,TotalVolume,Status FROM LaseTVM WHERE MeasurementID=?',
                (ident,)).fetchone()
            assert target>0 and status==5 and not z.get('error','')
            volume = float(z['volume'])
            variants = [observed[(ident,*key)] for key in KEYS]
            # Same feature order and units as C++: 48 relative volumes,
            # 48 median absolute residuals in metres, 48 log anchor counts.
            features.append([float(v['volume'])/volume-1 for v in variants]
                + [float(v['mad'])/1000 for v in variants]
                + [float(np.log1p(float(v['count']))) for v in variants])
            fold = int(hashlib.sha256(truck.encode()).hexdigest(),16)%5 if dataset=='n-gk' else 5
            samples.append(dict(dataset=dataset,id=ident,truck=truck,fold=fold,
                                target_m3=target,geometric_m3=volume))
        db.close()
    x = np.array(features)
    target = np.array([s['target_m3'] for s in samples])
    volume = np.array([s['geometric_m3'] for s in samples])
    folds = np.array([s['fold'] for s in samples])
    y = target/volume-1
    intercept, coef = fit(x,y)
    prediction = volume*(1+intercept+x@coef)
    cv = np.zeros(len(samples))
    for fold in range(6):
        train = folds!=fold
        b,w = fit(x[train],y[train])
        cv[~train] = volume[~train]*(1+b+x[~train]@w)
    metrics = dict(pairs=len(samples),features=x.shape[1],ridge=RIDGE,
                   validation='5 vehicle folds in n-gk plus all dmu held out together; exploratory, not blind')
    for label, mask in [('all',np.ones(len(samples),dtype=bool)),
                        ('n-gk',folds!=5),('dmu',folds==5)]:
        metrics[label] = dict(pairs=int(mask.sum()),
            training_mape=float(np.mean(abs(prediction[mask]/target[mask]-1))*100),
            training_max_percent=float(np.max(abs(prediction[mask]/target[mask]-1))*100),
            group_cv_mape=float(np.mean(abs(cv[mask]/target[mask]-1))*100),
            group_cv_max_percent=float(np.max(abs(cv[mask]/target[mask]-1))*100))
    assert all(metrics[k]['training_mape']<1 for k in ['all','n-gk','dmu'])
    model = dict(intercept=intercept,coefficients=coef.tolist(),
                 feature_min=x.min(0).tolist(),feature_max=x.max(0).tolist(),
                 volume_min=float(volume.min()*.8),volume_max=float(volume.max()*1.2),metrics=metrics)
    (ROOT/'build/volume-calibration-model.json').write_text(json.dumps(model,indent=2),encoding='utf-8')
    for j,s in enumerate(samples):
        s['training_prediction_m3']=float(prediction[j]);s['group_cv_prediction_m3']=float(cv[j])
    with (ROOT/'build/volume-calibration-validation.csv').open('w',newline='',encoding='utf-8') as f:
        writer=csv.DictWriter(f,fieldnames=list(samples[0]));writer.writeheader();writer.writerows(samples)
    with (ROOT/'build/volume-calibration-features.csv').open('w',newline='',encoding='utf-8') as f:
        writer=csv.writer(f);writer.writerow(['dataset','id']+[f'feature_{i}' for i in range(x.shape[1])])
        for s,z in zip(samples,x):writer.writerow([s['dataset'],s['id'],*z])
    def array(name,values):
        return 'inline constexpr double '+name+'[featureCount]={'+','.join(format(float(z),'.17g') for z in values)+'};\n'
    header='// Generated by scripts/train_volume_calibration.py; geometry only, no IDs.\n#pragma once\nnamespace volumeCalibrationModel {\n'
    header+=f'inline constexpr int featureCount={x.shape[1]};\n'
    header+=f'inline constexpr double intercept={intercept:.17g},volumeMin={model["volume_min"]:.17g},volumeMax={model["volume_max"]:.17g};\n'
    header+=array('coefficients',coef)+array('featureMin',x.min(0))+array('featureMax',x.max(0))+'}\n'
    (ROOT/'src/calibration_model.hpp').write_text(header,encoding='utf-8')
    print(json.dumps(metrics,indent=2))

if __name__=='__main__':main()
