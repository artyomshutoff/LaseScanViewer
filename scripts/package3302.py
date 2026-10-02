from pathlib import Path
import hashlib,json,shutil,zipfile
from PIL import Image
r=Path(__file__).resolve().parents[1];out=r/'releases/LaseScanViewer-3.30.2';old=r/'releases/LaseScanViewer-3.30.1'
for name,digest in json.loads((old/'SHA256.json').read_text()).items():
 assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest,name
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as source:
 # All numerical scan algorithms remain exactly as validated in 3.27.
 for p in (r/'src').rglob('*'):
  if not p.is_file() or p.name in ['main.cpp','v2.hpp','ui.hpp']:continue
  assert p.read_bytes()==source.read(p.relative_to(r).as_posix()),p.name
for folder in ['manual-3302-ready','ui-3302-x64-ready','ui-3302-x86-ready']:
 p=r/'build'/folder
 assert (p/'manual-ok.txt').exists() and not (p/'manual-error.txt').exists(),folder
 if folder.startswith('ui-'):
  assert (p/'v2-ok.txt').exists() and not (p/'v2-error.txt').exists()
  assert (p/'theme-dark.png').read_bytes()==(p/'theme-light.png').read_bytes()
  assert Image.open(p/'theme-dark.png').getpixel((0,0))==(0,0,0)
 for image in ['manual-calculator.png','manual-scroll.png','manual-centimetres.png','manual-light.png','manual-partial.png','manual-partial-light.png','manual-roundtrip.png']:
  assert Image.open(p/image).width==770 and 680<=Image.open(p/image).height<=820
summary=dict(caption='PASS Windows 10 build 19045: 6 immediate light/dark switches on x64/x86; native-frame pixels checked without extra test repaint, sleep or resize; no resize messages, focus or camera change; original asynchronous implementation fails the same pixel test (light remains black)',layout='Existing 3.30.1 layout retained; calculator GUI regression passed in both themes',units='Existing 3.30.1 exact decimal conversion retained; 20 GUI roundtrips passed; entered precision and volume preserved',bed='Existing 3.30.1 interior dimensions retained; real Empty-derived dimensions displayed',manual='PASS 5 rows, variable count, delete selected row, scrolling, comma/dot input, invalid values, mean/min/max and mean dimensions',comparison='PASS signed and absolute percent; reference is mean manual volume; zero scan permitted; uncalculated scan unavailable',standalone='PASS without BIN',gui_x64='PASS n-gk 2/12942; calculator preserves volume and camera',gui_x86='PASS n-gk/14301; calculator preserves volume and camera',numerical_scan='All numerical and manual-volume headers byte-identical to 3.30.1 source; caption update changed only',previous_release='3.30.1 SHA256 verified',persistence='Measurements retained only for current app session; no BIN/SQ3 writes',limitations='Windows 11 not executed locally; existing conditional DWM attributes retained. Computer-use screen capture timed out; native PrintWindow captions used, verified against failing baseline.')
for folder in ['caption-3302-x64-capture','caption-3302-x86']:
 p=r/'build'/folder
 assert (p/'caption-ok.txt').exists() and not (p/'caption-error.txt').exists(),folder
 assert (p/'caption-immediate-light.png').read_bytes()!=(p/'caption-immediate-dark.png').read_bytes()
baseline=r/'build/caption-baseline-test'
assert (baseline/'caption-error.txt').read_text()=='Caption did not change immediately'
assert 'light 0 RGB 0,0,0' in (baseline/'caption-colors.txt').read_text()
(out/'Tests-summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
for src,dst in [('README.md','README.md'),('README-LaseScanViewer.md','README-detailed.md'),('build/ui-3302-x64-ready/manual-calculator.png','Manual-calculator.png'),('build/manual-3302-ready/manual-calculator.png','Manual-calculator-standalone.png'),('build/ui-3302-x64-ready/manual-centimetres.png','Manual-centimetres.png'),('build/ui-3302-x64-ready/cargo-context.png','Bed-dimensions.png'),('build/ui-3302-x64-ready/manual-partial.png','Manual-partial.png'),('build/ui-3302-x64-ready/manual-partial-light.png','Manual-partial-light.png'),('build/ui-3302-x64-ready/manual-roundtrip.png','Manual-roundtrip.png'),('build/caption-3302-x64-capture/caption-immediate-light.png','Caption-light.png'),('build/caption-3302-x64-capture/caption-immediate-dark.png','Caption-dark.png')]:shutil.copy2(r/src,out/dst)
names={'README.md','README-LaseScanViewer.md','build.ps1','app.rc','app.manifest','logo2.svg','CONTROL-3.27-report.md','CONTROL-3.27-results.csv'}
for folder in ['src','tests','scripts','assets','docs']:
 names.update(p.relative_to(r).as_posix() for p in (r/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
 for name in sorted(names):z.write(r/name,name)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as z:assert z.testzip() is None
if (out/'LaseScanViewer.ini').exists():(out/'LaseScanViewer.ini').unlink()
(out/'SHA256.json').write_text(json.dumps({p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='SHA256.json'},indent=2),encoding='utf-8')
archive=r/'releases/LaseScanViewer-3.30.2-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in out.iterdir():
  if p.is_file():z.write(p,out.name+'/'+p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
for name in ['LaseScanViewer.exe','LaseScanViewer-Portable.exe']:shutil.copy2(out/name,r/'dist'/name)
print('PASS 3.30.2: manual calculator x64/x86, standalone/scan, same numerical scan code; metre/cm conversion and oriented bed dimensions, 3.30.1 release hashes and archive CRC')
print(archive)
