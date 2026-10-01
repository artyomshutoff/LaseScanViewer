from pathlib import Path
import json,hashlib,zipfile,shutil
r=Path(__file__).resolve().parents[1];out=r/'releases/LaseScanViewer-3.18.0';old=r/'releases/LaseScanViewer-3.17.0'
for name,digest in json.loads((old/'SHA256.json').read_text()).items():assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest,name
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as z:
 for name in ['cargo.hpp','volume.hpp','calibration.hpp','calibration_features.hpp','calibration_kernel_model.hpp','registration_focus.hpp','bed_alignment.hpp','model.hpp','water_fill.hpp','water_loaded.hpp']:
  assert (r/'src'/name).read_bytes()==z.read('src/'+name),name
for folder in ['ui-318-x64-final','ui-318-x86-final']:
 p=r/'build'/folder;assert (p/'v2-ok.txt').exists() and not (p/'v2-error.txt').exists()
import subprocess,sys
subprocess.run([sys.executable,str(r/'scripts/check_icon318.py')],check=True)
copies=[('README-LaseScanViewer.md','README.md'),('PERFORMANCE-3.18-report.md','PERFORMANCE-3.18-report.md'),('build/ui-318-x64-final/layers-full-water.png','Viewer-full-water.png'),('build/ui-318-x64-final/layers-all.png','Viewer-layers.png'),('build/perf318/summary.json','Performance-results.json'),('build/ui-318-x64-final/water-level.png','Viewer-water-level.png'),('assets/LaseScanViewer.ico','LaseScanViewer.ico'),('assets/LaseScanViewer.png','LaseScanViewer-icon.png')]
for src,dst in copies:shutil.copy2(r/src,out/dst)
summary=dict(model_validation='PASS all 271 independent predictions x64/x86',assembly_validation='PASS 10000 random vectors, relative error <=1e-14',registration_validation='PASS reversed/oblique/occluded/degenerate synthetic cases and three real pairs vs 3.17',calibration_validation='PASS independent n-gk/dmu predictions, metadata/ROI/units/domain/reference guards',performance='measured before/after on three real pairs',pair_x64='PASS layers/apply/cancel/camera/full-water/cursor/rotation/calculate/align/water/exports/camera/state',pair_x86='PASS layers/apply/cancel/camera/full-water/cursor/rotation/calculate/align/water/exports/camera/state',icon='PASS seven square resolutions in both EXEs',numeric_code='unchanged volume rules/model weights; alignment and SIMD implementations validated against 3.17',previous_release='3.17 hashes verified')
(out/'Tests-summary.json').write_text(json.dumps(summary,indent=2))
names={src for src,_ in copies};names.update(['build.ps1','app.rc','app.manifest','logo2.svg'])
for folder in ['src','tests','scripts','assets']:
 names.update(p.relative_to(r).as_posix() for p in (r/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
names.update(p.relative_to(r).as_posix() for p in (r/'build/perf318').rglob('*') if p.is_file() and p.suffix!='.exe')
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as previous:
 with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
  for name in previous.namelist():
   if name.startswith('build/') and name not in names:z.writestr(name,previous.read(name))
  for name in sorted(names):z.write(r/name,name)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as z:assert z.testzip() is None
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(hashes,indent=2))
archive=r/'releases/LaseScanViewer-3.18.0-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in out.iterdir():
  if p.is_file():z.write(p,out.name+'/'+p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print('PASS x64/x86 layers, full-water, cursor, rotation and exports, embedded square icon, matching alignment and volume results, old version preserved, ZIP CRC')
print(archive)
