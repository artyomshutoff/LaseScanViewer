from pathlib import Path
import csv,sqlite3,json,statistics,hashlib
r=Path(__file__).resolve().parents[1]
rows=list(csv.DictReader((r/'build/ngk-full-350.csv').open()))
assert len(rows)==89 and len({x['id'] for x in rows})==89
assert all(not x['error'] and x['calibrated']=='1' and float(x['null_volume'])==0 for x in rows)
dbpath=r/'n-gk/TVM_Measurement_Data_Base.sq3'
assert hashlib.sha256(dbpath.read_bytes()).hexdigest()=='0a1923cf6cda023fae422b8318388047f74e605741fd951108b6f7a3e2ebba21'
db=sqlite3.connect(dbpath.as_uri()+'?mode=ro',uri=True)
validation={int(x['id']):x for x in csv.DictReader((r/'build/ngk-calibration-validation.csv').open())}
results=[]
for x in rows:
 id=int(x['id']);target,status=db.execute('select TotalVolume,Status from LaseTVM where MeasurementID=?',(id,)).fetchone();assert status==5 and target>0
 raw=float(x['volume']);cal=float(x['calibrated_m3']);cv=float(validation[id]['group_cv_prediction'])
 assert abs(cal-float(validation[id]['training_prediction']))<1e-5
 assert abs(raw-float(validation[id]['geometric']))<1e-5
 results.append(dict(id=id,total_volume=target,geometric_m3=raw,calibrated_m3=cal,calibrated_error_m3=cal-target,calibrated_absolute_percent=abs(cal/target-1)*100,geometric_absolute_percent=abs(raw/target-1)*100,group_cv_m3=cv,group_cv_absolute_percent=abs(cv/target-1)*100,alignment_accepted=x['accepted']=='1'))
db.close();results.sort(key=lambda x:x['id']);worst=max(results,key=lambda x:x['calibrated_absolute_percent'])
summary=dict(pairs=89,training_mape=statistics.mean(x['calibrated_absolute_percent'] for x in results),training_mae_m3=statistics.mean(abs(x['calibrated_error_m3']) for x in results),training_max=worst,geometric_mape=statistics.mean(x['geometric_absolute_percent'] for x in results),group_cv_mape=statistics.mean(x['group_cv_absolute_percent'] for x in results),within1=sum(x['calibrated_absolute_percent']<1 for x in results),within5=sum(x['calibrated_absolute_percent']<=5 for x in results),weak=sum(not x['alignment_accepted'] for x in results))
assert summary['training_mape']<1
with (r/'NGK-3.5-results.csv').open('w',newline='',encoding='utf-8-sig') as f:
 w=csv.DictWriter(f,fieldnames=list(results[0]));w.writeheader();w.writerows(results)
lines=['# LaseScanViewer 3.5.0 — калибровка по n-gk','',
'## Что достигнуто','',
f'На всех **89 известных парах** среднее абсолютное относительное отклонение калиброванной оценки от TotalVolume — **{summary["training_mape"]:.4f}%**, среднее абсолютное отклонение — **{summary["training_mae_m3"]:.6f} м³**. Это результат на данных обучения, а не доказательство точности новых сканов.',
f'Исходный геометрический расчёт: **{summary["geometric_mape"]:.4f}%**. Пятикратная групповая перекрёстная проверка калибровки, с исключением целых автомобилей из обучения линейной модели: **{summary["group_cv_mape"]:.4f}%**. Критерий <1% здесь пока НЕ достигнут. Общий выбор алгоритмов использовал этот же набор, поэтому и групповая проверка не заменяет независимый новый набор.','',
f'Максимальное отклонение калибровки на обучении: **{worst["calibrated_absolute_percent"]:.4f}%**, ID **{worst["id"]}**. Меньше 1% у {summary["within1"]}/89, не больше 5% у {summary["within5"]}/89. Слабые совмещения ({summary["weak"]}) включены в статистику.','',
'## Повторный прогон','',
'После фиксации коэффициентов заново прочитаны BIN, выполнено полное совмещение и вызван calculateVolume для всех 89 пар. Все 89 калиброванных оценок доступны; ошибок нет. Empty/Empty даёт нулевой исходный объём. Результаты C++ совпадают с независимым расчётом модели в Python с допуском 0.00001 м³. TotalVolume заново прочитан из SQLite по MeasurementID; база открыта только для чтения, SHA-256 не изменился.','',
'## Модель и область применимости','',
'Модель получает 48 альтернативных объёмов из геометрии. Для четырёх размеров XY-ячеек верхних опорных точек (размах/60, /100, /160, /250), четырёх порогов верхней части Empty (80, 90, 95, 98%) и трёх квантилей высоты (50, 90, 100%) вычисляется медианная поправка по совпадающим ячейкам. По каждой поправке оценивается чувствительность интеграла. Признаки — относительные отличия этих объёмов от исходного.',
'Регуляризованная линейная модель предсказывает относительную поправку: Vcal = Vgeom × (1 + intercept + Σ coefficient[j] × feature[j]). Стандартизация признаков с минимальным масштабом 0.001, ridge λ=10. Встроены только числовые коэффициенты и границы признаков. В EXE нет контрольных объёмов, идентификаторов измерений, названий автомобилей, поиска по BIN-хешам или чтения базы.',
'Модель обучена на 89 парах / 51 автомобиле. В перекрёстной проверке группы формируются SHA256(TruckID) mod 5; стандартизация и коэффициенты заново обучаются на четырёх группах. TruckID используется только для разбиения при проверке, не в прогнозе.',
'Калибровка доступна для Full/Empty группы 0, предполагаемых миллиметров и стандартных параметров расчёта (шаг 100 с автоматическим выбором, нижняя поверхность, порог 50, крупнейшая область, фильтр бортов и восстановление). Для ручной области, Reference, иных настроек и значительного выхода признаков за обученный диапазон она отключается. Относительная поправка ограничена 8%; на обучении фактически от −2.76% до +3.23%. Это защита от экстраполяции, а не гарантия применимости внутри диапазона.','',
'## Отображение','',
'Калиброванная оценка явно подписана. Исходный геометрический объём показан рядом и сохранён в HTML. Геометрия, цветное облако, серый кузов и экспорт PLY остаются результатом исходного геометрического интеграла. Калибровка не дорисовывает груз и не меняет основание для совпадения с контролем. Опция отключается в настройках.','',
'## Проверенные альтернативы','',
'До калибровки проверены 48 способов выбора верхних опорных точек, интерполяция по треугольникам, совмещение точка–плоскость, дополнительные наклоны и небольшие изменения масштаба XY. Лучший эксперимент с геометрией дал около 1.29%, но не <1%. Эти эксперименты не включены в рабочую геометрию; сохранена проверенная версия 3.4.1.','',
'## Наибольшие отклонения калибровки на обучении','',
'| ID | TotalVolume | Геометрия | Калибровка | Отклонение, % |','|---|---:|---:|---:|---:|']
for x in sorted(results,key=lambda x:-x['calibrated_absolute_percent'])[:10]:lines.append(f'| {x["id"]} | {x["total_volume"]:.6f} | {x["geometric_m3"]:.6f} | {x["calibrated_m3"]:.6f} | {x["calibrated_absolute_percent"]:.4f} |')
(r/'NGK-3.5-report.md').write_text('\n'.join(lines)+'\n',encoding='utf-8');(r/'build/ngk35-summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2))
