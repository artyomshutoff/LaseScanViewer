from pathlib import Path
import json,hashlib,zipfile,shutil
r=Path(__file__).resolve().parents[1];out=r/'releases/LaseScanViewer-3.15.0';old=r/'releases/LaseScanViewer-3.14.0'
for name,digest in json.loads((old/'SHA256.json').read_text()).items():assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest,name
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as z:
 for name in ['analysis.hpp','cargo.hpp','volume.hpp','calibration.hpp','calibration_features.hpp','calibration_kernel_model.hpp','kernel_estimate.hpp','registration.hpp','registration_focus.hpp','bed_alignment.hpp','model.hpp','water_fill.hpp']:
  assert (r/'src'/name).read_bytes()==z.read('src/'+name),name
for folder in ['ui-315-x64','ui-315-x86']:
 p=r/'build'/folder;assert (p/'v2-ok.txt').exists() and not (p/'v2-error.txt').exists()
import subprocess,sys
subprocess.run([sys.executable,str(r/'scripts/check_icon315.py')],check=True)
copies=[('README-LaseScanViewer.md','README.md'),('CURSOR-3.15-report.md','CURSOR-3.15-report.md'),('assets/LaseScanViewer.ico','LaseScanViewer.ico'),('assets/LaseScanViewer.png','LaseScanViewer-icon.png')]
for src,dst in copies:shutil.copy2(r/src,out/dst)
summary=dict(pair_x64='PASS cursor/rotation/calculate/align/water/exports/camera/state',pair_x86='PASS cursor/rotation/calculate/align/water/exports/camera/state',icon='PASS seven square resolutions in both EXEs',numeric_code='byte identical to 3.14',previous_release='3.14 hashes verified')
(out/'Tests-summary.json').write_text(json.dumps(summary,indent=2))
names={src for src,_ in copies};names.update(['build.ps1','app.rc','app.manifest','logo2.svg'])
for folder in ['src','tests','scripts','assets']:
 names.update(p.relative_to(r).as_posix() for p in (r/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as previous:
 with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
  for name in previous.namelist():
   if name.startswith('build/') and name not in names:z.writestr(name,previous.read(name))
  for name in sorted(names):z.write(r/name,name)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as z:assert z.testzip() is None
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(hashes,indent=2))
archive=r/'releases/LaseScanViewer-3.15.0-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in out.iterdir():
  if p.is_file():z.write(p,out.name+'/'+p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print('PASS x64/x86 cursor, rotation and exports, embedded square icon, numerical code unchanged, old version preserved, ZIP CRC')
print(archive)
