from pathlib import Path
import csv,sqlite3,statistics,json,hashlib
r=Path(__file__).resolve().parents[1]
rows=list(csv.DictReader((r/'build/ngk-full-340.csv').open()))
assert len(rows)==89 and len({x['id'] for x in rows})==89
assert all(not x['error'] and float(x['null_volume'])==0 for x in rows)
database=r/'n-gk/TVM_Measurement_Data_Base.sq3'
assert hashlib.sha256(database.read_bytes()).hexdigest()=='0a1923cf6cda023fae422b8318388047f74e605741fd951108b6f7a3e2ebba21'
db=sqlite3.connect(database.as_uri()+'?mode=ro',uri=True)
old={int(x['id']):float(x['cargo_m3']) for x in csv.DictReader((r/'build/ngk-full-330.csv').open())}
records=[]
for x in rows:
 id=int(x['id']);target,status=db.execute('select TotalVolume,Status from LaseTVM where MeasurementID=?',(id,)).fetchone()
 assert target>0 and status==5
 v=float(x['volume']);prev=old[id]
 records.append(dict(id=id,target_m3=target,previous_m3=prev,volume_m3=v,error_m3=v-target,error_percent=(v/target-1)*100,absolute_percent=abs(v/target-1)*100,previous_absolute_percent=abs(prev/target-1)*100,step=float(x['step']),reconstructed_m3=float(x['reconstructed']),alignment_accepted=x['accepted']=='1'))
db.close();records.sort(key=lambda x:x['id'])
worst=max(records,key=lambda x:x['absolute_percent']);worst_m3=max(records,key=lambda x:abs(x['error_m3']))
summary=dict(pairs=89,mape=statistics.mean(x['absolute_percent'] for x in records),mae_m3=statistics.mean(abs(x['error_m3']) for x in records),max_percent=worst,max_m3=worst_m3,within5=sum(x['absolute_percent']<=5 for x in records),improved=sum(x['absolute_percent']<x['previous_absolute_percent'] for x in records),weak=sum(not x['alignment_accepted'] for x in records))
with (r/'NGK-3.4-results.csv').open('w',encoding='utf-8-sig',newline='') as f:
 w=csv.DictWriter(f,fieldnames=list(records[0]));w.writeheader();w.writerows(records)
lines=['# Проверка LaseScanViewer 3.4.0 по всем 89 парам n-gk','',
'Для каждой пары заново выполнено совмещение и вызван тот же calculateVolume, что и в EXE. Сохранённые позы не использованы. Включены слабые совмещения. Проверены идентификаторы, роли Full/Empty, метка автомобиля и нулевой результат Empty/Empty. Координаты приняты в миллиметрах.','',
'Контроль: LaseTVM.TotalVolume по MeasurementID. База открыта только для чтения, SHA-256 не изменился. Программа не читает базу и не использует идентификаторы или эталонные объёмы в расчёте.','',
'## Результат','',
f'- Среднее абсолютное относительное отклонение: **{summary["mape"]:.4f}%** (3.3: 1.7792%).',
f'- Среднее абсолютное отклонение: **{summary["mae_m3"]:.6f} м³** (3.3: 0.418785 м³).',
f'- Максимальное относительное отклонение: **{worst["absolute_percent"]:.4f}%**, пара **{worst["id"]}** (3.3: 9.3371%).',
f'- Максимальное отклонение в м³: **{abs(worst_m3["error_m3"]):.6f}**, пара **{worst_m3["id"]}**.',
f'- Не больше 5%: **{summary["within5"]}/89**; улучшилось: **{summary["improved"]}/89**; слабых совмещений: {summary["weak"]}.','',
'Отклонение = |расчёт − TotalVolume| / TotalVolume × 100%.','',
'## Изменения и ограничения','',
'- Смешанные ячейки с разбросом высот больше двух шагов уточняются по медиане соседей при наличии минимум 7 из 9 ячеек. Однородно измеренные рёбра не сглаживаются.',
'- После короткой интерполяции восстанавливаются замкнутые пробелы с измеренной границей: не более 256 ячеек и 4% квадрата меньшего размера области, разброс граничных высот не выше 30% этого размера. Итерационная гармоническая поверхность ограничена высотами границы; максимум 600 итераций, остановка при изменении <0.0001 шага. Открытые и отсутствующие в обоих сканах участки не заполняются.',
'- Для плотных сканов (в среднем ≥10 точек на занятую ячейку в каждом скане), в режиме нижней поверхности: сначала шаг 0.75 от заданного; если восстановленный вклад >1% его объёма, используется шаг 1.25. Это эмпирическое правило по геометрии, его можно отключить. Выбранный шаг указан в интерфейсе/отчёте; геометрия и показанные м³ соответствуют одной сетке.',
'- Проверяются пять разрешений и три смещения начала сетки. Диапазон отражает чувствительность, а не подтверждённую погрешность. Прогресс отражает завершённые этапы, искусственных задержек нет.','',
'**Все 89 контрольных объёмов уже известны и использованы при этой доработке. Это результат настройки и повторной проверки на известном наборе, а не независимый тест.** Ранее отложенные 27 пар больше не считаются слепой проверкой 3.4. Для оценки обобщения нужны новые пары. TotalVolume принят как предоставленный контроль; метрологическая достоверность независимо не проверялась.','',
'Проверялись и отвергнуты: простое расширение радиуса интерполяции, дополнительный наклон по верхним точкам, общее сглаживание и понижение квантиля смешанных ячеек. Они ухудшали среднюю ошибку или аналитические проверки.','',
'## Наибольшие отклонения','',
'| ID | Контроль, м³ | Было, м³ | Стало, м³ | Отклонение, % |','|---|---:|---:|---:|---:|']
for x in sorted(records,key=lambda x:-x['absolute_percent'])[:10]:lines.append(f'| {x["id"]} | {x["target_m3"]:.6f} | {x["previous_m3"]:.6f} | {x["volume_m3"]:.6f} | {x["absolute_percent"]:.4f} |')
(r/'NGK-3.4-report.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
(r/'build/ngk34-summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8');print(json.dumps(summary,indent=2))
