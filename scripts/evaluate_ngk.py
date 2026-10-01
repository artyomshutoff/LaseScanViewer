import csv,statistics,json,pathlib
r=pathlib.Path('D:/lase_scans');controls={int(x['id']):x for x in csv.DictReader((r/'build/ngk-controls.csv').open())};rows=list(csv.DictReader((r/'build/ngk-final-predictions.csv').open()));assert len(rows)==89
metrics={}
for split in ['development','holdout']:
 a=[x for x in rows if controls[int(x['id'])]['split']==split];metrics[split]={}
 for key in ['old','new']:
  e=[float(x[key])-float(controls[int(x['id'])]['total']) for x in a];ape=[abs(v)/float(controls[int(x['id'])]['total'])*100 for x,v in zip(a,e)]
  metrics[split][key]={'n':len(a),'mae_m3':statistics.mean(map(abs,e)),'mape_percent':statistics.mean(ape),'bias_m3':statistics.mean(e),'max_ape_percent':max(ape),'within_5_percent':sum(v<=5 for v in ape),'within_10_percent':sum(v<=10 for v in ape)}
print(json.dumps(metrics,indent=2));(r/'build/ngk-metrics.json').write_text(json.dumps(metrics,indent=2))
