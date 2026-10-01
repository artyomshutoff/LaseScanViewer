"""Integration tests for the C++ parser/exporter against a local reference scan."""
import pathlib
import struct
import subprocess
import sys
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
exe = root / 'build/inspect.exe'
source = pathlib.Path(sys.argv[1])
data = source.read_bytes()

def run(path, *args):
    return subprocess.run([str(exe), str(path), *map(str, args)], capture_output=True)

with tempfile.TemporaryDirectory(prefix='lase-test-') as tmp:
    tmp = pathlib.Path(tmp)
    output = tmp / 'model.ply'
    result = run(source, output)
    assert result.returncode == 0, result.stderr
    lines = output.read_text().splitlines()
    end = lines.index('end_header')
    vertices = int(next(s for s in lines if s.startswith('element vertex ')).split()[-1])
    faces = int(next(s for s in lines if s.startswith('element face ')).split()[-1])
    points = [tuple(map(float, s.split()[:3])) for s in lines[end+1:end+1+vertices]]
    expected = []
    header = 17 + data[16]
    images = struct.unpack_from('<I', data, header+24)[0]
    pos = header+28
    for _ in range(images):
        for _ in range(6):
            pos += 1+data[pos]
        pos += 20
    groups = struct.unpack_from('<I', data, pos)[0]
    pos += 4
    first_count = pos
    for _ in range(groups):
        count = struct.unpack_from('<I', data, pos)[0]
        pos += 4
        for _ in range(count):
            kind = struct.unpack_from('<I', data, pos+8)[0]
            n = struct.unpack_from('<I', data, pos+28)[0]
            selected = kind == (1 if struct.unpack_from('<I', data, header+12)[0] == 0 else 0)
            pos += 32
            for _ in range(n):
                if selected: expected.append(struct.unpack_from('<iii', data, pos+32))
                attributes = struct.unpack_from('<I', data, pos+44)[0]
                assert attributes in (2,3)
                pos += 52 if attributes == 2 else 60
            pos += 132

    assert pos == len(data)
    assert points == expected, 'Exported XYZ differs from the reference records'
    assert len(lines) == end+1+vertices+faces
    for row in lines[end+1+vertices:]:
        n,a,b,c = map(int,row.split())
        assert n == 3 and len({a,b,c}) == 3
        assert all(0 <= i < vertices for i in [a,b,c])
        for u,v in [(a,b),(b,c),(a,c)]:
            assert sum((points[u][k]-points[v][k])**2 for k in range(3)) <= 250**2
    fixtures = [b'', data[:12], data[:60], data[:-1], data+b'junk', b'not a supported format']
    bad_variant = bytearray(data)
    struct.pack_into('<II', bad_variant, header+12, 0, 99)
    fixtures.append(bad_variant)
    if struct.unpack_from('<II', data, header+12) == (0, 0):
        bad_kind = bytearray(data)
        struct.pack_into('<I', bad_kind, header+20, 2)
        fixtures.append(bad_kind)
    bad_count = bytearray(data)
    struct.pack_into('<I',bad_count,first_count,0xffffffff)
    fixtures.append(bad_count)
    bad_points = bytearray(data)
    struct.pack_into('<I',bad_points,first_count+4+28,0xffffffff)
    fixtures.append(bad_points)
    bad_tag = bytearray(data)
    struct.pack_into('<I',bad_tag,first_count+4+24,5)
    fixtures.append(bad_tag)
    for i, fixture in enumerate(fixtures):
        path = tmp / f'invalid-{i}.bin'
        path.write_bytes(fixture)
        result = run(path)
        assert result.returncode == 1, (i,result.returncode,result.stdout,result.stderr)
    assert run(tmp/'missing.bin').returncode == 1
    print(f'PASS: {vertices} exact XYZ points, {faces} valid faces; 10 invalid/missing inputs rejected')
