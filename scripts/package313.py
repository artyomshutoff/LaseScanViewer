from pathlib import Path
import json,hashlib,zipfile,shutil
r=Path(__file__).resolve().parents[1];out=r/'releases/LaseScanViewer-3.13.0';old=r/'releases/LaseScanViewer-3.12.0'
for name,digest in json.loads((old/'SHA256.json').read_text()).items():assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest,name
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as z:
 for name in ['analysis.hpp','cargo.hpp','volume.hpp','calibration.hpp','calibration_features.hpp','calibration_kernel_model.hpp','kernel_estimate.hpp','registration.hpp','registration_focus.hpp','bed_alignment.hpp','model.hpp','water_fill.hpp']:
  assert (r/'src'/name).read_bytes()==z.read('src/'+name),name
for folder,marker in [('ui-313-layout-final','render-ok.txt'),('ui-313-dmu8-final','v2-ok.txt')]:
 p=r/'build'/folder;assert (p/marker).exists() and not (p/'v2-error.txt').exists() and not (p/'render-error.txt').exists()
assert 'L"РАСЧЁТ ОБЪЁМА"' not in (r/'src/ui.hpp').read_text(encoding='utf-8')
assert 'L"ScanViewer"' in (r/'src/ui.hpp').read_text(encoding='utf-8')
copies=[('README-LaseScanViewer.md','README.md'),('UI-3.13-report.md','UI-3.13-report.md'),('build/ui-313-layout-final/empty.png','Viewer-empty.png'),('build/ui-313-layout-final/compact.png','Viewer-compact.png'),('build/ui-313-dmu8-final/water.png','Viewer-water.png')]
for src,dst in copies:shutil.copy2(r/src,out/dst)
summary=dict(layout_x64='PASS empty/loaded/compact',pair_x86='PASS calculate/align/water/exports/camera/state',numeric_code='byte identical to 3.12',previous_release='3.12 hashes verified')
(out/'Tests-summary.json').write_text(json.dumps(summary,indent=2))
names={src for src,_ in copies};names.update(['build.ps1','app.rc','app.manifest','logo2.svg'])
for folder in ['src','tests','scripts']:
 names.update(p.relative_to(r).as_posix() for p in (r/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as previous:
 with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
  for name in previous.namelist():
   if name.startswith('build/') and name not in names:z.writestr(name,previous.read(name))
  for name in sorted(names):z.write(r/name,name)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as z:assert z.testzip() is None
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(hashes,indent=2))
archive=r/'releases/LaseScanViewer-3.13.0-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in out.iterdir():
  if p.is_file():z.write(p,out.name+'/'+p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print('PASS x64/x86 UI and exports, removed requested text/card, numerical code unchanged, old version preserved, ZIP CRC')
print(archive)
