from pathlib import Path
import shutil,zipfile,hashlib,json
from PIL import Image
r=Path(__file__).resolve().parents[1];old=r/'releases/LaseScanViewer-3.5.0';out=r/'releases/LaseScanViewer-3.5.1';ui=r/'build/ui-351-14301'
assert (ui/'v2-ok.txt').exists() and not (ui/'v2-error.txt').exists()
for p in old.iterdir():
 if p.is_file() and p.suffix not in ['.exe','.zip','.png'] and p.name not in ['SHA256.json','README.md']:shutil.copy2(p,out/p.name)
shutil.copy2(r/'README-LaseScanViewer.md',out/'README.md')
Image.open(ui/'full-cargo.bmp').save(out/'Viewer-14301.png')
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as original,zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
 for name in original.namelist():
  p=r/name
  if p.is_file():z.write(p,name)
  else:z.writestr(name,original.read(name))
 z.write(r/'scripts/package351.py','scripts/package351.py')
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='SHA256.json'};(out/'SHA256.json').write_text(json.dumps(hashes,indent=2))
archive=r/'releases/LaseScanViewer-3.5.1-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in out.iterdir():
  if p.is_file():z.write(p,Path(out.name)/p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print('PASS: packaged 3.5.1, portable UI test, screenshot, source and ZIP CRC')
