"""Package the verified UI release; keep prior binaries and numeric core intact."""
from pathlib import Path
import hashlib,json,shutil,zipfile
from PIL import Image
r=Path(__file__).resolve().parents[1];out=r/'releases/LaseScanViewer-3.34.0';old=r/'releases/LaseScanViewer-3.33.0';qa=r/'build/web334'
for name,digest in json.loads((old/'SHA256.json').read_text()).items():assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest,name
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as archive:
    ui={'src/main.cpp','src/v2.hpp','src/v2_state.hpp','src/ui.hpp','src/layers_ui.hpp','src/web_main.cpp','src/web_core.hpp'}
    for name in archive.namelist():
        if name.startswith('src/') and name not in ui:assert (r/name).read_bytes()==archive.read(name),name
    assert (r/'src/web_core.hpp').read_text(encoding='utf-8').splitlines()==archive.read('src/web_core.hpp').decode('utf-8').replace('3.33.0','3.34.0').splitlines()
api=json.loads((qa/'api-results.json').read_text(encoding='utf-8'));assert len(api)==3 and all(x['exact_332'] and x['originals_unchanged'] for x in api)
assert 'PASS manual x64' in (r/'build/web334-tested.log').read_text() and 'PASS manual x86' in (r/'build/web334-tested.log').read_text()
for platform in ['x64','x86']:
    gui=r/'build'/('ui-334-'+platform+'-verified')
    for flag in ['v2','manual','sidebar']:assert (gui/(flag+'-ok.txt')).exists() and not (gui/(flag+'-error.txt')).exists()
    assert (gui/'theme-dark.png').read_bytes()==(gui/'theme-light.png').read_bytes()
assert (r/'build/theme-334-final/caption-ok.txt').exists()
browser=json.loads((qa/'browser-results.json').read_text(encoding='utf-8'));assert all(browser[k] for k in ['units','add_delete','invalid_input','reopen','camera_preserved','png','html','themes']) and browser['console_errors']==[]
summary={'version':'3.34.0','api':api,'browser':browser,'native_gui':'PASS x64/x86: sidebar layers, scrolling at 1120x820, Full/Empty buttons, explicit Reference, manual calculator, camera preservation, SQ3 and exports','numeric_core':'Existing registration/volume/body extraction source is byte-identical to 3.33.0. Web core changed only version metadata. Manual web arithmetic uses the byte-identical desktop manual_volume.hpp. Full benchmark not rerun.','previous_release':'3.33.0 recursive SHA256 verified'}
(out/'Tests-summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
for name in ['README.md','README-LaseScanViewer.md','INTERFACE-3.34-report.md','WEB-3.33-report.md','CONTROL-3.32-report.md','CONTROL-3.32-results.csv','PERFORMANCE-3.31-report.md']:shutil.copy2(r/name,out/name)
shutil.copytree(r/'assets',out/'assets',dirs_exist_ok=True);shutil.copytree(r/'docs',out/'docs',dirs_exist_ok=True)
for name in ['manual-dark.png','manual-light.png','desktop-sidebar.png','desktop-small.png']:shutil.copy2(qa/name,out/name)
source={name for name in ['README.md','README-LaseScanViewer.md','INTERFACE-3.34-report.md','WEB-3.33-report.md','CONTROL-3.32-report.md','CONTROL-3.32-results.csv','PERFORMANCE-3.31-report.md','build.ps1','app.rc','app.manifest','logo2.svg']}
for folder in ['src','tests','scripts','web','assets','docs']:source.update(p.relative_to(r).as_posix() for p in (r/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as archive:
    for name in sorted(source):assert Path(name).suffix.lower() not in ['.bin','.sq3','.ini'];archive.write(r/name,name)
for ini in out.glob('*.ini'):ini.unlink()
checks={p.relative_to(out).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in out.rglob('*') if p.is_file() and p.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(checks,indent=2),encoding='utf-8')
zip_path=r/'releases/LaseScanViewer-3.34.0-Windows.zip'
with zipfile.ZipFile(zip_path,'w',zipfile.ZIP_DEFLATED) as archive:
    for name in sorted(checks):archive.write(out/name,out.name+'/'+name)
    archive.write(out/'SHA256.json',out.name+'/SHA256.json')
with zipfile.ZipFile(zip_path) as archive:
    assert archive.testzip() is None
    for name,digest in checks.items():assert hashlib.sha256(archive.read(out.name+'/'+name)).hexdigest()==digest,name
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as archive:assert archive.testzip() is None
for name in ['LaseScanViewer.exe','LaseScanViewer-Portable.exe','LaseScanViewer-Web.exe','LaseScanViewer-Web-Portable.exe']:shutil.copy2(out/name,r/'dist'/name)
shutil.copy2(r/'README.md',r/'dist/README.md')
print('PASS 3.34: desktop/web x64/x86, shared manual arithmetic, unchanged numeric core and previous release, archive/source integrity')
print(zip_path)
