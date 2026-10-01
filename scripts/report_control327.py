from pathlib import Path
import csv,json,math,statistics
r=Path(__file__).resolve().parents[1];p=r/'build/control327';inv=json.loads((p/'inventory.json').read_text(encoding='utf-8'))
lookup={}
for folder in ['new','old']:
 f=p/folder/'raw.csv'
 if f.exists():lookup.update({(x['dataset'],int(x['id'])):x for x in csv.DictReader(f.open(encoding='utf-8'))})
results=[]
for x in inv:
 row=lookup.get((x['dataset'],x['id']));controls=x['controls'];z=dict(dataset=x['dataset'],id=x['id'],calculated_m3='',geometric_m3='',total_volume_m3='',difference_m3='',absolute_percent='',geometric_percent='',label='',status='',accepted='',overlap='',rms='',calibrated='',seconds='',reason='')
 if len(controls)==1:z.update(total_volume_m3=controls[0][2],status=controls[0][3])
 if not x['empty']:z['reason']='Missing Empty'
 elif row is None:z['reason']='Not calculated yet'
 elif row['error']:z['reason']='Calculation error: '+row['error']
 else:
  v=float(row['calculated_m3']);g=float(row['geometric_m3']);z.update(calculated_m3=v,geometric_m3=g,label=row['label'],accepted=row['accepted'],overlap=float(row['overlap']),rms=float(row['rms']),calibrated=row['calibrated'],seconds=float(row['seconds']))
  if len(controls)!=1:z['reason']='No unique reference record'
  elif str(controls[0][1]).strip().upper()!=row['label'].strip().upper():z['reason']='Truck label mismatch'
  elif not isinstance(z['total_volume_m3'],(int,float)) or not math.isfinite(z['total_volume_m3']) or z['total_volume_m3']<=0:z['reason']='Invalid/nonpositive TotalVolume'
  elif z['status']!=5:z['reason']='Reference record not complete'
  else:
   t=z['total_volume_m3'];z.update(difference_m3=v-t,absolute_percent=abs(v/t-1)*100,geometric_percent=abs(g/t-1)*100)
 results.append(z)
def metrics(rows):
 a=[x for x in rows if x['absolute_percent']!=''];return dict(compared=len(a),calculated=sum(x['calculated_m3']!='' for x in rows),mape=statistics.mean(x['absolute_percent'] for x in a) if a else None,mae_m3=statistics.mean(abs(x['difference_m3']) for x in a) if a else None,max_percent=max((x['absolute_percent'] for x in a),default=None),geometric_mape=statistics.mean(x['geometric_percent'] for x in a) if a else None,weak=sum(x['accepted']=='0' for x in a),model_used=sum(x['calibrated']=='1' for x in a),worst=sorted(a,key=lambda x:x['absolute_percent'],reverse=True)[:10])
summary={ds:metrics([x for x in results if x['dataset']==ds]) for ds in ['n-gk 2','n-gk','dmu','тест']};summary['all']=metrics(results)
with (p/'baseline-results.csv').open('w',newline='',encoding='utf-8-sig') as f:w=csv.DictWriter(f,fieldnames=results[0].keys());w.writeheader();w.writerows(results)
(p/'baseline-summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({ds:{k:v for k,v in m.items() if k!='worst'} for ds,m in summary.items()},ensure_ascii=False,indent=2))
for x in summary['n-gk 2']['worst'][:5]:print(x['id'],x['absolute_percent'],x['calibrated'],x['calculated_m3'],x['total_volume_m3'])
