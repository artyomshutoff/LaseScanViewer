"""Freeze BIN pairs and read-only control records for the 3.32 investigation."""
from pathlib import Path
import csv, hashlib, json, sqlite3, zipfile

r = Path(__file__).resolve().parents[1]
p = r / 'build/control332'
p.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(r / 'releases/LaseScanViewer-3.31.0/LaseScanViewer-source.zip') as archive:
    for name in archive.namelist():
        if name.startswith('src/') and not name.endswith('/'):
            target = p / 'baseline' / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(archive.read(name))
entries, hashes, counts = [], {}, {}
for dataset in ['n-gk', 'dmu', 'тест', 'n-gk 2', 'n-gk new']:
    folder = r / dataset
    database = folder / 'TVM_Measurement_Data_Base.sq3'
    if not database.exists() and dataset == 'n-gk 2':
        database = r / 'n-gk/TVM_Measurement_Data_Base.sq3'
    conn = sqlite3.connect(database.as_uri() + '?mode=ro', uri=True) if database.exists() else None
    fulls = sorted(folder.glob('*_Full.bin'))
    missing = []
    for full in fulls:
        empty = full.with_name(full.name.replace('_Full', '_Empty'))
        if not empty.exists():
            missing.append(full.name)
            continue
        number = int(full.name[:10])
        controls = []
        if conn:
            controls = conn.execute('SELECT MeasurementID,TruckID,TotalVolume,Status FROM LaseTVM WHERE MeasurementID=?', (number,)).fetchall()
            if not controls:
                controls = conn.execute('SELECT MeasurementID,TruckID,TotalVolume,Status FROM LaseTVM WHERE IncomingMeasurementID=? OR OutgoingMeasurementID=?', (number, number)).fetchall()
        for file in [full, empty]:
            hashes[file.relative_to(r).as_posix()] = hashlib.sha256(file.read_bytes()).hexdigest()
        entries.append(dict(dataset=dataset, id=number, full=full.relative_to(r).as_posix(), controls=controls))
    if conn:
        conn.close()
        hashes[database.relative_to(r).as_posix()] = hashlib.sha256(database.read_bytes()).hexdigest()
    counts[dataset] = dict(full=len(fulls), pairs=len(fulls)-len(missing), missing_empty=missing)
(p / 'inventory.json').write_text(json.dumps(entries, ensure_ascii=False, indent=2), encoding='utf-8')
(p / 'input-hashes.json').write_text(json.dumps(hashes, ensure_ascii=False, indent=2), encoding='utf-8')
for name, selected in [('all', entries), ('new', [x for x in entries if x['dataset'] == 'n-gk new'])]:
    (p / (name + '.tsv')).write_text(''.join(x['dataset']+'\t'+str(r / x['full'])+'\n' for x in selected), encoding='utf-8')
(p / 'counts.json').write_text(json.dumps(counts, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({k: {f: v for f, v in x.items() if f != 'missing_empty'} for k, x in counts.items()}, ensure_ascii=False, indent=2))
