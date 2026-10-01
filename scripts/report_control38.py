"""Compare real application runs; held-out controls are read only here."""
from pathlib import Path
import csv,json,hashlib,sqlite3,statistics,datetime

r=Path(__file__).resolve().parents[1]
def read(name):return list(csv.DictReader((r/name).open(encoding='utf-8-sig')))
def keyed(rows):return {(z['dataset'],int(z['id'])):z for z in rows}
inventory=keyed(read('build/research38/inventory.csv'))
old=keyed(read('build/research38/baseline.csv'))
new=keyed(read('build/control-full-380.csv'))
assert len(new)==182 and set(old)==set(new)==set(inventory)
for name in ['build/research38/input-hashes.json','build/research38/frozen-model.json']:
    for file,digest in json.loads((r/name).read_text()).items():
        assert hashlib.sha256((r/file).read_bytes()).hexdigest()==digest,file
validation=keyed(read('build/volume38-validation.csv'))
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
with (r/'CONTROL-3.8-results.csv').open('w',newline='',encoding='utf-8-sig') as f:
    w=csv.DictWriter(f,fieldnames=list(results[0]));w.writeheader();w.writerows(sorted(results,key=lambda z:(z['dataset'],z['id'])))
(r/'build/control38-summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
(r/'build/control38-failures.json').write_text(json.dumps(failures,indent=2),encoding='utf-8')
model=json.loads((r/'build/volume38-model.json').read_text())
lines=['# LaseScanViewer 3.8 — расширенный набор n-gk и dmu','',
    f'Дата: {datetime.datetime.now().astimezone().isoformat(timespec="seconds")}.','',
    'Зафиксированы 344 Full в n-gk: 172 имеют Empty с тем же номером, 172 не имеют пары. Также доступны 10 пар dmu. Все 182 пары имеют Status=5 и положительный TotalVolume. На каждой выполнена попытка расчёта обеими версиями.','',
    '**181 пара рассчитана. Пара n-gk 13955 не рассчитана обеими версиями:** оба BIN обрываются внутри профилей. Full: 61 439 байт, заявлено 633 профиля, запись обрывается на профиле с индексом 170; Empty: 1 904 609 байт, заявлено 637 профилей, обрыв на индексе 237. Контроль не подставляется вместо отсутствующих измерений. Эта ошибка чтения не входит в MAPE, но явно включена в число попыток.','',
    '## Сравнение с 3.7','',
    '| Выборка | Пар | MAPE 3.7 | MAPE 3.8 | Геометрия 3.7 | Геометрия 3.8 |',
    '|---|---:|---:|---:|---:|---:|']
for name,s in summary.items():lines.append(f'| {name} | {s["pairs"]} | {s["previous_mape"]:.4f}% | {s["mape"]:.4f}% | {s["previous_geometric_mape"]:.4f}% | {s["geometric_mape"]:.4f}% |')
s=summary['all'];w=s['worst']
lines+=['',f'Максимум: {s["previous_max"]:.4f}% → {w["error_percent"]:.4f}%. Худшая пара 3.8: {w["dataset"]} {w["id"]}; расчёт {w["displayed_m3"]:.6f} м³, контроль {w["total_volume_m3"]:.6f} м³, разница {w["error_m3"]:+.6f} м³.',
    f'Средняя абсолютная ошибка {s["mae_m3"]:.6f} м³; меньше 1% у {s["within1"]}/181 пар; улучшены {s["improved"]}/181. Все {s["weak"]} слабых совмещений учтены. Статистическая поправка применена к {s["calibrated"]}/181 парам, остальные показаны по геометрии.','',
    '## Разделение разработки и проверки','',
    '173 исправные пары использованы для разработки: 163 n-gk и 10 dmu. Из новых автомобилей заранее отложены восемь пар шести автомобилей (SHA256 TruckID mod 4 == 0). Ни один автомобиль отложенной группы не встречается в группе разработки или прежних 132 проверенных парах. Измерения остальных автомобилей используются для разработки. Количественная сверка отложенной группы проведена только после фиксации модели и завершения прогнозов для неё. По нему модель не перенастраивалась.',
    'Средняя ошибка всей совокупности смешивает данные разработки и отложенные данные; она не является независимым подтверждением точности. Отложенная выборка мала и относится к тому же сканеру/набору. Даже её хороший результат не гарантирует точность всех будущих измерений.',
    f'Групповая проверка на данных разработки: {model["metrics"]["all"]["guarded_cv_mape"]:.6f}% с теми же ограничениями, что в приложении. Пять групп n-gk содержат целые автомобили; вся dmu исключается шестой группой. Нормализация, коэффициенты и границы применимости обучаются без исключённой группы. Параметры выбраны по этой проверке, поэтому она исследовательская.','',
    '## Изменения алгоритма','',
    'Девять гипотез опорной высоты кузова теперь взвешиваются по внутреннему разбросу: вес 1/(1+MAD/шаг опор). Число точек не задаёт вес, чтобы плотная сетка не подавляла остальные. Сохраняются ограничения на число опор, покрытие двух координат, минимум шесть гипотез и максимальный сдвиг. Исправление относится к реальному положению Empty и применяется согласованно в геометрии и расчёте.',
    'Модель по 180 геометрическим признакам переобучена на расширенной группе разработки. Используются десять итераций устойчивого взвешивания остатков с порогом 2% относительной поправки и регуляризация 30 для глобальных признаков / 300 для локальных. Это порог функции обучения, а не заявленная погрешность. Сильно отклоняющиеся примеры меньше влияют на общую модель; они не удалены из проверки.',
    'Проверены десять способов объединения опор и 24 сочетания регуляризации, устойчивого обучения и весов автомобилей. Веса опор дают небольшое общее улучшение геометрии; основное изменение итоговой оценки обеспечивает обновлённая статистическая модель. Улучшение не одинаково для всех автомобилей и наборов.',
    'Ограничения применимости и предел поправки 8% сохранены. Если признаки, формат или параметры не подходят, программа возвращает геометрическую оценку. Такие случаи не исключаются из статистики. Модель не читает SQLite и не использует имена файлов, TruckID или MeasurementID для подстановки объёма. Изменён текст ошибки для слишком короткого BIN. Интерфейс и экспорт сохранены.','',
    '## Воспроизводимость','',
    'Версия 3.7 и версия 3.8 заново прочитали и совместили все исправные пары, затем выполнили штатный calculateVolume. Проверены монотонность прогресса и нулевой результат Empty/Empty. Прогнозы C++ совпали с независимым Python-расчётом на 173 парах разработки в пределах 0.000001 м³. SHA256 исходных файлов и базы, а также зафиксированной до внешней проверки модели проверены.',
    'MAPE = среднее(|расчёт−TotalVolume|/TotalVolume)×100%; каждая исправная пара имеет одинаковый вес. Используется TotalVolume, не CorrectedTotalVolume. Скрипты: train_volume_calibration38.py, report_control38.py; тест: control38_benchmark.cpp. Манифест, разделение, хеши, исходные прогнозы и промежуточные эксперименты включены в архив исходников. Предыдущая версия сохранена.']
(r/'CONTROL-3.8-report.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(json.dumps(summary,ensure_ascii=False,indent=2))
