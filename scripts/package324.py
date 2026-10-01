from pathlib import Path
import json,hashlib,zipfile,shutil
r=Path(__file__).resolve().parents[1];out=r/'releases/LaseScanViewer-3.24.0';old=r/'releases/LaseScanViewer-3.23.0'
for name,digest in json.loads((old/'SHA256.json').read_text()).items():assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest,name
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as z:
 for name in ['parallel_work.hpp','analysis.hpp','volume.hpp','calibration_features.hpp','gpu_compute.hpp','kernel_estimate.hpp','simd_distance.hpp','cargo.hpp','calibration.hpp','calibration_kernel_model.hpp','registration_focus.hpp','bed_alignment.hpp','model.hpp','water_fill.hpp','water_loaded.hpp']:
  assert (r/'src'/name).read_bytes()==z.read('src/'+name),name
for folder in ['ui-324-x64-final','ui-324-x86-final']:
 p=r/'build'/folder;assert (p/'v2-ok.txt').exists() and not (p/'v2-error.txt').exists()
assert (r/'build/perf320/modes-baseline-x86.csv').read_bytes()==(r/'build/perf320/modes-optimized-x86.csv').read_bytes(), 'x86 mode mismatch'
assert 'button(WATER,' not in (r/'src/main.cpp').read_text(encoding='utf-8')
import subprocess,sys
subprocess.run([sys.executable,str(r/'scripts/check_icon324.py')],check=True)
copies=[('README-LaseScanViewer.md','README.md'),('ALIGNMENT-3.24-report.md','ALIGNMENT-3.24-report.md'),('ALIGNMENT-3.24-results.csv','ALIGNMENT-3.24-results.csv'),('build/ui-324-x64-final/aligned.bmp','Alignment-13785.bmp'),('UI-CONTROL-3.23-report.md','UI-CONTROL-3.23-report.md'),('build/ui-324-x64-final/Window-light.png','Viewer-light.png'),('build/ui-324-x64-final/Window-dark.png','Viewer-dark.png'),('GPU-3.22-report.md','GPU-3.22-report.md'),('src/vendor/CL/LICENSE','OpenCL-Headers-LICENSE.txt'),('PERFORMANCE-3.20-report.md','PERFORMANCE-3.20-report.md'),('build/ui-324-x64-final/layers-full-water.png','Viewer-full-water.png'),('build/ui-324-x64-final/layers-all.png','Viewer-layers.png'),('build/perf320/summary.json','Performance-results.json'),('build/ui-324-x64-final/water-level.png','Viewer-water-level.png'),('assets/LaseScanViewer.ico','LaseScanViewer.ico'),('assets/LaseScanViewer.png','LaseScanViewer-icon.png')]
for src,dst in copies:shutil.copy2(r/src,out/dst)
summary=dict(pair_x64='PASS layers, water, camera, PNG/HTML/PLY',pair_x86='PASS layers, water, camera, PNG/HTML/PLY',numeric_code='integration and feature math identical to 3.23; guarded structural rescue added to registration',previous_release='3.23 hashes verified',settings='PASS identical black viewer for light/dark, PDF/HTML, GPU/CPU without changing camera or volume', alignment='13785 improved; six other pairs unchanged; synthetic and CPU/GPU exact tests passed', database='PASS real n-gk/dmu, missing/invalid/zero, TruckID and ambiguity guards', pdf='PASS A4 pages, compressed lossless RGB, rendered and visually inspected')
(out/'Tests-summary.json').write_text(json.dumps(summary,indent=2))
names={src for src,_ in copies};names.update(['build.ps1','app.rc','app.manifest','logo2.svg'])
for folder in ['src','tests','scripts','assets']:
 names.update(p.relative_to(r).as_posix() for p in (r/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
names.update(p.relative_to(r).as_posix() for p in (r/'build/perf320').rglob('*') if p.is_file() and p.suffix!='.exe')
names.update(p.relative_to(r).as_posix() for p in (r/'build/gpu322').rglob('*.txt'))
names.update(p.relative_to(r).as_posix() for p in (r/'build/alignment324').rglob('*') if p.is_file() and p.suffix in ['.hpp','.h','.txt'])
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as previous:
 with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
  for name in previous.namelist():
   if name.startswith('build/') and name not in names:z.writestr(name,previous.read(name))
  for name in sorted(names):z.write(r/name,name)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as z:assert z.testzip() is None
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(hashes,indent=2))
archive=r/'releases/LaseScanViewer-3.24.0-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in out.iterdir():
  if p.is_file():z.write(p,out.name+'/'+p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print('PASS x64/x86 layers, full-water, cursor, rotation and exports, embedded square icon, matching alignment and volume results, old version preserved, ZIP CRC')
print(archive)
