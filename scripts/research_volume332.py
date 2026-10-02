"""Offline exploratory model tests. No production header is written here."""
from pathlib import Path
import csv,json,sys
r=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(r/'.tools/science327'))
import numpy as np
p=r/'build/control332'
def load(path): return {(x['dataset'],int(x['id'])):x for x in csv.DictReader(path.open(encoding='utf-8'))}
raw=load(r/'build/perf331/audit/optimized/raw.csv');raw.update(load(p/'before-new/raw.csv'))
features=load(r/'build/perf331/audit/optimized/features.csv');features.update(load(p/'before-new/features.csv'))
inv=json.loads((p/'inventory.json').read_text(encoding='utf-8'))
rows=[]
for item in inv:
    key=(item['dataset'],item['id']);row=raw.get(key);controls=item['controls']
    if not row or row['error'] or len(controls)!=1 or controls[0][3]!=5 or controls[0][2]<=0 or controls[0][1].strip().upper()!=row['label'].strip().upper(): continue
    rows.append(dict(dataset=key[0],id=key[1],truck=row['label'],target=controls[0][2],volume=float(row['geometric_m3']),previous=float(row['calculated_m3']),features=[float(features[key]['f'+str(i)]) for i in range(190)]))
x=np.array([a['features'] for a in rows]);v=np.array([a['volume'] for a in rows]);t=np.array([a['target'] for a in rows]);prior=np.array([a['previous'] for a in rows]);y=t/v-1
new=np.array([a['dataset']=='n-gk new' for a in rows]);eligible=(v>0)&(np.abs(y)<=.15)
def fit(indices,width,alpha):
    xx=x[indices];mean=xx.mean(0);scale=np.maximum(xx.std(0),.005);z=(xx-mean)/scale
    d=np.maximum(0,(z*z).sum(1)[:,None]+(z*z).sum(1)[None,:]-2*z@z.T)/x.shape[1]
    k=np.exp(-d/(2*width*width));bias=float(np.median(y[indices]));weights=np.linalg.solve(k+alpha*np.eye(len(z)),y[indices]-bias)
    return dict(mean=mean,scale=scale,z=z,weights=weights,bias=bias,width=width,minimum=xx.min(0),maximum=xx.max(0),vmin=v[indices].min()*.8,vmax=v[indices].max()*1.2)
def predict(model,xx,vv):
    a=(xx-model['mean'])/model['scale'];z=model['z'];d=np.maximum(0,(a*a).sum(1)[:,None]+(z*z).sum(1)[None,:]-2*a@z.T)/x.shape[1]
    correction=model['bias']+np.exp(-d/(2*model['width']**2))@model['weights'];margin=np.maximum(.005,(model['maximum']-model['minimum'])*.25)
    guard=np.all((xx>=model['minimum']-margin)&(xx<=model['maximum']+margin),axis=1)&(vv>=model['vmin'])&(vv<=model['vmax'])&(np.abs(correction)<=.15)
    return vv*(1+np.where(guard,correction,0)),guard
def metric(pred,mask): return float(np.mean(np.abs(pred[mask]/t[mask]-1))*100)
records=[]
# New data are untouched by this training; compare the same fixed hyperparameters.
model=fit((~new)&eligible,1.15,.07);pred,guard=predict(model,x,v)
print('Old-only expanded training: new holdout',metric(pred,new),'previous',metric(prior,new),'old',metric(pred,~new),flush=True)
records.append(dict(test='old-only independent new-dataset holdout',new=metric(pred,new),old=metric(pred,~new),previous_new=metric(prior,new)))
# Leave whole vehicles out, including both new P099 observations together.
labels=sorted(set(a['truck'] for a in rows));fold=np.array([labels.index(a['truck'])%6 for a in rows])
for width,alpha in [(1.15,.07),(1.15,.15),(.8,.07),(1.5,.07)]:
    cv=np.zeros(len(rows));guards=np.zeros(len(rows),bool)
    for k in range(6):
        mask=fold==k;model=fit((~mask)&eligible,width,alpha);cv[mask],guards[mask]=predict(model,x[mask],v[mask])
    model=fit(eligible,width,alpha);pred,guard=predict(model,x,v)
    record=dict(test='exploratory whole-vehicle folds',width=width,alpha=alpha,cv_all=metric(cv,np.ones(len(rows),bool)),cv_new=metric(cv,new),cv_old=metric(cv,~new),training_new=metric(pred,new),training_old=metric(pred,~new),training_max=float(np.max(np.abs(pred/t-1))*100),previous_new=metric(prior,new))
    records.append(record);print(record,flush=True)
    if width==1.15 and alpha==.07:
        np.savez(p/'model-data.npz',x=x,v=v,t=t,y=y,new=new,eligible=eligible,fold=fold,previous=prior,cv=cv,fit=pred)
        (p/'model-rows.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
(p/'model-research.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
