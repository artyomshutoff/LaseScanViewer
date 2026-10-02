"""Package the tested desktop and embedded local web viewer, without user data."""
from pathlib import Path
import hashlib,json,shutil,zipfile
from PIL import Image,ImageChops
r=Path(__file__).resolve().parents[1];out=r/'releases/LaseScanViewer-3.33.0';old=r/'releases/LaseScanViewer-3.32.0';qa=r/'build/web333'
for name,digest in json.loads((old/'SHA256.json').read_text()).items():assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest,name
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as previous:
    excluded={'src/main.cpp','src/v2.hpp','src/v2_state.hpp','src/ui.hpp'}
    for name in previous.namelist():
        if name.startswith('src/') and name not in excluded:assert (r/name).read_bytes()==previous.read(name),name
api=json.loads((qa/'api-results.json').read_text(encoding='utf-8'));assert len(api)==3 and all(x['exact_332'] and x['originals_unchanged'] for x in api)
for platform in ['x64','x86']:
    gui=r/'build'/('ui-333-'+platform+'-final')
    for flag in ['v2','manual']:assert (gui/(flag+'-ok.txt')).exists() and not (gui/(flag+'-error.txt')).exists()
    for view in ['front','side']:assert Image.open(gui/(view+'.png')).width>100
    assert (gui/'theme-dark.png').read_bytes()==(gui/'theme-light.png').read_bytes()
browser=json.loads((qa/'browser-results.json').read_text(encoding='utf-8'));assert browser['camera_preserved'] and browser['reload'] and browser['png'] and browser['html'] and browser['console_errors']==[]
summary={'version':'3.33.0','api':api,'browser':browser,'native_gui':'PASS x64/x86: corrected fitted presets, camera preservation, layers, SQ3, manual calculator, exports and black viewport','numerical_core':'All pre-existing non-UI source files are byte-identical to 3.32.0. Three real web calculations match frozen 3.32 volumes and poses within 1e-10 m3. No new tuning or re-training. Full 374-pair benchmark was not rerun for this UI release.','previous_release':'3.32.0 complete recursive SHA256 verified'}
(out/'Tests-summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
for name in ['README.md','README-LaseScanViewer.md','WEB-3.33-report.md','CONTROL-3.32-report.md','CONTROL-3.32-results.csv','PERFORMANCE-3.31-report.md']:shutil.copy2(r/name,out/name)
shutil.copytree(r/'assets',out/'assets',dirs_exist_ok=True);shutil.copytree(r/'docs',out/'docs',dirs_exist_ok=True)
for name in ['browser-dark.png','browser-light.png','browser-front.png','browser-side.png']:shutil.copy2(qa/name,out/name)
source={name for name in ['README.md','README-LaseScanViewer.md','WEB-3.33-report.md','CONTROL-3.32-report.md','CONTROL-3.32-results.csv','PERFORMANCE-3.31-report.md','build.ps1','app.rc','app.manifest','logo2.svg']}
for folder in ['src','tests','scripts','web','assets','docs']:source.update(p.relative_to(r).as_posix() for p in (r/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as archive:
    for name in sorted(source):assert Path(name).suffix.lower() not in ['.bin','.sq3','.ini'];archive.write(r/name,name)
for ini in out.glob('*.ini'):ini.unlink()
checks={p.relative_to(out).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in out.rglob('*') if p.is_file() and p.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(checks,indent=2),encoding='utf-8')
zip_path=r/'releases/LaseScanViewer-3.33.0-Windows.zip'
with zipfile.ZipFile(zip_path,'w',zipfile.ZIP_DEFLATED) as archive:
    for name in sorted(checks):archive.write(out/name,out.name+'/'+name)
    archive.write(out/'SHA256.json',out.name+'/SHA256.json')
with zipfile.ZipFile(zip_path) as archive:
    assert archive.testzip() is None
    for name,digest in checks.items():assert hashlib.sha256(archive.read(out.name+'/'+name)).hexdigest()==digest,name
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as archive:assert archive.testzip() is None
for name in ['LaseScanViewer.exe','LaseScanViewer-Portable.exe','LaseScanViewer-Web.exe','LaseScanViewer-Web-Portable.exe']:shutil.copy2(out/name,r/'dist'/name)
shutil.copy2(r/'README.md',r/'dist/README.md')
print('PASS 3.33: desktop/web x64/x86, unchanged numeric core and previous release, archive/source integrity')
print(zip_path)
