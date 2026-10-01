"""Compare real application runs; held-out controls are read only here."""
from pathlib import Path
import csv,json,hashlib,sqlite3,statistics,datetime

r=Path(__file__).resolve().parents[1]
def read(name):return list(csv.DictReader((r/name).open(encoding='utf-8-sig')))
def keyed(rows):return {(z['dataset'],int(z['id'])):z for z in rows}
inventory=keyed(read('build/research38/inventory.csv'))
old=keyed(read('build/control-full-380.csv'))
new=keyed(read('build/control-full-390.csv'))
assert len(new)==182 and set(old)==set(new)==set(inventory)
for name in ['build/research38/input-hashes.json','build/research39/frozen-model.json']:
    for file,digest in json.loads((r/name).read_text()).items():
        assert hashlib.sha256((r/file).read_bytes()).hexdigest()==digest,file
validation=keyed(read('build/volume39-validation.csv'))
results=[];failures=[]
for ds in ['n-gk','dmu']:
    db=sqlite3.connect((r/ds/'TVM_Measurement_Data_Base.sq3').as_uri()+'?mode=ro',uri=True)
    for key,z in inventory.items():
        if key[0]!=ds:continue
        a,b=old[key],new[key]
        if a['error'] or b['error']:
            failures.append(dict(dataset=ds,id=key[1],old_error=a['error'],new_error=b['error']))
            continue
        assert float(a['null_volume'])==float(b['null_volume'])==0
        target,status=db.execute('SELECT TotalVolume,Status FROM LaseTVM WHERE MeasurementID=?',(key[1],)).fetchone()
        assert target>0 and status==5
        previous=float(a['calibrated_m3'] if a['calibrated']=='1' else a['volume'])
        shown=float(b['calibrated_m3'] if b['calibrated']=='1' else b['volume'])
        raw=float(b['volume']);oldraw=float(a['volume'])
        assert abs(raw-oldraw)<1e-6,(key,raw,oldraw)
        if key in validation:
            v=validation[key]
            assert abs(raw-float(v['geometric_m3']))<1e-6,(key,raw,v['geometric_m3'])
            assert abs(shown-float(v['training_prediction_m3']))<1e-6,(key,shown,v['training_prediction_m3'])
            assert (b['calibrated']=='1')==(v['training_applied']=='True')
        results.append(dict(dataset=ds,id=key[1],split=z['split'],known_before=z['known'],truck=z['truck'],
            total_volume_m3=target,previous_m3=previous,displayed_m3=shown,
            previous_error_percent=abs(previous/target-1)*100,error_percent=abs(shown/target-1)*100,
            error_m3=shown-target,previous_geometric_m3=oldraw,geometric_m3=raw,
            previous_geometric_error_percent=abs(oldraw/target-1)*100,geometric_error_percent=abs(raw/target-1)*100,
            calibrated=b['calibrated']=='1',accepted=b['accepted']=='1',seconds=float(b['seconds'])))
    db.close()
assert len(results)==181 and len(failures)==1 and failures[0]['id']==13955
def metrics(s):
    return dict(pairs=len(s),previous_mape=statistics.mean(z['previous_error_percent'] for z in s),
        mape=statistics.mean(z['error_percent'] for z in s),
        previous_geometric_mape=statistics.mean(z['previous_geometric_error_percent'] for z in s),
        geometric_mape=statistics.mean(z['geometric_error_percent'] for z in s),
        previous_max=max(z['previous_error_percent'] for z in s),worst=max(s,key=lambda z:z['error_percent']),
        mae_m3=statistics.mean(abs(z['error_m3']) for z in s),within1=sum(z['error_percent']<1 for z in s),
        improved=sum(z['error_percent']<z['previous_error_percent'] for z in s),
        weak=sum(not z['accepted'] for z in s),calibrated=sum(z['calibrated'] for z in s))
summary={}
for name,s in [('all',results),('n-gk',[z for z in results if z['dataset']=='n-gk']),
    ('dmu',[z for z in results if z['dataset']=='dmu']),
    ('development',[z for z in results if z['split']=='development']),
    ('holdout',[z for z in results if z['split']=='holdout']),
    ('new_since_37',[z for z in results if z['known_before']=='False'])]:summary[name]=metrics(s)
with (r/'CONTROL-3.9-results.csv').open('w',newline='',encoding='utf-8-sig') as f:
    w=csv.DictWriter(f,fieldnames=list(results[0]));w.writeheader();w.writerows(sorted(results,key=lambda z:(z['dataset'],z['id'])))
