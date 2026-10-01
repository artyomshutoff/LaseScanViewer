from pathlib import Path
import shutil,zipfile,hashlib,json,csv
root=Path(__file__).resolve().parents[1];out=root/'releases/LaseScanViewer-3.4.0'
assert len(list(csv.DictReader((root/'NGK-3.4-results.csv').open(encoding='utf-8-sig'))))==89
for name in ['14269','14230']:
 folder=root/'build'/('ngk34-ui-'+name)
 assert (folder/'v2-ok.txt').exists() and not (folder/'v2-error.txt').exists()
for src,dst in [('README-LaseScanViewer.md','README.md'),('NGK-3.4-report.md','NGK-3.4-report.md'),('NGK-3.4-results.csv','NGK-3.4-results.csv'),('build/ngk-full-340.csv','NGK-raw.csv'),('build/ngk34-summary.json','NGK-summary.json'),('build/volume-audit-3.4.csv','Learn-regression.csv'),('build/ngk34-ui-14269/full-cargo.png','Volume-14269.png')]:shutil.copy2(root/src,out/dst)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
 for directory in ['src','tests','scripts']:
  for p in (root/directory).rglob('*'):
   if p.is_file() and '__pycache__' not in p.parts:z.write(p,p.relative_to(root))
 for name in ['build.ps1','app.rc','app.manifest','logo2.svg','README-LaseScanViewer.md','NGK-3.4-report.md','NGK-3.4-results.csv','build/ngk-full-330.csv','build/ngk-full-340.csv','build/learn-volume34.cpp','build/learn-audit-3.1.csv']:
  z.write(root/name,name)
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(hashes,indent=2),encoding='utf-8')
archive=root/'releases/LaseScanViewer-3.4.0-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in out.iterdir():
  if p.is_file():z.write(p,Path(out.name)/p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print('PASS packaged binaries, sources, 89 control comparisons, 69 regressions, SHA256, ZIP CRC',archive)
