from pathlib import Path
import csv,sqlite3,hashlib,json,statistics,datetime
r=Path(__file__).resolve().parents[1]
rows=list(csv.DictReader((r/'build/ngk-full-341-retest.csv').open()))
assert len(rows)==89 and len({x['id'] for x in rows})==89
assert all(not x['error'] and float(x['null_volume'])==0 for x in rows)
hashes=json.loads((r/'build/ngk341-retest-input-hashes.json').read_text())
assert all(hashlib.sha256((r/p).read_bytes()).hexdigest()==h for p,h in hashes.items())
db=sqlite3.connect((r/'n-gk/TVM_Measurement_Data_Base.sq3').as_uri()+'?mode=ro',uri=True)
previous={int(x['id']):float(x['volume']) for x in csv.DictReader((r/'build/ngk-full-340.csv').open())}
results=[]
for x in rows:
 id=int(x['id']);target,status=db.execute('select TotalVolume,Status from LaseTVM where MeasurementID=?',(id,)).fetchone()
 assert target>0 and status==5
 v=float(x['volume']);results.append(dict(id=id,total_volume_m3=target,calculated_m3=v,error_m3=v-target,error_percent=100*(v-target)/target,absolute_percent=100*abs(v-target)/target,change_from_340_m3=v-previous[id],step=float(x['step']),reconstructed_m3=float(x['reconstructed']),alignment_accepted=x['accepted']=='1',overlap=float(x['overlap'])))
db.close();results.sort(key=lambda x:x['id'])
worst=max(results,key=lambda x:x['absolute_percent']);worst_m3=max(results,key=lambda x:abs(x['error_m3']))
summary=dict(pairs=89,max_percent=worst,max_m3=worst_m3,mean_absolute_percent=statistics.mean(x['absolute_percent'] for x in results),mean_absolute_m3=statistics.mean(abs(x['error_m3']) for x in results),within_5=sum(x['absolute_percent']<=5 for x in results),within_10=sum(x['absolute_percent']<=10 for x in results),weak_alignment=sum(not x['alignment_accepted'] for x in results),max_change_from_340_m3=max(abs(x['change_from_340_m3']) for x in results))
with (r/'NGK-3.4.1-retest.csv').open('w',newline='',encoding='utf-8-sig') as f:
 w=csv.DictWriter(f,fieldnames=list(results[0]));w.writeheader();w.writerows(results)
lines=['# Повторная проверка LaseScanViewer 3.4.1 — n-gk','',
f'Дата отчёта: {datetime.datetime.now().astimezone().isoformat(timespec="seconds")}.','',
'Исходный код проверен на совпадение с исходниками релиза 3.4.1. Тестовый EXE заново собран и вызывает те же registration::align и calculateVolume, что приложение. Все 89 пар заново прочитаны, совмещены и рассчитаны; прежние позы не использованы. Параметры по умолчанию: шаг 100 с автоматическим выбором, нижняя поверхность, порог 50, фильтр бортов, крупнейшая область, восстановление пропусков. Координаты приняты в миллиметрах.','',
'Контроль заново прочитан из LaseTVM.TotalVolume по MeasurementID. Все строки Status=5. База открыта только для чтения. Проверены SHA-256 базы и всех 178 BIN до/после прогона: изменений нет. Из 164 Full доступны 89 пар; 75 Full без одноимённого Empty в статистику не включены.','',
f'- Успешно: **89/89**, ошибок расчёта нет. Все 89 проверок Empty/Empty дали ноль.',
f'- Среднее абсолютное относительное отклонение: **{summary["mean_absolute_percent"]:.4f}%**.',
f'- Среднее абсолютное отклонение: **{summary["mean_absolute_m3"]:.6f} м³**.',
f'- Максимальное относительное отклонение: **{worst["absolute_percent"]:.4f}%**, ID **{worst["id"]}**.',
f'- Максимальное абсолютное отклонение: **{abs(worst_m3["error_m3"]):.6f} м³**, ID **{worst_m3["id"]}**.',
f'- В пределах 5%: **{summary["within_5"]}/89**; в пределах 10%: **{summary["within_10"]}/89**.',
f'- Слабых совмещений: **{summary["weak_alignment"]}**, они включены в общую статистику.',
f'- Максимальное изменение объёма относительно прогона 3.4.0: **{summary["max_change_from_340_m3"]:.12f} м³**.','',
'Относительное отклонение = |расчёт − TotalVolume| / TotalVolume × 100%. Эти данные уже использовались при доработке алгоритма: повторный прогон проверяет воспроизводимость, а не точность на независимом наборе.','',
'| ID | Контроль, м³ | Расчёт, м³ | Ошибка, м³ | Отклонение, % |','|---|---:|---:|---:|---:|']
for x in sorted(results,key=lambda x:-x['absolute_percent'])[:10]:lines.append(f'| {x["id"]} | {x["total_volume_m3"]:.6f} | {x["calculated_m3"]:.6f} | {x["error_m3"]:+.6f} | {x["absolute_percent"]:.4f} |')
(r/'NGK-3.4.1-retest.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
(r/'build/ngk341-retest-summary.json').write_text(json.dumps(summary,indent=2))
print(json.dumps(summary,indent=2))
