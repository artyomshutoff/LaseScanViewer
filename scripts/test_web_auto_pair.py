"""Verify the web local-file auto-pair loader via server startup (same code as /api/open)."""
from pathlib import Path
import json,subprocess,time,urllib.request,shutil
root=Path(__file__).resolve().parents[1];out=root/'build/ui3357/auto-pair';out.mkdir(parents=True,exist_ok=True)
full=root/'dmu/0000000010_Full.bin';empty=root/'dmu/0000000010_Empty.bin'
missing=out/'missing';missing.mkdir(exist_ok=True);shutil.copy2(full,missing/full.name)
wrong=out/'wrong';wrong.mkdir(exist_ok=True);shutil.copy2(full,wrong/full.name);shutil.copy2(root/'dmu/0000000007_Empty.bin',wrong/empty.name)
bad=out/'bad';bad.mkdir(exist_ok=True);shutil.copy2(full,bad/full.name);(bad/empty.name).write_bytes(b'invalid')
results=[]
for arch,exe in [('x64','LaseScanViewer-Web.exe'),('x86','LaseScanViewer-Web-Portable.exe')]:
 for name,paths,paired,failed in [('full-first',[full],True,False),('empty-first',[empty],True,False),('explicit-pair',[full,empty],True,False),('missing-pair',[missing/full.name],False,False),('wrong-measurement',[wrong/full.name],False,False),('bad-pair',[bad/full.name],False,True)]:
  ready=out/f'{arch}-{name}.txt';ready.unlink(missing_ok=True)
  p=subprocess.Popen([str(root/'build/ui3357'/exe),'--no-browser','--port','0','--ready-file',str(ready),*[str(x) for x in paths]],creationflags=0x08000000)
  try:
   deadline=time.monotonic()+30
   while not ready.exists():
    assert time.monotonic()<deadline and p.poll() is None,(arch,name);time.sleep(.05)
   url=ready.read_text()
   if failed:
    assert url.startswith('ERROR:'),url;p.wait(timeout=5);assert p.returncode==1
   else:
    origin=url.rstrip('/')
    token=json.load(urllib.request.urlopen(origin+'/api/session'))['token']
    def request(path,method='GET'):
     return urllib.request.urlopen(urllib.request.Request(origin+'/api/'+path,headers={'X-Lase-Token':token},method=method),timeout=10)
    state=json.load(request('status'));assert state['version']=='3.35.7'
    assert state['full']['scan']==10 and bool(state['empty'])==paired,state
    if paired:assert state['empty']['scan']==10,state
    assert b"api('open?layer='+layer" in urllib.request.urlopen(origin+'/viewer.js').read()
    request('stop','POST').close();p.wait(timeout=10);assert p.returncode==0
   results.append({'arch':arch,'case':name,'status':'PASS'})
  finally:
   if p.poll() is None:p.terminate();p.wait(timeout=5)
(out/'results.json').write_text(json.dumps(results,indent=2));print(json.dumps(results,indent=2))
