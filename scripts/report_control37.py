from pathlib import Path
import csv,sqlite3,json,statistics,hashlib,datetime
r=Path(__file__).resolve().parents[1]
rows=list(csv.DictReader((r/'build/control-full-370.csv').open(encoding='utf-8')))
assert len(rows)==99 and len({(z['dataset'],z['id']) for z in rows})==99
assert all(not z['error'] and z['calibrated']=='1' and float(z['null_volume'])==0 for z in rows)
for manifest in ['build/ngk341-retest-input-hashes.json','build/dmu-input-hashes.json']:
    for path,digest in json.loads((r/manifest).read_text()).items():
        p=Path(path);p=p if p.is_absolute() else r/p
        assert hashlib.sha256(p.read_bytes()).hexdigest()==digest,str(p)
old={(z['dataset'],int(z['id'])):z for z in csv.DictReader((r/'CONTROL-3.6-results.csv').open(encoding='utf-8-sig'))}
validation={(z['dataset'],int(z['id'])):z for z in csv.DictReader((r/'build/volume37-validation.csv').open())}
results=[]
for ds,count in [('n-gk',89),('dmu',10)]:
    subset=[z for z in rows if z['dataset']==ds];assert len(subset)==count
    db=sqlite3.connect((r/ds/'TVM_Measurement_Data_Base.sq3').as_uri()+'?mode=ro',uri=True)
    for z in subset:
        ident=int(z['id']);target,status=db.execute('SELECT TotalVolume,Status FROM LaseTVM WHERE MeasurementID=?',(ident,)).fetchone()
        assert target>0 and status==5
        key=(ds,ident);prev=old[key];val=validation[key];raw=float(z['volume']);shown=float(z['calibrated_m3'])
        assert abs(raw-float(val['geometric_m3']))<1e-6,(key,raw,val['geometric_m3'])
        assert abs(shown-float(val['training_prediction_m3']))<1e-6,(key,shown,val['training_prediction_m3'])
        results.append(dict(dataset=ds,id=ident,total_volume_m3=target,
            previous_geometric_m3=float(prev['geometric_m3']),geometric_m3=raw,
            previous_displayed_m3=float(prev['displayed_m3']),displayed_m3=shown,
            error_m3=shown-target,absolute_error_percent=abs(shown/target-1)*100,
            previous_absolute_error_percent=float(prev['absolute_error_percent']),
            geometric_absolute_error_percent=abs(raw/target-1)*100,
            previous_geometric_error_percent=float(prev['geometric_absolute_error_percent']),
            group_cv_m3=float(val['group_cv_prediction_m3']),guarded_cv_m3=float(val['guarded_cv_prediction_m3']),
            guarded_cv_applied=val['guarded_cv_applied']=='True',alignment_accepted=z['accepted']=='1',
            bed_shift=float(z['bed_shift']),bed_spread=float(z['bed_spread']),bed_variants=int(z['bed_variants']),
            datum_min_m3=float(z['datum_min']),datum_max_m3=float(z['datum_max']),seconds=float(z['seconds'])))
    db.close()
results.sort(key=lambda z:(z['dataset'],z['id']))
summary={}
for name,s in [('all',results),('n-gk',[z for z in results if z['dataset']=='n-gk']),('dmu',[z for z in results if z['dataset']=='dmu'])]:
    worst=max(s,key=lambda z:z['absolute_error_percent'])
    summary[name]=dict(pairs=len(s),mape=statistics.mean(z['absolute_error_percent'] for z in s),
        previous_mape=statistics.mean(z['previous_absolute_error_percent'] for z in s),
        geometric_mape=statistics.mean(z['geometric_absolute_error_percent'] for z in s),
        previous_geometric_mape=statistics.mean(z['previous_geometric_error_percent'] for z in s),
        mae_m3=statistics.mean(abs(z['error_m3']) for z in s),worst=worst,
        group_cv_mape=statistics.mean(abs(z['group_cv_m3']/z['total_volume_m3']-1)*100 for z in s),
        guarded_cv_mape=statistics.mean(abs(z['guarded_cv_m3']/z['total_volume_m3']-1)*100 for z in s),
        within1=sum(z['absolute_error_percent']<1 for z in s),weak=sum(not z['alignment_accepted'] for z in s))
    assert summary[name]['mape']<summary[name]['previous_mape']
    assert summary[name]['geometric_mape']<summary[name]['previous_geometric_mape']
with (r/'CONTROL-3.7-results.csv').open('w',newline='',encoding='utf-8-sig') as f:
    w=csv.DictWriter(f,fieldnames=list(results[0]));w.writeheader();w.writerows(results)
