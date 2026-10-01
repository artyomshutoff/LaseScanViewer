from pathlib import Path
import subprocess,time,json
r=Path(__file__).resolve().parents[1];build=r/'build/perf325';build.mkdir(exist_ok=True)
compiler=r/'.tools/llvm-mingw-20260922-ucrt-x86_64/bin/clang++.exe'
def run(args):return subprocess.run([str(x) for x in args],cwd=r,check=True,capture_output=True,text=True).stdout
run([compiler,'-std=c++17','-O2','-static','-municode','tests/alignment_probe324.cpp','-o',build/'probe.exe'])
results=[]
for dataset,id in [('n-gk',13785),('n-gk',13770),('n-gk',13784),('n-gk',14269),('n-gk',14301),('dmu',8),('тест',43279)]:
 full=r/dataset/f'{id:010d}_Full.bin';empty=r/dataset/f'{id:010d}_Empty.bin'
 if not full.exists() or not empty.exists():
  candidates=list((r/dataset).rglob(f'{id:010d}_Full.bin'))
  if candidates:full=candidates[0];empty=full.with_name(f'{id:010d}_Empty.bin')
 if not full.exists() or not empty.exists():print('MISSING',dataset,id,flush=True);continue
 row={'dataset':dataset,'id':id,'runs':[]}
 for exe in [r/'build/alignment324/optimized.exe',build/'probe.exe']:
  t=time.perf_counter();line=run([exe,full,empty]).strip();elapsed=time.perf_counter()-t;row['runs'].append({'seconds':elapsed,'csv':line})
 assert row['runs'][0]['csv']==row['runs'][1]['csv'],row
 results.append(row);(build/'results.json').write_text(json.dumps(results,indent=2),encoding='utf-8');print(dataset,id,'EXACT',*[round(x['seconds'],3) for x in row['runs']],flush=True)
