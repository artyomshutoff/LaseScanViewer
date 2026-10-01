"""3.8: weighted bed anchors, robust regularized fit on expanded development data.
All development uses known data. Group CV is exploratory, not a blind test.
"""
from pathlib import Path
import csv,hashlib,json,sqlite3
import numpy as np
ROOT=Path(__file__).resolve().parents[1]
FEATURES=180

def fit(x,y):
    mean=x.mean(0);scale=np.maximum(x.std(0),.001)
    z=np.column_stack([np.ones(len(x)),(x-mean)/scale])
    # Local descriptors are strongly correlated with global anchors: shrink
    # their coefficients more, rather than allowing them to dominate a fit.
    penalty=np.diag([0]+[30.]*144+[300.]*36)
    weights=np.ones(len(y))
    for iteration in range(10):
        w=np.linalg.solve(z.T@(weights[:,None]*z)+penalty,z.T@(weights*y))
        weights=np.minimum(1.,.02/np.maximum(np.abs(y-z@w),1e-12))
    coef=w[1:]/scale
    return float(w[0]-mean@coef),coef

def applicable(x,volume,correction,train,train_volume):
    lo=train.min(0);hi=train.max(0);margin=np.maximum(.005,(hi-lo)*.25)
    return np.all((x>=lo-margin)&(x<=hi+margin),axis=1)&(abs(correction)<=.08)&(volume>=train_volume.min()*.8)&(volume<=train_volume.max()*1.2)

def main():
    source=sorted(csv.DictReader((ROOT/'build/research38/weighted-features.csv').open()),key=lambda z:(z['dataset'],int(z['id'])))
    inventory=list(csv.DictReader((ROOT/'build/research38/inventory.csv').open()))
    dev={(z['dataset'],int(z['id'])) for z in inventory if z['split']=='development'}
    held={(z['dataset'],int(z['id'])) for z in inventory if z['split']=='holdout'}
    assert len(source)==173 and all((z['dataset'],int(z['id'])) in dev for z in source)
    assert not held & {(z['dataset'],int(z['id'])) for z in source}
    rows=[];features=[]
    for ds,count in [('n-gk',163),('dmu',10)]:
        subset=[z for z in source if z['dataset']==ds];assert len(subset)==count
        db=sqlite3.connect((ROOT/ds/'TVM_Measurement_Data_Base.sq3').as_uri()+'?mode=ro',uri=True)
        for z in subset:
            ident=int(z['id']);truck,target,status=db.execute('SELECT TruckID,TotalVolume,Status FROM LaseTVM WHERE MeasurementID=?',(ident,)).fetchone()
            assert target>0 and status==5
            fold=int(hashlib.sha256(truck.encode()).hexdigest(),16)%5 if ds=='n-gk' else 5
            rows.append(dict(dataset=ds,id=ident,truck=truck,fold=fold,target_m3=target,geometric_m3=float(z['volume'])))
            features.append([float(z[f'feature_{i}']) for i in range(FEATURES)])
        db.close()
    x=np.array(features);volume=np.array([z['geometric_m3'] for z in rows]);target=np.array([z['target_m3'] for z in rows]);y=target/volume-1
    assert np.isfinite(x).all()
    folds=np.array([z['fold'] for z in rows]);b,w=fit(x,y);correction=b+x@w;prediction=volume*(1+correction)
    training_applied=applicable(x,volume,correction,x,volume)
    prediction=np.where(training_applied,prediction,volume)
    cv=np.zeros(len(rows));guarded=np.zeros(len(rows));accepted=np.zeros(len(rows),dtype=bool)
    for fold in range(6):
        train=folds!=fold;bias,coef=fit(x[train],y[train]);correction=bias+x[~train]@coef
        cv[~train]=volume[~train]*(1+correction)
        ok=applicable(x[~train],volume[~train],correction,x[train],volume[train])
        guarded[~train]=np.where(ok,cv[~train],volume[~train]);accepted[~train]=ok
    metrics=dict(pairs=173,features=FEATURES,ridge_global=30,ridge_local=300,huber_delta=.02,iterations=10,
        validation='5 n-gk vehicle folds plus entire dmu as fold 6; exploratory, not blind')
    for name,mask in [('all',np.ones(len(rows),dtype=bool)),('n-gk',folds!=5),('dmu',folds==5)]:
        metrics[name]=dict(pairs=int(mask.sum()),training_mape=float(np.mean(abs(prediction[mask]/target[mask]-1))*100),
            training_max_percent=float(np.max(abs(prediction[mask]/target[mask]-1))*100),
            geometric_mape=float(np.mean(abs(volume[mask]/target[mask]-1))*100),
            group_cv_mape=float(np.mean(abs(cv[mask]/target[mask]-1))*100),
            guarded_cv_mape=float(np.mean(abs(guarded[mask]/target[mask]-1))*100),
            guarded_cv_applied=int(accepted[mask].sum()))
    for i,z in enumerate(rows):
        z.update(training_prediction_m3=float(prediction[i]),group_cv_prediction_m3=float(cv[i]),
            guarded_cv_prediction_m3=float(guarded[i]),guarded_cv_applied=bool(accepted[i]),training_applied=bool(training_applied[i]))
    model=dict(intercept=b,coefficients=w.tolist(),feature_min=x.min(0).tolist(),feature_max=x.max(0).tolist(),
        volume_min=float(volume.min()*.8),volume_max=float(volume.max()*1.2),metrics=metrics)
    (ROOT/'build/volume38-model.json').write_text(json.dumps(model,indent=2),encoding='utf-8')
    with (ROOT/'build/volume38-validation.csv').open('w',newline='',encoding='utf-8') as f:
        writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    with (ROOT/'build/volume38-features.csv').open('w',newline='',encoding='utf-8') as f:
        writer=csv.writer(f);writer.writerow(['dataset','id']+[f'feature_{i}' for i in range(FEATURES)])
        for row,values in zip(rows,x):writer.writerow([row['dataset'],row['id'],*values])
    def array(name,values):return 'inline constexpr double '+name+'[featureCount]={'+','.join(format(float(z),'.17g') for z in values)+'};\n'
    header='// Generated by scripts/train_volume_calibration38.py; geometric descriptors only.\n#pragma once\nnamespace volumeCalibrationModel {\n'
    header+=f'inline constexpr int featureCount={FEATURES};\n'
    header+=f'inline constexpr double intercept={b:.17g},volumeMin={model["volume_min"]:.17g},volumeMax={model["volume_max"]:.17g};\n'
    header+=array('coefficients',w)+array('featureMin',x.min(0))+array('featureMax',x.max(0))+'}\n'
    (ROOT/'src/calibration_model.hpp').write_text(header,encoding='utf-8')
    print(json.dumps(metrics,indent=2))
if __name__=='__main__':main()