(r/'build/control37-summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
lines=['# LaseScanViewer 3.7 — сравнение с версией 3.6','',
f'Дата: {datetime.datetime.now().astimezone().isoformat(timespec="seconds")}.','',
'## Исходные 99 пар','',
'Модель и геометрический метод доработаны на этих данных. Значения ниже описывают повторный прогон известных файлов; это не слепой тест. Дополнительные файлы, появившиеся после фиксации модели, проверяются отдельно и не включены в эту таблицу.','',
'| Набор | Пар | Оценка 3.6 | Оценка 3.7 | Геометрия 3.6 | Геометрия 3.7 |',
'|---|---:|---:|---:|---:|---:|']
for name in ['all','n-gk','dmu']:
    s=summary[name];lines.append(f'| {name} | {s["pairs"]} | {s["previous_mape"]:.4f}% | {s["mape"]:.4f}% | {s["previous_geometric_mape"]:.4f}% | {s["geometric_mape"]:.4f}% |')
w=summary['all']['worst']
lines+=['',f'Среднее абсолютное отклонение в м³: {summary["all"]["mae_m3"]:.6f}. Ошибка <1% у {summary["all"]["within1"]}/99 отдельных пар.',
f'Максимум: {w["absolute_error_percent"]:.4f}%, {w["dataset"]}, ID {w["id"]}. Расчёт {w["displayed_m3"]:.6f} м³, контроль {w["total_volume_m3"]:.6f} м³, разница {w["error_m3"]:+.6f} м³. Все {summary["all"]["weak"]} слабых совмещения включены в статистику.','',
'## Что изменилось','',
'После глобального совмещения высота базы уточняется по бортам непосредственно рядом с выделенным кузовом. Кабина, дорога и удалённые платформы исключаются из опорной области. Сравниваются девять гипотез: три пространственных разрешения и три порога верхней части Empty. Каждая гипотеза использует медиану высотных остатков; валидные гипотезы имеют равный вес, чтобы плотная сетка не доминировала над остальными.',
'Для принятия гипотезы нужны не менее 24 опор, достаточное распределение по обеим сторонам области и ограниченный разброс. Для применения поправки нужны минимум 6 из 9 гипотез. Поправка ограничена 1% горизонтального размаха, разброс — 0.75%. При недостатке опор сохраняется исходное совмещение. Исправление применяется к реальному сдвигу Empty: наложение, разность поверхностей, геометрический объём, фильтр кузова и PLY используют одинаковые координаты.',
'В HTML-отчёт добавлены поправка по бортам, число опорных вариантов, разброс высоты и диапазон чувствительности объёма к этой высоте. Диапазон не является доверительным интервалом или гарантированной погрешностью. Повторное нажатие «Рассчитать» не накапливает поправку.',
'Модель уточнения расширена до 180 признаков: 144 глобальных признака и 36 альтернативных объёмов по локальным опорам кузова. Регуляризация локальных признаков сильнее (λ=100 против λ=10 для глобальных), чтобы коррелированные варианты не доминировали в модели. Статистическая поправка остаётся отдельной: она не растягивает облако и не добавляет точек.',
'Дополнительно испытаны шесть вариантов уточнения наклона/масштаба, четыре способа обработки смешанных высотных ячеек, 36 локальных опорных вариантов, восемь способов объединения поправок и 20 сочетаний шага/положения сетки на каждой паре. В основной алгоритм включены изменения с лучшим общим результатом; наклоны, деформации и агрессивное заполнение не включены, поскольку не дали устойчивого улучшения на обоих наборах.','',
'## Проверка исключённых групп','',
f'Повторная групповая проверка самой модели: {summary["all"]["group_cv_mape"]:.6f}%. С ограничениями применимости, такими же как в программе, и возвратом к геометрическому объёму: {summary["all"]["guarded_cv_mape"]:.6f}%. Это всё ещё немного выше 1%.',
'Пять групп n-gk содержат целые автомобили (SHA256 TruckID mod 5), вся dmu исключается шестой группой. Стандартизация, коэффициенты и диапазоны применимости обучаются без исключённой группы. Все эти данные участвовали в исследовании структуры алгоритма и параметров: групповая проверка не является слепым внешним испытанием.',
'Диапазоны признаков и ограничение поправки 8% предотвращают часть экстраполяций, но не гарантируют точность внутри диапазона. В новых данных может применяться исходный геометрический результат; такие случаи должны учитываться в проверке, а не исключаться из средней ошибки.','',
'## Воспроизводимость','',
'Все 99 пар заново прочитаны, совмещены и рассчитаны штатными registration::align и calculateVolume. Прогресс проверен на монотонность. Empty/Empty дал ноль для каждой пары. Python-оценки обученной модели и C++ совпали с допуском 0.000001 м³. TotalVolume заново прочитан из SQLite read-only по папке и MeasurementID, проверены Status=5 и метаданные BIN. Хеши всех исходных 99 пар и баз совпали с прежними сохранёнными значениями.',
'Формула MAPE: среднее(|расчёт−TotalVolume|/TotalVolume)×100%. Все допустимые пары имеют одинаковый вес. Контроль — TotalVolume, а не CorrectedTotalVolume. Дополнительные новые пары оцениваются отдельно после фиксации кода и модели.',
'CONTROL-3.7-results.csv содержит каждую пару. Исходники включают scripts/train_volume_calibration37.py, tests/control37_benchmark.cpp, tests/manifest_benchmark.cpp и scripts/report_control37.py; для воспроизведения нужны исходные BIN и SQLite. Предыдущие EXE и отчёты сохранены.']
(r/'CONTROL-3.7-report.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(json.dumps(summary,indent=2))
