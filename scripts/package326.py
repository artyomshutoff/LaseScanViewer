from pathlib import Path
import json,hashlib,zipfile,shutil,subprocess,sys
from PIL import Image
r=Path(__file__).resolve().parents[1];out=r/'releases/LaseScanViewer-3.26.0';old=r/'releases/LaseScanViewer-3.25.0'
for name,digest in json.loads((old/'SHA256.json').read_text()).items():assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest,name
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as z:
 for p in (r/'src').rglob('*'):
  if p.is_file() and p.name not in ['main.cpp','ui.hpp','v2.hpp','v2_state.hpp','layers_ui.hpp','preferences_ui.hpp']:
   assert p.read_bytes()==z.read(p.relative_to(r).as_posix()),p.name
for folder in ['ui-326-x64-final','ui-326-x86-final']:
 p=r/'build'/folder;assert (p/'v2-ok.txt').exists() and not (p/'v2-error.txt').exists()
 assert (p/'theme-light.png').read_bytes()==(p/'theme-dark.png').read_bytes()
 assert Image.open(p/'theme-light.png').getpixel((0,0))==(0,0,0)
 for theme in ['light','dark']:Image.open(p/f'theme-{theme}.bmp').save(p/f'Window-{theme}.png')
script=(r/'scripts/check_icon325.py').read_text().replace('3.25.0','3.26.0');(r/'scripts/check_icon326.py').write_text(script)
subprocess.run([sys.executable,str(r/'scripts/check_icon326.py')],check=True)
copies=[('README-LaseScanViewer.md','README.md'),('build/ui-326-x64-final/Window-dark.png','Viewer-dark.png'),('build/ui-326-x64-final/Window-light.png','Viewer-light.png'),('build/ui-326-x64-final/virtual-tarp.png','Viewer-virtual-tarp.png'),('build/ui-326-x64-final/virtual-tarp-surface.png','Viewer-virtual-tarp-surface.png'),('assets/LaseScanViewer.ico','LaseScanViewer.ico'),('assets/LaseScanViewer.png','LaseScanViewer-icon.png')]
for src,dst in copies:shutil.copy2(r/src,out/dst)
summary=dict(camera='PASS non-default rotation, zoom, pan and exact bounds preserved before/during/after alignment and repeated volume calculation',tarp='PASS actual settings checkbox retains all six other layers; point and surface modes',theme='PASS fresh default dark, explicit saved choice respected, unchanged black viewport',gui_x64='PASS n-gk 13785',gui_x86='PASS n-gk 14301',numerical_math='All numerical headers byte-identical to 3.25 source',previous_release='3.25 SHA256 verified',exports='PASS PNG HTML PDF PLY')
(out/'Tests-summary.json').write_text(json.dumps(summary,indent=2))
names={src for src,_ in copies};names.update(['build.ps1','app.rc','app.manifest','logo2.svg'])
for folder in ['src','tests','scripts','assets']:names.update(p.relative_to(r).as_posix() for p in (r/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as previous:
 with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
  for name in previous.namelist():
   if name.startswith('build/') and name not in names:z.writestr(name,previous.read(name))
  for name in sorted(names):z.write(r/name,name)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as z:assert z.testzip() is None
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(hashes,indent=2))
archive=r/'releases/LaseScanViewer-3.26.0-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in out.iterdir():
  if p.is_file():z.write(p,out.name+'/'+p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print('PASS 3.26 package: camera preserved, dark default, combined tarp layers, unchanged math, old hashes, ZIP CRC')
print(archive)
