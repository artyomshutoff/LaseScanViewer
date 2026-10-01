from pathlib import Path
import csv, hashlib, json, shutil, zipfile

r = Path(__file__).resolve().parents[1]
out = r / 'releases/LaseScanViewer-3.10.0'
rows = list(csv.DictReader((r/'build/alignment40/production.csv').open(encoding='utf-8')))
assert len(rows) == 6
assert all(abs(abs(float(x['angle']))-180) < 0.5 and int(x['focused']) == 1 for x in rows)
for name, digest in json.loads((r/'build/alignment40/input-hashes.json').read_text(encoding='utf-8')).items():
    assert hashlib.sha256((r/name).read_bytes()).hexdigest() == digest, name
old = r/'releases/LaseScanViewer-3.9.0'
for name, digest in json.loads((old/'SHA256.json').read_text(encoding='utf-8')).items():
    assert hashlib.sha256((old/name).read_bytes()).hexdigest() == digest, name
for folder in ['ui-310-43279', 'ui-310-43298']:
    assert (r/'build'/folder/'v2-ok.txt').exists()
    assert not (r/'build'/folder/'v2-error.txt').exists()
copies = [
    ('README-LaseScanViewer.md', 'README.md'),
    ('ALIGNMENT-3.10-report.md', 'ALIGNMENT-3.10-report.md'),
    ('build/alignment40/production.csv', 'Alignment-results.csv'),
    ('build/alignment40/input-hashes.json', 'Input-hashes.json'),
    ('build/ui-310-43279/full-cargo.png', 'Viewer-43279.png'),
    ('build/ui-310-43298/full-cargo.png', 'Viewer-43298.png'),
]
for src, dst in copies:
    shutil.copy2(r/src, out/dst)
names = {src for src, _ in copies}
names.update(['build.ps1', 'app.rc', 'app.manifest', 'logo2.svg'])
for folder in ['src', 'tests', 'scripts', 'build/alignment40']:
    names.update(p.relative_to(r).as_posix() for p in (r/folder).rglob('*')
                 if p.is_file() and p.suffix in ['.cpp', '.hpp', '.h', '.py', '.csv', '.json', '.ps1']
                 and '__pycache__' not in p.parts)
# Preserve prior experiment inputs needed by regression scripts.
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as previous:
    with zipfile.ZipFile(out/'LaseScanViewer-source.zip', 'w', zipfile.ZIP_DEFLATED) as z:
        for name in previous.namelist():
            if name.startswith('build/') and name not in names:
                z.writestr(name, previous.read(name))
        for name in sorted(names):
            z.write(r/name, name)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as z:
    assert z.testzip() is None
hashes = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
          for p in out.iterdir() if p.is_file() and p.name != 'SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(hashes, indent=2), encoding='utf-8')
archive = r/'releases/LaseScanViewer-3.10.0-Windows.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    for p in out.iterdir():
        if p.is_file():
            z.write(p, out.name+'/'+p.name)
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
print('PASS: six reversals, input hashes, previous release preserved, x64/x86 GUI results, ZIP CRC')
print(archive)
