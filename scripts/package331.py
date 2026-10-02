"""Package only after exact numerical, GUI and independent control checks pass."""
from pathlib import Path
import hashlib, json, shutil, zipfile
from PIL import Image

r = Path(__file__).resolve().parents[1]
out = r / 'releases/LaseScanViewer-3.31.0'
old = r / 'releases/LaseScanViewer-3.30.2'
for name, digest in json.loads((old / 'SHA256.json').read_text()).items():
    assert hashlib.sha256((old / name).read_bytes()).hexdigest() == digest, name
with zipfile.ZipFile(old / 'LaseScanViewer-source.zip') as archive:
    for path in (r / 'src').rglob('*'):
        if path.is_file() and ('model' in path.name or path.suffix == '.S'):
            assert path.read_bytes() == archive.read(path.relative_to(r).as_posix()), path.name

audit = json.loads((r / 'build/perf331/audit/summary.json').read_text(encoding='utf-8'))
verification = audit['verification']
assert verification['attempts'] == 351 and verification['successful'] == 350
assert verification['controls'] == 344 and audit['all']['changed'] == 0
timings = json.loads((r / 'build/perf331/timing-summary.json').read_text())
assert len(timings) == 6 and all(x['runs'] == 3 and x['exact_outputs'] for x in timings)
regression = json.loads((r / 'build/perf331/regression-summary.json').read_text(encoding='utf-8'))
assert len(regression) == 21
x86_equivalence = json.loads((r / 'build/perf331/x86-equivalence.json').read_text())
assert len(x86_equivalence) == 2 and all(x['result'].startswith('PASS exact') for x in x86_equivalence)
for platform in ['x64', 'x86']:
    folder = r / 'build' / ('ui-331-' + platform)
    assert (folder / 'manual-ok.txt').exists() and not (folder / 'manual-error.txt').exists()
    assert (folder / 'v2-ok.txt').exists() and not (folder / 'v2-error.txt').exists()
    assert (folder / 'theme-dark.png').read_bytes() == (folder / 'theme-light.png').read_bytes()
    assert Image.open(folder / 'theme-dark.png').getpixel((0, 0)) == (0, 0, 0)
    caption = r / 'build' / ('caption-331-' + platform)
    assert (caption / 'caption-ok.txt').exists() and not (caption / 'caption-error.txt').exists()
gpu = (r / 'build/perf331/gpu-result.txt').read_text()
assert 'PASS exact CPU/GPU alignment and volume' in gpu

summary = dict(version='3.31.0', numerical_audit=audit, timings=timings,
               regression=regression, x86_equivalence=x86_equivalence, gpu=gpu.strip(),
               gui='PASS x64/x86: Full/Empty, overlay, ROI, camera preservation, independent layers, water/tarp, manual calculator, units, invalid input, PNG/PLY/HTML/PDF and black viewport in both themes',
               caption='PASS x64/x86: 6 immediate theme switches; no resize, focus or camera changes',
               previous_release='3.30.2 archive hashes verified; models and SSE2 assembly unchanged',
               limitations='Performance measured on Ryzen 5 4600H, Windows 10. Windows 11 not run locally. Control data include prior training data; accuracy model unchanged. GPU can be slower than CPU.')
(out / 'Tests-summary.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding='utf-8')
copies = {'README.md': 'README.md', 'README-LaseScanViewer.md': 'README-detailed.md',
          'CONTROL-3.27-report.md': 'CONTROL-3.27-report.md',
          'CONTROL-3.27-results.csv': 'CONTROL-3.27-results.csv',
          'PERFORMANCE-3.31-report.md': 'PERFORMANCE-3.31-report.md',
          'CONTROL-3.31-results.csv': 'CONTROL-3.31-results.csv',
          'build/ui-331-x64/manual-calculator.png': 'Manual-calculator.png',
          'build/ui-331-x64/manual-centimetres.png': 'Manual-centimetres.png',
          'build/ui-331-x64/manual-roundtrip.png': 'Manual-roundtrip.png',
          'build/ui-331-x64/cargo-context.png': 'Bed-dimensions.png',
          'build/caption-331-x64/caption-immediate-light.png': 'Caption-light.png',
          'build/caption-331-x64/caption-immediate-dark.png': 'Caption-dark.png'}
for source, target in copies.items():
    shutil.copy2(r / source, out / target)
shutil.copy2(r / 'README-LaseScanViewer.md', out / 'README-LaseScanViewer.md')
for source in [r / 'assets/LaseScanViewer.png', *sorted((r / 'docs').rglob('*'))]:
    if source.is_file():
        target = out / source.relative_to(r)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
names = {'README.md', 'README-LaseScanViewer.md', 'build.ps1', 'app.rc', 'app.manifest',
         'logo2.svg', 'CONTROL-3.27-report.md', 'CONTROL-3.27-results.csv',
         'PERFORMANCE-3.31-report.md', 'CONTROL-3.31-results.csv'}
for folder in ['src', 'tests', 'scripts', 'assets', 'docs']:
    names.update(path.relative_to(r).as_posix() for path in (r / folder).rglob('*')
                 if path.is_file() and '__pycache__' not in path.parts)
with zipfile.ZipFile(out / 'LaseScanViewer-source.zip', 'w', zipfile.ZIP_DEFLATED) as archive:
    for name in sorted(names):
        assert Path(name).suffix.lower() not in ['.bin', '.sq3', '.ini']
        archive.write(r / name, name)
with zipfile.ZipFile(out / 'LaseScanViewer-source.zip') as archive:
    assert archive.testzip() is None
settings = out / 'LaseScanViewer.ini'
if settings.exists():
    settings.unlink()
(out / 'SHA256.json').write_text(json.dumps({path.relative_to(out).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in out.rglob('*') if path.is_file() and path.name != 'SHA256.json'}, indent=2), encoding='utf-8')
archive_path = r / 'releases/LaseScanViewer-3.31.0-Windows.zip'
with zipfile.ZipFile(archive_path, 'w', zipfile.ZIP_DEFLATED) as archive:
    for path in out.rglob('*'):
        if path.is_file():
            archive.write(path, out.name + '/' + path.relative_to(out).as_posix())
with zipfile.ZipFile(archive_path) as archive:
    assert archive.testzip() is None
    for name, digest in json.loads((out / 'SHA256.json').read_text()).items():
        assert hashlib.sha256(archive.read(out.name + '/' + name)).hexdigest() == digest, name
for name in ['LaseScanViewer.exe', 'LaseScanViewer-Portable.exe']:
    shutil.copy2(out / name, r / 'dist' / name)
print('PASS 3.31.0 exact numerical audit, regression, GUI, unchanged model/assembly/previous release and archive CRC')
print(archive_path)
