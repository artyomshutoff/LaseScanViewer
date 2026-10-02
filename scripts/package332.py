"""Package the verified 3.32 release without scans, databases or saved settings."""
from pathlib import Path
import hashlib,json,shutil,zipfile
from PIL import Image
r=Path(__file__).resolve().parents[1];p=r/'build/control332';out=r/'releases/LaseScanViewer-3.32.0';old=r/'releases/LaseScanViewer-3.31.0'
for name,digest in json.loads((old/'SHA256.json').read_text()).items():
    assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest,name
audit=json.loads((p/'summary.json').read_text(encoding='utf-8'))
assert audit['verification']['attempts']==381 and audit['verification']['successful']==380
assert audit['verification']['controls']==374 and len(audit['verification']['changes'])==1
assert audit['verification']['changes'][0]['id']==14387
assert audit['all']['worsened']==0
gpu=(p/'gpu-result.txt').read_text();assert 'PASS exact CPU/GPU alignment and volume' in gpu
for platform in ['x64','x86']:
    gui=r/'build'/('ui-332-'+platform+'-final')
    assert (gui/'v2-ok.txt').exists() and not (gui/'v2-error.txt').exists()
    assert (gui/'manual-ok.txt').exists() and not (gui/'manual-error.txt').exists()
    assert (gui/'theme-dark.png').read_bytes()==(gui/'theme-light.png').read_bytes()
    assert Image.open(gui/'theme-dark.png').getpixel((0,0))==(0,0,0)
    caption=r/'build'/('caption-332-'+platform)
    assert (caption/'caption-ok.txt').exists() and not (caption/'caption-error.txt').exists()
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as archive:
    for path in (r/'src').rglob('*'):
        if path.is_file() and ('model' in path.name or path.suffix=='.S'):
            assert path.read_bytes()==archive.read(path.relative_to(r).as_posix()),path.name
summary=dict(version='3.32.0',control=audit,gpu=gpu.strip(),
    regression='PASS x64/x86: fragmented-refinement guards, real 14387 preserves requested integral/geometry and pose, provisional warning, sensitivity, monotonic progress, adaptive toggle. Existing volume tests pass both architectures; model/domain/ROI/units/metadata-independence test passes with rejected-refinement stale-value guard.',
    full_new='PASS all 30 new Full/Empty pairs aligned from scratch: numerical fields and 190 features match replay; original 3.31 replay matches actual complete runs',
    gui_x64='PASS real 14387: preliminary calculation, camera, ROI, independent layers, SQ3 comparison, manual calculator/units, PNG/HTML/PDF/PLY, black viewport both themes',
    gui_x86='PASS real 14375: same GUI suite and camera preservation',
    captions='PASS both architectures: 6 immediate theme switches, no resize/focus/camera changes',
    previous_release='3.31.0 recursive SHA256 verified; previous models and SSE2 assembly unchanged',
    limitations='Development benchmark, not blind validation; 14387 used during development. True geometry of a deformed/incomplete Empty cannot be guaranteed. Prior model training data included. Windows 11 not executed locally.')
(out/'Tests-summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
names={'README.md','README-LaseScanViewer.md','CONTROL-3.27-report.md','CONTROL-3.27-results.csv',
       'PERFORMANCE-3.31-report.md','CONTROL-3.31-results.csv','CONTROL-3.32-report.md','CONTROL-3.32-results.csv'}
for name in names:shutil.copy2(r/name,out/name)
for source in [r/'assets/LaseScanViewer.png',*sorted((r/'docs').rglob('*'))]:
    if source.is_file():
        target=out/source.relative_to(r);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
for source,target in [('build/ui-332-x64-final/refinement.png','Scan-14387.png'),
                     ('build/ui-332-x64-final/control-report.html','Scan-14387-report.html'),
                     ('build/ui-332-x86-final/manual-calculator.png','Manual-calculator.png'),
                     ('build/caption-332-x64/caption-immediate-light.png','Caption-light.png'),
                     ('build/caption-332-x64/caption-immediate-dark.png','Caption-dark.png')]:
    shutil.copy2(r/source,out/target)
names.update({'build.ps1','app.rc','app.manifest','logo2.svg'})
for folder in ['src','tests','scripts','assets','docs']:
    names.update(path.relative_to(r).as_posix() for path in (r/folder).rglob('*') if path.is_file() and '__pycache__' not in path.parts)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as archive:
    for name in sorted(names):
        assert Path(name).suffix.lower() not in ['.bin','.sq3','.ini']
        archive.write(r/name,name)
settings=out/'LaseScanViewer.ini'
if settings.exists():settings.unlink()
checks={path.relative_to(out).as_posix():hashlib.sha256(path.read_bytes()).hexdigest() for path in out.rglob('*') if path.is_file() and path.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(checks,indent=2),encoding='utf-8')
archive_path=r/'releases/LaseScanViewer-3.32.0-Windows.zip'
with zipfile.ZipFile(archive_path,'w',zipfile.ZIP_DEFLATED) as archive:
    for path in out.rglob('*'):
        if path.is_file():archive.write(path,out.name+'/'+path.relative_to(out).as_posix())
with zipfile.ZipFile(archive_path) as archive:
    assert archive.testzip() is None
    for name,digest in checks.items():assert hashlib.sha256(archive.read(out.name+'/'+name)).hexdigest()==digest,name
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as archive:assert archive.testzip() is None
for name in ['LaseScanViewer.exe','LaseScanViewer-Portable.exe']:shutil.copy2(out/name,r/'dist'/name)
print('PASS 3.32: full volume replay, full new alignment, 379 unchanged successful cases, one improvement, CPU/GPU, GUI x64/x86, previous release and archive integrity')
print(archive_path)
