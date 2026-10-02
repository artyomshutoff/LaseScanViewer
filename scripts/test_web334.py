"""Exercise the shipped local HTTP server with real BINs and frozen 3.32 results."""
from pathlib import Path
import urllib.parse
import csv, hashlib, json, struct, subprocess, time, urllib.request, urllib.error
r=Path(__file__).resolve().parents[1];out=r/'build/web334';out.mkdir(exist_ok=True)
expected={(x['dataset'],int(x['id'])):x for x in csv.DictReader((r/'build/control332/replay-after/raw.csv').open(encoding='utf-8')) if not x['error']}
results=[]
for arch,exe,cases in [('x64','LaseScanViewer-Web.exe',[('dmu',10),('n-gk new',14387)]),('x86','LaseScanViewer-Web-Portable.exe',[('dmu',10)])]:
    ready=out/('ready-'+arch+'.txt');ready.unlink(missing_ok=True)
    process=subprocess.Popen([str(r/'releases/LaseScanViewer-3.34.0'/exe),'--no-browser','--ready-file',str(ready)],creationflags=0x08000000)
    try:
        deadline=time.monotonic()+30
        while not ready.exists():
            assert time.monotonic()<deadline and process.poll() is None;time.sleep(.1)
        url=ready.read_text();origin,token=url.split('/#');assert origin.startswith('http://127.0.0.1:')
        def request(path,method='GET',body=None,headers=None,code=200):
            h={'X-Lase-Token':token};h.update(headers or {})
            req=urllib.request.Request(origin+'/'+path,data=body,headers=h,method=method)
            try:
                with urllib.request.urlopen(req,timeout=40) as response:status=response.status;data=response.read()
            except urllib.error.HTTPError as e:status=e.code;data=e.read()
            assert status==code,(status,code,data[:300]);return data
        assert b'3.34.0' in request('')
        def manual(rows,unit='m',to=None,code=200):
            fields={'count':len(rows),'unit':unit}
            if to:fields['to']=to
            for i,row in enumerate(rows):
                for j,value in enumerate(row):fields[f'r{i}c{j}']=value
            return json.loads(request('api/manual',method='POST',body=urllib.parse.urlencode(fields).encode(),code=code))
        blank=manual([['','','']]*5);assert not blank['valid'] and blank['mean'] is None and blank['count']==0
        rows=[['5.05','2,31','1.36'],['5','2.2','1.4'],['5.25','2.25','1.3'],['','',''],['','','']]
        measured=manual(rows);expected_mean=(5.05*2.31*1.36+5*2.2*1.4+5.25*2.25*1.3)/3
        assert measured['valid'] and measured['count']==3 and abs(measured['mean']-expected_mean)<1e-12
        assert abs(measured['minimum']-15.35625)<1e-12 and abs(measured['maximum']-15.86508)<1e-12
        converted=manual(rows,to='cm');assert converted['rows'][0]==['505','231','136'] and abs(converted['mean']-measured['mean'])<1e-12
        for i in range(100):
            converted=manual(converted['rows'],unit='cm',to='m');assert converted['rows'][0]==['5.05','2.31','1.36']
            converted=manual(converted['rows'],to='cm');assert abs(converted['mean']-measured['mean'])<1e-12
        assert manual([['2','1','1'],['4','2','1']])['mean']==5 # mean of volumes, not product of means (4.5)
        assert manual([['500','200','100']],unit='cm')['mean']==10
        for invalid in ['0','-1','NaN','1e3','1 2','x','1.2.3']:
            bad=manual([['5','2','1'],[invalid,'2','1']]);assert not bad['valid'] and bad['invalidRow']==2 and bad['mean'] is None
            manual([[invalid,'2','1']],to='cm',code=400)
        partial=manual([['5','','']]);assert partial['invalidRow']==1
        assert manual([['5','','']],to='cm')['rows'][0]==['500','','']
        assert manual([['1','1','1']]*200)['count']==200
        manual([['1','1','1']]*201,code=400)
        manual([['1','1','1']],unit='mm',code=400)
        request('api/manual',method='POST',body=b'count=5&count=1',code=400)
        print('PASS manual',arch,'native arithmetic, 100 decimal round-trips, 1..200 rows, invalid and partial values',flush=True)
        request('api/status',headers={'X-Lase-Token':'wrong'},code=403)
        request('api/status',headers={'Origin':'http://other.invalid'},code=403)
        request('api/status',headers={'Host':'other.invalid'},code=403)
        request('api/calculate',method='POST',body=b'',code=400)
        request('api/full',method='PUT',body=b'broken',code=400)
        for dataset,id in cases:
            full=r/dataset/f'{id:010d}_Full.bin';empty=r/dataset/f'{id:010d}_Empty.bin';db=r/dataset/'TVM_Measurement_Data_Base.sq3'
            before={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [full,empty,db]}
            request('api/full',method='PUT',body=empty.read_bytes(),code=400)
            a=json.loads(request('api/full',method='PUT',body=full.read_bytes()));assert a['full']['scan']==id
            b=json.loads(request('api/empty',method='PUT',body=empty.read_bytes()));assert b['empty']['scan']==id
            same=json.loads(request('api/group?layer=full&group='+str(a['full']['group']),method='POST',body=b''));assert same['full']==a['full']
            request('api/group?layer=full&group=0.5',method='POST',body=b'',code=400)
            mesh=request('api/scene?layer=full&mesh=1');n,indices=struct.unpack('<II',mesh[:8]);assert n==a['full']['points'] and indices>0 and len(mesh)==8+n*24+indices*4
            request('api/calculate?step=NaN',method='POST',body=b'',code=400)
            request('api/calculate?unit=0.1',method='POST',body=b'',code=400)
            start=json.loads(request('api/calculate',method='POST',body=b'',code=202));assert start['busy']
            request('api/empty',method='PUT',body=b'bad',code=409)
            assert manual(rows)['mean']==measured['mean'] # calculator stays available during alignment
            values=[];deadline=time.monotonic()+120
            while True:
                status=json.loads(request('api/status'));values.append(status['progress'])
                if not status['busy']:break
                assert time.monotonic()<deadline;time.sleep(.08)
            assert not status['error'],status
            assert values==sorted(values) and values[-1]==100,values
            v=status['result']['volume'];old=expected[dataset,id]
            assert abs(v['m3']-float(old['calculated_m3']))<1e-10,(v,old)
            assert abs(v['geometric']-float(old['geometric_m3']))<1e-10
            assert status['result']['angle']==float(old['angle'])
            assert v['step']==float(old['step'])
            if id==14387:assert v['provisional'] and 'раздробила' in v['warning']
            for layer in ['full','empty','cargo','context','water','loadedWater','tarp']:
                packet=request('api/scene?layer='+layer+'&mesh=1');n,idx=struct.unpack('<II',packet[:8]);assert len(packet)==8+n*24+idx*4
                if layer in ['cargo','context','empty']:assert n>0
            withdb=json.loads(request('api/database',method='PUT',body=db.read_bytes()));assert withdb['databaseRecords'] and withdb['control']
            assert withdb['result']==status['result'] # SQ3 is comparison only.
            manual_before=withdb['result'];assert manual(rows)['mean']==measured['mean'];assert json.loads(request('api/status'))['result']==manual_before
            assert before=={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [full,empty,db]}
            results.append(dict(architecture=arch,dataset=dataset,id=id,m3=v['m3'],geometric=v['geometric'],total=withdb['control']['total'],progress_samples=values,exact_332=True,originals_unchanged=True))
            print('PASS',arch,dataset,id,v['m3'],flush=True)
        if arch=='x64':
            source=Path('C:/Users/a_shutov/Downloads/Для LASE/Для LASE')
            for file in [source/'Cat3/Reference/Cat3.bin',source/'Белаз 49/Reference/Belaz49.bin']:
                if not file.exists():continue
                digest=hashlib.sha256(file.read_bytes()).hexdigest()
                ref=json.loads(request('api/empty',method='PUT',body=file.read_bytes()))['empty'];assert ref['kind']==3 and ref['points']>0
                for group in range(2):
                    if ref['groups']&(1<<group):
                        selected=json.loads(request(f'api/group?layer=empty&group={group}',method='POST',body=b''));assert selected['empty']['group']==group
                assert digest==hashlib.sha256(file.read_bytes()).hexdigest();print('PASS Reference',file.name,flush=True)
        request('api/stop',method='POST',body=b'');assert process.wait(timeout=15)==0
    finally:
        if process.poll() is None:process.terminate();process.wait()
(out/'api-results.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
print('PASS local origins/session guard, malformed uploads, busy guard, native volume parity, all layers, SQ3 and clean shutdown')
