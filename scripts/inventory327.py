from pathlib import Path
import sqlite3,json,hashlib
r=Path(__file__).resolve().parents[1];p=r/'build/control327';p.mkdir(exist_ok=True)
inventory=[];hashes={};counts={}
for dataset in ['n-gk 2','n-gk','dmu','тест']:
 folder=r/dataset;db=(r/'n-gk' if dataset=='n-gk 2' else folder)/'TVM_Measurement_Data_Base.sq3'
 c=sqlite3.connect(db.as_uri()+'?mode=ro',uri=True)
 counts[dataset]={'full':0,'pairs':0}
 for f in sorted(folder.glob('*_Full.bin')):
  id=int(f.name[:10]);empty=f.with_name(f.name.replace('_Full.bin','_Empty.bin'));counts[dataset]['full']+=1;counts[dataset]['pairs']+=empty.exists()
  rows=c.execute('select MeasurementID,TruckID,TotalVolume,Status,IncomingMeasurementID,OutgoingMeasurementID from LaseTVM where MeasurementID=?',(id,)).fetchall()
  if not rows:rows=c.execute('select MeasurementID,TruckID,TotalVolume,Status,IncomingMeasurementID,OutgoingMeasurementID from LaseTVM where IncomingMeasurementID=? or OutgoingMeasurementID=?',(id,id)).fetchall()
  inventory.append(dict(dataset=dataset,id=id,full=str(f.relative_to(r)),empty=str(empty.relative_to(r)) if empty.exists() else None,database=str(db.relative_to(r)),controls=rows))
  if empty.exists():
   for file in [f,empty]:hashes[str(file.relative_to(r))]={'sha256':hashlib.sha256(file.read_bytes()).hexdigest(),'size':file.stat().st_size,'mtime_ns':file.stat().st_mtime_ns}
 hashes[str(db.relative_to(r))]={'sha256':hashlib.sha256(db.read_bytes()).hexdigest(),'size':db.stat().st_size,'mtime_ns':db.stat().st_mtime_ns};c.close()
(p/'inventory.json').write_text(json.dumps(inventory,ensure_ascii=False,indent=2),encoding='utf-8');(p/'input-hashes.json').write_text(json.dumps(hashes,ensure_ascii=False,indent=2),encoding='utf-8')
for tag,datasets in [('new',['n-gk 2']),('old',['n-gk','dmu','тест'])]:
 (p/(tag+'.tsv')).write_text(''.join(x['dataset']+'\t'+str(r/x['full'])+'\n' for x in inventory if x['empty'] and x['dataset'] in datasets),encoding='utf-8')
print(json.dumps(counts,ensure_ascii=False,indent=2))
