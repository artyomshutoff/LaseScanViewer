from pathlib import Path
import csv,json,hashlib,zipfile,shutil

r=Path(__file__).resolve().parents[1]
out=r/'releases/LaseScanViewer-3.6.0'
summary=json.loads((r/'build/control36-summary.json').read_text())
assert summary['all']['pairs']==99
assert all(summary[ds]['mape']<1 for ds in ['all','n-gk','dmu'])
learn=list(csv.DictReader((r/'build/volume-audit-3.6.csv').open()))
assert len(learn)==69 and all(not x['error'] and float(x['self_m3'])==0 for x in learn)
for ui in ['ui-360-dmu8','ui-360-14269']:
    assert (r/'build'/ui/'v2-ok.txt').exists() and not (r/'build'/ui/'v2-error.txt').exists()
for version in ['3.5.0','3.5.1']:
    old=r/'releases'/f'LaseScanViewer-{version}'
    for name,digest in json.loads((old/'SHA256.json').read_text()).items():
        assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest,(version,name)
for src,dst in [
    ('README-LaseScanViewer.md','README.md'),
    ('CONTROL-3.6-report.md','CONTROL-3.6-report.md'),
    ('CONTROL-3.6-results.csv','CONTROL-3.6-results.csv'),
    ('build/control36-summary.json','Control-summary.json'),
    ('build/control-full-360.csv','Control-raw.csv'),
    ('build/volume-calibration-model.json','Calibration-model.json'),
    ('build/volume-calibration-validation.csv','Calibration-validation.csv'),
    ('build/volume-calibration-features.csv','Calibration-features.csv'),
    ('build/volume-audit-3.6.csv','Learn-regression.csv'),
    ('build/ui-360-dmu8/full-cargo.png','Viewer-dmu8.png'),
    ('build/ui-360-14269/full-cargo.png','Viewer-14269.png')]:shutil.copy2(r/src,out/dst)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
    for folder in ['src','tests','scripts']:
        for p in (r/folder).rglob('*'):
            if p.is_file() and '__pycache__' not in p.parts:z.write(p,p.relative_to(r))
    names=['build.ps1','app.rc','app.manifest','logo2.svg','README-LaseScanViewer.md',
        'CONTROL-3.6-report.md','CONTROL-3.6-results.csv','build/ngk-controls.csv',
        'build/ngk-full-341-retest.csv','build/ngk-full-350.csv','build/dmu-full-351.csv',
        'build/control-full-360.csv','build/control36-summary.json',
        'build/volume-calibration-model.json','build/volume-calibration-validation.csv',
        'build/volume-calibration-features.csv','build/research35/rim.cpp','build/research35/rim.csv',
        'build/ngk341-retest-input-hashes.json','build/dmu-input-hashes.json',
        'build/learn-volume36.cpp','build/learn-audit-3.1.csv','build/volume-audit-3.6.csv']
    for p in (r/'build/research36').iterdir():
        if p.suffix in ['.cpp','.py','.csv']:names.append(str(p.relative_to(r)))
    for name in names:z.write(r/name,name)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as z:assert z.testzip() is None
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(hashes,indent=2))
archive=r/'releases/LaseScanViewer-3.6.0-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
    for p in out.iterdir():
        if p.is_file():z.write(p,Path(out.name)/p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print('PASS: 99 control pairs, 69 Learn regressions, x86/x64 UI, preserved previous releases, source and ZIP CRC')
print(archive)
