"""Synthetic/real regression tests and exact per-platform alternate-mode checks."""
from pathlib import Path
import subprocess,json,csv
r=Path(__file__).resolve().parents[1];out=r/'build/perf331';tools=r/'.tools/llvm-mingw-20260922-ucrt-x86_64/bin';records=[]
def run(args):
 p=subprocess.run([str(x) for x in args],cwd=r,capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=240)
 if p.returncode:raise RuntimeError((args,p.stdout,p.stderr))
 return p.stdout+p.stderr
def compile(source,name,x86=False,unicode=False,flags=()):
 exe=out/(name+'.exe');cc=tools/('i686-w64-mingw32-clang++.exe' if x86 else 'clang++.exe')
 run([cc,'-std=c++17','-O2','-static',*(['-municode'] if unicode else []),*flags,r/'tests'/source,'-o',exe]);return exe
for name in ['nearest331_test','volume_test','bed_alignment_test','kernel_estimate_test','calibration_test','water_fill_test','water_loaded_test','water_preview_test','tarp325_test','bed_dimensions_test','manual_volume_test']:
 exe=compile(name+'.cpp',name);result=run([exe]);records.append(dict(test=name,platform='x64',result=result.strip()));print(name,result.strip(),flush=True)
for name in ['registration_test','structural_alignment_test','registration327_test']:
 unicode=name!='registration_test';exe=compile(name+'.cpp',name,unicode=unicode)
 args=[]
 if name=='structural_alignment_test':args=[r/'n-gk/0000013785_Full.bin',r/'n-gk/0000013785_Empty.bin']
 if name=='registration327_test':args=[r/'n-gk 2/0000012942_Full.bin',r/'n-gk 2/0000012942_Empty.bin']
 result=run([exe,*args]);records.append(dict(test=name,platform='x64',result=result.strip()));print(name,result.strip(),flush=True)
for platform,x86 in [('x64',False),('x86',True)]:
 if x86:
  for name in ['nearest331_test','volume_test','manual_volume_test']:
   exe=compile(name+'.cpp',name+'-x86',x86=True);result=run([exe]);records.append(dict(test=name,platform=platform,result=result.strip()));print(platform,name,result.strip(),flush=True)
 original=compile('pipeline_equivalence320.cpp','modes-baseline-'+platform,x86=x86,unicode=True,flags=['-DPERF_331_BASELINE'])
 changed=compile('pipeline_equivalence320.cpp','modes-optimized-'+platform,x86=x86,unicode=True)
 for ds,id,case in [('n-gk',14301,'ngk14301'),('dmu',8,'dmu8')]:
  outputs=[]
  for mode,exe in [('baseline',original),('optimized',changed)]:
   dest=out/f'modes-{mode}-{platform}-{case}.csv';run([exe,r/ds/f'{id:010}_Full.bin',r/ds/f'{id:010}_Empty.bin',out/f'baseline-{case}-0.csv',dest]);outputs.append(list(csv.DictReader(dest.open())))
  assert outputs[0]==outputs[1],(platform,case,'mode outputs changed')
  records.append(dict(test='six-modes',platform=platform,case=case,result='PASS exact volume, sensitivity, cloud/mesh/cell digests, warnings and monotonic progress'))
  print('PASS',platform,case,'six exact alternate modes',flush=True)
(out/'regression-summary.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8')
print('PASS complete regression suite',len(records),flush=True)
