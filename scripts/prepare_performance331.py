"""Extract an explicit 3.30.2 source archive for reproducible before/after tests.

Usage: python scripts/prepare_performance331.py path/to/LaseScanViewer-source.zip
Then run inventory331.py; run the baseline benchmark for baseline-needed.tsv.
Historical local results are optional: a clean checkout requests a fresh baseline.
"""
from pathlib import Path
import argparse, zipfile

r = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('archive', type=Path)
args = parser.parse_args()
destination = r / 'build/perf331/baseline'
with zipfile.ZipFile(args.archive) as archive:
    main = archive.read('src/main.cpp').decode('utf-8')
    if 'Version: 3.30.2' not in main:
        raise ValueError('Expected the unmodified 3.30.2 source archive')
    for name in archive.namelist():
        relative = Path(name)
        if not name.startswith('src/') or name.endswith('/'):
            continue
        target = (destination / relative).resolve()
        if not target.is_relative_to(destination.resolve()):
            raise ValueError('Unsafe archive member')
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(archive.read(name))
print('Prepared 3.30.2 baseline:', destination)