(r/'build/control39-summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
(r/'build/control39-failures.json').write_text(json.dumps(failures,indent=2),encoding='utf-8')
model=json.loads((r/'build/volume39-model.json').read_text())
lines=['# LaseScanViewer 3.9 — сравнение с 3.8','',
'Зафиксированный набор не изменён: 172 пары n-gk и 10 dmu. Рассчитана 181 пара. Пара n-gk 13955 по-прежнему содержит обрезанные Full и Empty, отклоняется обеими версиями и явно указана в Failed-pairs.json. Контрольные объёмы не подставляются вместо данных.','',
'| Выборка | Пар | MAPE 3.8 | MAPE 3.9 |', '|---|---:|---:|---:|']
for name,x in summary.items():lines.append(f'| {name} | {x["pairs"]} | {x["previous_mape"]:.4f}% | {x["mape"]:.4f}% |')
a=summary['all'];h=summary['holdout'];w=a['worst']
lines+=['',f'Максимальная ошибка: {a["previous_max"]:.4f}% → {w["error_percent"]:.4f}%; худшая пара {w["dataset"]} {w["id"]}. Расчёт {w["displayed_m3"]:.6f} м³, TotalVolume {w["total_volume_m3"]:.6f} м³, разница {w["error_m3"]:+.6f} м³.',
f'Средняя абсолютная ошибка {a["mae_m3"]:.6f} м³. Улучшены {a["improved"]}/181 пар, ошибка меньше 1% у {a["within1"]}/181. Все {a["weak"]} слабых совмещений включены. Статистическая поправка применена к {a["calibrated"]}/181 парам.','',
'## Изменение метода','',
'Добавлены десять признаков формы и покрытия груза: площадь, периметр относительно площади, разброс толщины слоя, разброс высот Empty и Full, доля восстановленного объёма, покрытие XY-области, разброс опорной высоты, отношение сторон области груза и средняя толщина. Признаки рассчитываются по существующим интеграционным ячейкам; метаданные файла и автомобиля не используются.',
'Проверены четыре группы дополнительных признаков при четырёх значениях регуляризации: изменения объёма на пяти шагах/трёх смещениях сетки, варианты оценки поверхности и собственно форма груза. Выбран вариант с наименьшей групповой ошибкой на данных разработки: десять признаков формы с регуляризацией 30. Модель содержит 190 признаков вместо 180; прежние устойчивое обучение, ограничения области применимости и предел поправки 8% сохранены.',
'Простое усреднение четырёх положений сетки дало геометрическую ошибку 1.545% против 1.526% на данных разработки; медиана пяти шагов — 2.025%. Эти способы не включены. Алгоритмы чтения, совмещения, выделения точек груза и геометрического интегрирования не изменены. Изменена отдельная статистическая оценка объёма. Геометрия и PLY не растягиваются под контрольное значение.','',
'## Ограничения проверки','',
'Модель обучена на тех же 173 парах разработки. Восемь дополнительных пар шести автомобилей не включены в обучение. Они уже проверялись в предыдущей работе, поэтому нынешнее сравнение не является новым слепым испытанием. Параметры текущей модели выбраны по групповой проверке внутри 173 пар, до просмотра результата обновлённой модели на этих восьми парах. После этой сверки модель не перенастраивалась.',
f'Групповая ошибка с ограничениями применимости: 1.210398% → {model["metrics"]["all"]["guarded_cv_mape"]:.6f}%. Без ограничений новая модель дала {model["metrics"]["all"]["group_cv_mape"]:.6f}%; это ухудшение относительно 1.178756% у 3.8. Поэтому сохранение проверок применимости существенно: экстраполяция за пределы обученной геометрии ненадёжна.',
'Пять групп n-gk исключают целые автомобили, вся dmu исключается шестой группой. Нормализация, веса и диапазоны признаков обучаются без исключённой группы. Эта проверка использована для выбора модели и остаётся исследовательской. Средняя ошибка всей совокупности включает данные обучения и не гарантирует такую точность на будущих сканах.','',
'## Проверки реализации','',
'На всех 181 исправных парах заново выполнен штатный calculateVolume с новыми признаками и моделью. Использованы сохранённые положения базы из полного прогона 3.8: исходники совмещения и геометрического расчёта проверены на байтовую неизменность. Геометрические объёмы обеих версий совпали. Empty/Empty дал ноль; прогресс монотонен и достигает 100%. C++ совпал с независимым Python-прогнозом на 173 парах разработки с допуском 0.000001 м³.',
'Добавлен аналитический тест площади, периметра, разброса, толщины, переноса координат и пустого результата. Проверены модель x64/x86, ограничения применения, неизменность точек/геометрии при уточнении, повторный расчёт и прогресс. Интерфейс x64/x86 проверен на 13965 и dmu 8 с полным совмещением и экспортом PLY/PNG/HTML.',
'SHA256 исходных BIN/SQLite и зафиксированных исходников новой модели проверены. Формула MAPE: среднее(|расчёт−TotalVolume|/TotalVolume)×100%. Используется TotalVolume, не CorrectedTotalVolume. Источники: train_volume_calibration39.py, control39_benchmark.cpp, report_control39.py. Предыдущие релизы сохранены.']
(r/'CONTROL-3.9-report.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(json.dumps(summary,ensure_ascii=False,indent=2))
