"""Compare frozen, independent C++ runs; controls never enter the executables."""
from pathlib import Path
import csv,json,statistics
r=Path(__file__).resolve().parents[1];p=r/'build/control327'
inv=json.loads((p/'inventory.json').read_text(encoding='utf-8'))
baseline={};candidate={}
for tag in ['new','old']:
 for target,folder in [(baseline,tag),(candidate,'candidate-'+tag)]:
  f=p/folder/'raw.csv'
  if f.exists():target.update({(x['dataset'],int(x['id'])):x for x in csv.DictReader(f.open(encoding='utf-8'))})
rows=[];changes=[];excluded=[]
for entry in inv:
 key=(entry['dataset'],entry['id']);old=baseline.get(key);new=candidate.get(key);control=entry['controls']
 if old is None or new is None:continue
 if old['error'] or new['error']:excluded.append(dict(dataset=key[0],id=key[1],reason=old['error'] or new['error']));continue
 changed=any(old[x]!=new[x] for x in ['angle','dx','dy','dz','geometric_m3','calculated_m3'])
 if changed:changes.append(dict(dataset=key[0],id=key[1],before=float(old['calculated_m3']),after=float(new['calculated_m3']),old_overlap=float(old['overlap']),new_overlap=float(new['overlap']),old_rms=float(old['rms']),new_rms=float(new['rms']),structure=new['structure_refined']))
 if len(control)!=1 or control[0][3]!=5 or not isinstance(control[0][2],(int,float)) or control[0][2]<=0 or str(control[0][1]).strip().upper()!=new['label'].strip().upper():excluded.append(dict(dataset=key[0],id=key[1],reason='No unique completed matching TotalVolume'));continue
 t=control[0][2];a=float(old['calculated_m3']);b=float(new['calculated_m3']);g=float(new['geometric_m3'])
 rows.append(dict(dataset=key[0],id=key[1],truck=new['label'],total_volume_m3=t,before_m3=a,after_m3=b,difference_m3=b-t,before_percent=abs(a/t-1)*100,after_percent=abs(b/t-1)*100,geometric_m3=g,geometric_percent=abs(g/t-1)*100,changed=changed,overlap=float(new['overlap']),rms=float(new['rms']),accepted=int(new['accepted']),calibrated=int(new['calibrated'])))
def metrics(a):
 return dict(count=len(a),before_mape=statistics.mean(x['before_percent'] for x in a),after_mape=statistics.mean(x['after_percent'] for x in a),before_mae_m3=statistics.mean(abs(x['before_m3']-x['total_volume_m3']) for x in a),after_mae_m3=statistics.mean(abs(x['difference_m3']) for x in a),before_max=max(x['before_percent'] for x in a),after_max=max(x['after_percent'] for x in a),geometric_mape=statistics.mean(x['geometric_percent'] for x in a),changed=sum(x['changed'] for x in a),improved=sum(x['after_percent']<x['before_percent']-1e-8 for x in a),worsened=sum(x['after_percent']>x['before_percent']+1e-8 for x in a),weak=sum(not x['accepted'] for x in a)) if a else dict(count=0)
summary={ds:metrics([x for x in rows if x['dataset']==ds]) for ds in ['n-gk 2','n-gk','dmu','тест']};summary['all']=metrics(rows)
for name,data in [('results',rows),('changes',changes),('excluded',excluded)]:
 if data:
  with (p/('candidate-'+name+'.csv')).open('w',newline='',encoding='utf-8-sig') as f:w=csv.DictWriter(f,fieldnames=data[0].keys());w.writeheader();w.writerows(data)
(p/'candidate-summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(summary,ensure_ascii=False,indent=2))
for x in rows:
 if x['changed']:print(x['dataset'],x['id'],round(x['before_percent'],4),round(x['after_percent'],4),round(x['overlap'],3))
print('excluded',excluded)
