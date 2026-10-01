import sqlite3,pathlib,csv,hashlib,json,collections
r=pathlib.Path('D:/lase_scans');db=r/'n-gk/TVM_Measurement_Data_Base.sq3';c=sqlite3.connect(db.as_uri()+'?mode=ro',uri=True);c.row_factory=sqlite3.Row
rows=[];excluded=[]
for f in sorted((r/'n-gk').glob('*_Full.bin')):
 id=int(f.stem.split('_')[0]);row=c.execute('select * from LaseTVM where MeasurementID=?',(id,)).fetchone();empty=f.with_name(f.name.replace('_Full','_Empty'))
 if row is None or not empty.exists():excluded.append((id,'no database record' if row is None else 'no matching Empty'));continue
 row=dict(row)
 if row['TotalVolume'] is None or row['TotalVolume']<=0:excluded.append((id,'non-positive or missing TotalVolume'));continue
 split='holdout' if int(hashlib.sha256(row['TruckID'].encode()).hexdigest(),16)%4==0 else 'development'
 rows.append(dict(id=id,truck=row['TruckID'],split=split,total=row['TotalVolume'],status=row['Status'],truck_volume=row['TruckVolume'],trailer_volume=row['TrailerVolume']))
with (r/'build/ngk-controls.csv').open('w',newline='',encoding='utf-8') as f:
 w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
with (r/'build/ngk-manifest.csv').open('w',newline='',encoding='utf-8') as f:
 w=csv.writer(f);w.writerow(['id','split']);w.writerows((x['id'],x['split']) for x in rows)
(r/'build/ngk-excluded.json').write_text(json.dumps(excluded,indent=2))
print('eligible',len(rows),'split',dict(collections.Counter(x['split'] for x in rows)),'status',dict(collections.Counter(x['status'] for x in rows)),'excluded',dict(collections.Counter(x[1] for x in excluded)),'trailers',sum(x['trailer_volume']>0 for x in rows));print('DB SHA256',hashlib.sha256(db.read_bytes()).hexdigest())
