from pathlib import Path
import csv,json,subprocess,statistics,argparse
r=Path(__file__).resolve().parents[1];out=r/'build/perf331'
args=argparse.ArgumentParser();args.add_argument('--runs',type=int,default=1);args.add_argument('--quick',action='store_true');a=args.parse_args()
cases=[('ngk14301','n-gk',14301),('ngk13785','n-gk',13785),('dmu8','dmu',8),('test43279','тест',43279),('ngk2-12942','n-gk 2',12942),('ngk2-12837','n-gk 2',12837)]
if a.quick:cases=cases[:4]
results=[]
for case,ds,id in cases:
 full=r/ds/f'{id:010}_Full.bin';empty=r/ds/f'{id:010}_Empty.bin';rows={}
 for repeat in range(a.runs):
  for mode in (['baseline','optimized'] if repeat%2==0 else ['optimized','baseline']):
   dest=out/f'{mode}-{case}-{repeat}.csv'
   p=subprocess.run([str(out/(mode+'.exe')),str(full),str(empty),str(dest)],capture_output=True,text=True,timeout=180)
   if p.returncode:raise RuntimeError((case,mode,p.stderr))
   row=next(csv.DictReader(dest.open()));rows.setdefault(mode,[]).append(row)
   print(case,mode,repeat,round(float(row['total']),3), 's',flush=True)
 ref=rows['baseline'][0]
 for row in rows['baseline']+rows['optimized']:
  for key in ref:
   if key not in ['load','align','volume','total']:assert row[key]==ref[key],(case,key,ref[key],row[key])
 result=dict(case=case,runs=a.runs,exact_outputs=True)
 for phase in ['load','align','volume','total']:
  before=statistics.median(float(x[phase]) for x in rows['baseline']);after=statistics.median(float(x[phase]) for x in rows['optimized'])
  result[phase]=dict(before=before,after=after,reduction_percent=100*(1-after/before))
 results.append(result);(out/'timing-summary.json').write_text(json.dumps(results,indent=2))
 print('EXACT alignment, volumes, sensitivities, cells and display geometry',case,flush=True)
print(json.dumps(results,indent=2),flush=True)
