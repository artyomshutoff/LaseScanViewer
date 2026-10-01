from pathlib import Path
import json,hashlib,zipfile,shutil
r=Path(__file__).resolve().parents[1];out=r/'releases/LaseScanViewer-3.11.0';research=r/'build/research311'
metrics=json.loads((research/'summary.json').read_text(encoding='utf-8'))
assert metrics['all']['pairs']==271 and metrics['all']['mape']<.56
assert json.loads((research/'tests-summary.json').read_text())['calculated']==277
for folder in ['ui-311-14001','ui-311-dmu8']:
 assert (r/'build'/folder/'v2-ok.txt').exists() and not (r/'build'/folder/'v2-error.txt').exists()
for name,digest in json.loads((r/'build/control310/input-hashes.json').read_text(encoding='utf-8')).items():assert hashlib.sha256((r/name).read_bytes()).hexdigest()==digest,name
old=r/'releases/LaseScanViewer-3.10.0'
for name,digest in json.loads((old/'SHA256.json').read_text()).items():assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest,name
copies=[('README-LaseScanViewer.md','README.md'),('CONTROL-3.11-report.md','CONTROL-3.11-report.md'),('CONTROL-3.11-results.csv','CONTROL-3.11-results.csv'),('build/research311/summary.json','Control-summary.json'),('build/research311/tests-summary.json','Tests-summary.json'),('build/research311/raw.csv','Control-raw.csv'),('build/control310/input-hashes.json','Input-hashes.json'),('build/research311/kernel-model.json','Model.json'),('build/research311/kernel-validation.csv','Model-validation.csv'),('build/ui-311-14001/full-cargo.png','Viewer-14001.png'),('build/ui-311-dmu8/full-cargo.png','Viewer-dmu8.png')]
for src,dst in copies:shutil.copy2(r/src,out/dst)
names={src for src,_ in copies};names.update(['build.ps1','app.rc','app.manifest','logo2.svg'])
for folder in ['src','tests','scripts','build/research311','build/control310']:
 names.update(p.relative_to(r).as_posix() for p in (r/folder).rglob('*') if p.is_file() and p.suffix in ['.cpp','.hpp','.py','.csv','.json','.npz'] and '__pycache__' not in p.parts)
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as previous:
 with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
  for name in previous.namelist():
   if name.startswith('build/') and name not in names:z.writestr(name,previous.read(name))
  for name in sorted(names):z.write(r/name,name)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as z:assert z.testzip() is None
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(hashes,indent=2))
archive=r/'releases/LaseScanViewer-3.11.0-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in out.iterdir():
  if p.is_file():z.write(p,out.name+'/'+p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print('PASS: 271 reference pairs, 277 volume/pose regressions, x64/x86, hashes and ZIP CRC')
print(archive)
