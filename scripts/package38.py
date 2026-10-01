from pathlib import Path
import csv, json, hashlib, zipfile, shutil

r=Path(__file__).resolve().parents[1]
out=r/'releases/LaseScanViewer-3.8.0'
summary=json.loads((r/'build/control38-summary.json').read_text(encoding='utf-8'))
assert summary['all']['pairs']==181 and summary['holdout']['pairs']==8
failures=json.loads((r/'build/control38-failures.json').read_text())
assert len(failures)==1 and failures[0]['id']==13955
assert summary['all']['mape']<summary['all']['previous_mape']
learn=list(csv.DictReader((r/'build/volume-audit-3.8.csv').open()))
assert len(learn)==69 and all(not x['error'] and float(x['self_m3'])==0 for x in learn)
for ui in ['ui-380-dmu8','ui-380-13965']:
    assert (r/'build'/ui/'v2-ok.txt').exists() and not (r/'build'/ui/'v2-error.txt').exists()
for version in ['3.5.0','3.5.1','3.6.0','3.7.0']:
    old=r/'releases'/f'LaseScanViewer-{version}'
    for name,digest in json.loads((old/'SHA256.json').read_text()).items():
        assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest,(version,name)
copies=[
    ('README-LaseScanViewer.md','README.md'),
    ('CONTROL-3.8-report.md','CONTROL-3.8-report.md'),
    ('CONTROL-3.8-results.csv','CONTROL-3.8-results.csv'),
    ('build/control38-summary.json','Control-summary.json'),
    ('build/control38-failures.json','Failed-pairs.json'),
    ('build/tests38-summary.json','Tests-summary.json'),
    ('build/control-full-380.csv','Control-raw.csv'),
    ('build/research38/baseline.csv','Previous-3.7-raw.csv'),
    ('build/volume38-model.json','Calibration-model.json'),
    ('build/volume38-validation.csv','Calibration-validation.csv'),
    ('build/volume38-features.csv','Calibration-features.csv'),
    ('build/volume-audit-3.8.csv','Learn-regression.csv'),
    ('build/ui-380-dmu8/full-cargo.png','Viewer-dmu8.png'),
    ('build/ui-380-13965/full-cargo.png','Viewer-13965.png')]
for src,dst in copies:shutil.copy2(r/src,out/dst)
names=[src for src,_ in copies]
names += ['build.ps1','app.rc','app.manifest','logo2.svg',
    'CONTROL-3.6-results.csv','build/ngk-controls.csv','build/control-full-370.csv',
    'build/volume37-features.csv','build/volume-audit-3.7.csv',
    'build/ngk-full-341-retest.csv','build/ngk-full-350.csv','build/dmu-full-351.csv',
    'build/ngk341-retest-input-hashes.json','build/dmu-input-hashes.json',
    'build/new-data-input-hashes.json','build/model37-frozen-before-new-data.json',
    'build/control37-known.csv','build/control37-new.csv','build/control37-inventory.csv',
    'build/learn-volume38.cpp','build/learn-audit-3.1.csv','build/volume-audit-3.6.csv']
for folder in ['src','tests','scripts']:
    names += [str(p.relative_to(r)) for p in (r/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts]
for folder in ['build/research35','build/research36','build/research37','build/research38']:
    names += [str(p.relative_to(r)) for p in (r/folder).rglob('*') if p.is_file() and p.suffix in ['.cpp','.hpp','.py','.csv','.json','.log']]
with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
    for name in sorted({Path(name).as_posix() for name in names}):z.write(r/name,Path(name).as_posix())
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as z:assert z.testzip() is None
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(hashes,indent=2))
archive=r/'releases/LaseScanViewer-3.8.0-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
    for p in out.iterdir():
        if p.is_file():z.write(p,Path(out.name)/p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print('PASS: 181 calculated + 1 damaged pair documented; 8 held out; 69 Learn regressions; x86/x64 UI; previous releases and ZIP CRC checked')
print(archive)
