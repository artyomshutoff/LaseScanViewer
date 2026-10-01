from pathlib import Path
import json,hashlib,zipfile,shutil,csv
r=Path(__file__).resolve().parents[1];out=r/'releases/LaseScanViewer-3.12.0';old=r/'releases/LaseScanViewer-3.11.0'
for name,digest in json.loads((old/'SHA256.json').read_text()).items():assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest,name
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as z:
 for name in ['analysis.hpp','cargo.hpp','volume.hpp','calibration.hpp','calibration_features.hpp','calibration_kernel_model.hpp','kernel_estimate.hpp','registration.hpp','registration_focus.hpp','bed_alignment.hpp','model.hpp']:
  assert (r/'src'/name).read_bytes()==z.read('src/'+name),name
rows=list(csv.DictReader((r/'build/water312/audit.csv').open(encoding='utf-8')))
assert len(rows)==278 and sum(z['available']=='1' for z in rows)==277
for folder in ['ui-312-14301','ui-312-dmu8']:
 p=r/'build'/folder;assert (p/'v2-ok.txt').exists() and (p/'water.png').exists() and (p/'water-report.html').exists() and not (p/'v2-error.txt').exists()
summary=dict(attempts=278,available=277,damaged=13955,analytic_x64='PASS',analytic_x86='PASS',calibration_regression='PASS',gui_x64='14301 PASS',gui_x86='dmu8 PASS',camera_toggle='PASS',immutable_cargo='PASS',numeric_pipeline_byte_identity='PASS',water_capacity_ground_truth='Unavailable')
(r/'build/water312/tests-summary.json').write_text(json.dumps(summary,indent=2))
copies=[('README-LaseScanViewer.md','README.md'),('WATER-3.12-report.md','WATER-3.12-report.md'),('build/water312/audit.csv','Water-audit.csv'),('build/water312/tests-summary.json','Tests-summary.json'),('build/ui-312-14301/water.png','Water-14301.png'),('build/ui-312-14301/water-interface.png','Viewer-water.png'),('build/ui-312-dmu8/water.png','Water-dmu8.png')]
for src,dst in copies:shutil.copy2(r/src,out/dst)
names={src for src,_ in copies};names.update(['build.ps1','app.rc','app.manifest','logo2.svg'])
for folder in ['src','tests','scripts','build/water312']:
 names.update(p.relative_to(r).as_posix() for p in (r/folder).rglob('*') if p.is_file() and p.suffix in ['.cpp','.hpp','.py','.csv','.json','.npz'] and '__pycache__' not in p.parts)
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as previous:
 with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
  for name in previous.namelist():
   if name.startswith('build/') and name not in names:z.writestr(name,previous.read(name))
  for name in sorted(names):z.write(r/name,name)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as z:assert z.testzip() is None
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(hashes,indent=2))
archive=r/'releases/LaseScanViewer-3.12.0-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in out.iterdir():
  if p.is_file():z.write(p,out.name+'/'+p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print('PASS 277 measured water basins, analytic and x64/x86 GUI tests, unchanged cargo pipeline, previous version preserved, ZIP CRC')
print(archive)
