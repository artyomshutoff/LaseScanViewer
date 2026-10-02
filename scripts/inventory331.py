"""Freeze current input hashes and independent SQ3 controls; never feed controls to C++."""
from pathlib import Path
import json,hashlib,sqlite3,csv,re,zipfile
r=Path(__file__).resolve().parents[1];p=r/'build/perf331';audit=p/'audit';audit.mkdir(parents=True,exist_ok=True)
previous=r/'build/control327';saved={}
baseline={}
# All transitive numerical headers equal those used by the stored 3.27 run.
source=p/'baseline/src';visited=set()
def visit(name):
 if name in visited:return
 visited.add(name)
 for inc in re.findall(r'^\s*#include\s+"([^"]+)"',(source/name).read_text(encoding='utf-8'),re.M):
  relative=str((Path(name).parent/inc).as_posix());visit(relative)
for name in ['registration.hpp','volume.hpp']:visit(name)
historical=r/'releases/LaseScanViewer-3.27.0/LaseScanViewer-source.zip'
if historical.exists() and (previous/'input-hashes.json').exists():
 with zipfile.ZipFile(historical) as z:
  for name in visited:assert (source/name).read_bytes()==z.read('src/'+name),name
 saved=json.loads((previous/'input-hashes.json').read_text(encoding='utf-8'))
 for folder in ['candidate-old','candidate-new']:
  path=previous/folder/'raw.csv'
  if path.exists():
   for row in csv.DictReader(path.open(encoding='utf-8')):baseline[(row['dataset'],int(row['id']))]=row
entries=[];hashes={};needBaseline=[];cached=[]
for ds in ['n-gk','dmu','тест','n-gk 2']:
 folder=r/ds;db=folder/'TVM_Measurement_Data_Base.sq3'
 if not db.exists() and ds=='n-gk 2':db=r/'n-gk/TVM_Measurement_Data_Base.sq3'
 connection=sqlite3.connect(db.as_uri()+'?mode=ro',uri=True) if db.exists() else None
 for full in sorted(folder.glob('*_Full.bin')):
  id=int(full.name[:10]);empty=full.with_name(full.name.replace('_Full','_Empty'))
  if not empty.exists():continue
  unchanged=True
  for f in [full,empty]:
   name=str(f.relative_to(r));digest=hashlib.sha256(f.read_bytes()).hexdigest();hashes[name]=digest
   unchanged&=saved.get(name,{}).get('sha256')==digest
  controls=[]
  if connection:
   controls=connection.execute('select MeasurementID,TruckID,TotalVolume,Status from LaseTVM where MeasurementID=?',(id,)).fetchall()
   if not controls:controls=connection.execute('select MeasurementID,TruckID,TotalVolume,Status from LaseTVM where IncomingMeasurementID=? or OutgoingMeasurementID=?',(id,id)).fetchall()
  entries.append(dict(dataset=ds,id=id,full=str(full.relative_to(r)),controls=controls,baseline_cached=bool(unchanged and (ds,id) in baseline)))
  if unchanged and (ds,id) in baseline:cached.append(baseline[(ds,id)])
  else:needBaseline.append(ds+'\t'+str(full))
 if connection:connection.close()
 if db.exists():hashes[str(db.relative_to(r))]=hashlib.sha256(db.read_bytes()).hexdigest()
(audit/'inventory.json').write_text(json.dumps(entries,ensure_ascii=False,indent=2),encoding='utf-8')
(audit/'input-hashes.json').write_text(json.dumps(hashes,ensure_ascii=False,indent=2),encoding='utf-8')
(p/'all.tsv').write_text(''.join(x['dataset']+'\t'+str(r/x['full'])+'\n' for x in entries),encoding='utf-8')
(p/'baseline-needed.tsv').write_text('\n'.join(needBaseline)+ ('\n' if needBaseline else ''),encoding='utf-8')
with (audit/'baseline-cached.csv').open('w',newline='',encoding='utf-8') as f:
 if cached:
  w=csv.DictWriter(f,fieldnames=cached[0].keys());w.writeheader();w.writerows(cached)
print(f'Frozen {len(entries)} pairs, {len(cached)} SHA256-verified historical baseline rows, {len(needBaseline)} need new baseline runs, {len(visited)} identical numerical headers',flush=True)
