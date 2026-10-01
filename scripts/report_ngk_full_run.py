from pathlib import Path
import csv,sqlite3,json,statistics,datetime
r=Path('D:/lase_scans');rows=list(csv.DictReader((r/'build/ngk-full-330.csv').open()))
assert len(rows)==89, f'Only {len(rows)} completed'
assert all(not x['error'] for x in rows), [x for x in rows if x['error']]
c=sqlite3.connect((r/'n-gk/TVM_Measurement_Data_Base.sq3').as_uri()+'?mode=ro',uri=True)
records=[]
for x in rows:
 id=int(x['id']);truth=c.execute('select TotalVolume,Status from LaseTVM where MeasurementID=?',(id,)).fetchone();assert truth and truth[0]>0 and truth[1]==5
 assert (r/'n-gk'/f'{id:010d}_Full.bin').exists() and (r/'n-gk'/f'{id:010d}_Empty.bin').exists()
 value=float(x['cargo_m3']);delta=value-truth[0];pct=delta/truth[0]*100
 records.append(dict(MeasurementID=id,TotalVolume=truth[0],CalculatedVolume=value,ErrorM3=delta,ErrorPercent=pct,AbsoluteErrorM3=abs(delta),AbsoluteErrorPercent=abs(pct),Angle=float(x['angle']),ShiftX=float(x['dx']),ShiftY=float(x['dy']),ShiftZ=float(x['dz']),AlignmentAccepted=x['accepted']=='1'))
c.close();assert len({x['MeasurementID'] for x in records})==89
worst_pct=max(records,key=lambda x:x['AbsoluteErrorPercent']);worst_m3=max(records,key=lambda x:x['AbsoluteErrorM3'])
summary={'pairs':len(records),'worst_percent':worst_pct,'worst_m3':worst_m3,'mean_absolute_error_m3':statistics.mean(x['AbsoluteErrorM3'] for x in records),'mean_absolute_error_percent':statistics.mean(x['AbsoluteErrorPercent'] for x in records),'within_5_percent':sum(x['AbsoluteErrorPercent']<=5 for x in records),'within_10_percent':sum(x['AbsoluteErrorPercent']<=10 for x in records),'weak_alignment_count':sum(not x['AlignmentAccepted'] for x in records)}
with (r/'NGK-full-run.csv').open('w',newline='',encoding='utf-8-sig') as f:
 w=csv.DictWriter(f,fieldnames=list(records[0]));w.writeheader();w.writerows(sorted(records,key=lambda x:x['MeasurementID']))
lines=['# Полный повторный прогон n-gk — LaseScanViewer 3.3.0','',
'Повторно прочитаны все 89 пар Full/Empty. Для каждой заново выполнен поиск совмещения, включая уточнение высоты по кузову, затем расчёт на шаге 100 с нижней поверхностью, порогом 50, фильтром и крупнейшей связной областью. Сохранённые преобразования предыдущего прогона не использовались. Масштаб — миллиметры. Расчёт разрешён и при ненадёжном совмещении, как в приложении.','',
'Контроль взят заново из `LaseTVM.TotalVolume` базы `n-gk/TVM_Measurement_Data_Base.sq3` по MeasurementID; база открывалась только для чтения. Все 89 строк имеют Status=5 и положительный TotalVolume.','',
f'- Максимальное абсолютное относительное отклонение: **{worst_pct["AbsoluteErrorPercent"]:.4f}%**, измерение **{worst_pct["MeasurementID"]}**.',
f'- Максимальное абсолютное отклонение в м³: **{worst_m3["AbsoluteErrorM3"]:.6f} м³**, измерение **{worst_m3["MeasurementID"]}**.',
f'- Среднее абсолютное относительное отклонение по всем парам: {summary["mean_absolute_error_percent"]:.4f}%.',
f'- Среднее абсолютное отклонение: {summary["mean_absolute_error_m3"]:.6f} м³.',
f'- Не больше 5%: {summary["within_5_percent"]}/89; не больше 10%: {summary["within_10_percent"]}/89.',
f'- Ненадёжных совмещений (объём всё равно включён в статистику): {summary["weak_alignment_count"]}.','',
'Формула относительного отклонения: |расчёт − TotalVolume| / TotalVolume × 100%. Это проверка всех данных, включая 62 пары разработки и 27 отложенных ранее; она не заменяет отдельную оценку отложенной группы.','',
'## Десять наибольших относительных отклонений','',
'| MeasurementID | TotalVolume, м³ | Расчёт, м³ | Ошибка, м³ со знаком | Отклонение, % |','|---|---:|---:|---:|---:|']
for x in sorted(records,key=lambda x:-x['AbsoluteErrorPercent'])[:10]:lines.append(f'| {x["MeasurementID"]} | {x["TotalVolume"]:.6f} | {x["CalculatedVolume"]:.6f} | {x["ErrorM3"]:+.6f} | {x["AbsoluteErrorPercent"]:.4f} |')
lines += ['', 'Полная таблица всех результатов и преобразований: `NGK-full-run.csv`. Исходные BIN и база не изменялись.']
(r/'NGK-full-run.md').write_text('\n'.join(lines)+'\n',encoding='utf-8');(r/'build/ngk-full-run-summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2))
