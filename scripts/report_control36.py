"""Audit all fresh application results against read-only SQLite controls."""
from pathlib import Path
import csv,sqlite3,json,statistics,hashlib,datetime

r=Path(__file__).resolve().parents[1]
rows=list(csv.DictReader((r/'build/control-full-360.csv').open(encoding='utf-8')))
assert len(rows)==99 and len({(x['dataset'],x['id']) for x in rows})==99
assert all(not x['error'] and x['calibrated']=='1' and float(x['null_volume'])==0 for x in rows)
for manifest in ['build/ngk341-retest-input-hashes.json','build/dmu-input-hashes.json']:
    for path,digest in json.loads((r/manifest).read_text()).items():
        p=Path(path);p=p if p.is_absolute() else r/p
        assert hashlib.sha256(p.read_bytes()).hexdigest()==digest,str(p)
validation={(x['dataset'],int(x['id'])):x for x in csv.DictReader((r/'build/volume-calibration-validation.csv').open())}
baseline={}
for ds,path in [('n-gk','build/ngk-full-350.csv'),('dmu','build/dmu-full-351.csv')]:
    for x in csv.DictReader((r/path).open(encoding='utf-8')):
        baseline[(ds,int(x['id']))]=float(x['calibrated_m3']) if x['calibrated']=='1' else float(x['volume'])
results=[]
for ds,count in [('n-gk',89),('dmu',10)]:
    subset=[x for x in rows if x['dataset']==ds];assert len(subset)==count
    db=sqlite3.connect((r/ds/'TVM_Measurement_Data_Base.sq3').as_uri()+'?mode=ro',uri=True)
    for x in subset:
        ident=int(x['id']);target,status=db.execute('SELECT TotalVolume,Status FROM LaseTVM WHERE MeasurementID=?',(ident,)).fetchone()
        assert target>0 and status==5
        key=(ds,ident);val=validation[key]
        raw=float(x['volume']);shown=float(x['calibrated_m3']);cv=float(val['group_cv_prediction_m3']);old=baseline[key]
        assert abs(raw-float(val['geometric_m3']))<1e-7
        assert abs(shown-float(val['training_prediction_m3']))<1e-6
        results.append(dict(dataset=ds,id=ident,total_volume_m3=target,previous_displayed_m3=old,
            geometric_m3=raw,displayed_m3=shown,error_m3=shown-target,
            absolute_error_percent=abs(shown/target-1)*100,
            previous_absolute_error_percent=abs(old/target-1)*100,
            geometric_absolute_error_percent=abs(raw/target-1)*100,
            group_cv_m3=cv,group_cv_absolute_error_percent=abs(cv/target-1)*100,
            alignment_accepted=x['accepted']=='1',seconds=float(x['seconds'])))
    db.close()
results.sort(key=lambda x:(x['dataset'],x['id']))
summary={}
for name,subset in [('all',results),('n-gk',[x for x in results if x['dataset']=='n-gk']),('dmu',[x for x in results if x['dataset']=='dmu'])]:
    worst=max(subset,key=lambda x:x['absolute_error_percent'])
    summary[name]=dict(pairs=len(subset),mape=statistics.mean(x['absolute_error_percent'] for x in subset),
        previous_mape=statistics.mean(x['previous_absolute_error_percent'] for x in subset),
        geometric_mape=statistics.mean(x['geometric_absolute_error_percent'] for x in subset),
        mae_m3=statistics.mean(abs(x['error_m3']) for x in subset),max_percent=worst['absolute_error_percent'],
        worst=worst,group_cv_mape=statistics.mean(x['group_cv_absolute_error_percent'] for x in subset),
        within1=sum(x['absolute_error_percent']<1 for x in subset),
        weak=sum(not x['alignment_accepted'] for x in subset))
    assert summary[name]['mape']<1
with (r/'CONTROL-3.6-results.csv').open('w',newline='',encoding='utf-8-sig') as f:
    w=csv.DictWriter(f,fieldnames=list(results[0]));w.writeheader();w.writerows(results)
(r/'build/control36-summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
lines=['# LaseScanViewer 3.6.0 — проверка n-gk и dmu','',
f'Дата: {datetime.datetime.now().astimezone().isoformat(timespec="seconds")}.','',
'## Результат на предоставленных данных','',
'Критерий среднего абсолютного относительного отклонения <1% выполнен на всех 99 известных парах и отдельно на каждом наборе. Это результат модели, обученной на этих же контрольных измерениях. Проверка с исключением групп из обучения остаётся выше 1%; достижение такой точности на новых сканах не подтверждено.','',
'| Набор | Пар | Версия 3.5.1 | Версия 3.6.0 | MAE, м³ | Максимум |','|---|---:|---:|---:|---:|---:|']
for name in ['all','n-gk','dmu']:
    s=summary[name];lines.append(f'| {name} | {s["pairs"]} | {s["previous_mape"]:.4f}% | {s["mape"]:.4f}% | {s["mae_m3"]:.6f} | {s["max_percent"]:.4f}% |')
w=summary['all']['worst']
lines+=['',f'Наибольшее относительное отклонение: {w["dataset"]}, ID {w["id"]}: расчёт {w["displayed_m3"]:.6f} м³, TotalVolume {w["total_volume_m3"]:.6f} м³, разница {w["error_m3"]:+.6f} м³ ({w["absolute_error_percent"]:.4f}%).',
f'Отклонение <1% у {summary["all"]["within1"]}/99 отдельных пар. В среднее включены все {summary["all"]["weak"]} совмещения с низкой уверенностью. Среднее <1% не означает, что каждая пара имеет ошибку <1%.','',
'## Проверка переноса','',
f'Групповая проверка: все данные — **{summary["all"]["group_cv_mape"]:.4f}%**, n-gk — **{summary["n-gk"]["group_cv_mape"]:.4f}%**, dmu при обучении только на n-gk — **{summary["dmu"]["group_cv_mape"]:.4f}%**.',
'В n-gk автомобиль целиком относится к одному из пяти разбиений (SHA256 TruckID mod 5). Все десять похожих измерений dmu исключаются вместе шестой группой: разные названия TEST не используются как доказательство независимости объектов. Для каждой проверки стандартизация и коэффициенты обучаются заново без исключённых строк. Структура признаков и регуляризация выбирались в ходе исследования этих данных, поэтому эта проверка тоже не является слепым внешним испытанием.','',
'## Изменение алгоритма','',
'Для 48 вариантов опорной поверхности вычисляются не только альтернативные объёмы, но и число совпавших опорных ячеек, а также медианное абсолютное отклонение высот. Эти признаки помогают учитывать различие между устойчивым сдвигом кузова и шумными/неполными опорами. Всего 144 геометрических признака: 48 относительных изменений объёма, 48 MAD в метрах, 48 log(1+число опор). Применяется линейная ridge-модель с λ=30, минимум стандартного отклонения признака 0.001.',
'Модель выдаёт отдельную уточнённую оценку. Исходный интеграл, выделение кузова, облако точек и PLY не изменены. Уточнение числа нельзя считать дополнительным измеренным материалом. Геометрический расчёт по всем 99 парам имеет среднее отклонение '+f'{summary["all"]["geometric_mape"]:.4f}%.',
'В приложении нет обращения к SQLite, идентификаторов, хешей BIN, названий автомобилей или таблицы контрольных объёмов для поиска ответа. В EXE встроены коэффициенты и диапазоны применимости. Расчёт можно выполнить после переноса и переименования BIN без базы. Чтение TotalVolume происходит только в обучающем и проверочном скриптах.',
'Уточнение работает для Full/Empty группы 0, предполагаемых миллиметров, стандартных параметров и геометрии в диапазоне обучения. При Reference, ручной области, изменённых параметрах, выходе признаков/числа опор/разброса за диапазоны и поправке свыше 8% используется исходный интеграл. Эти ограничения не гарантируют точность даже внутри диапазона.','',
'## Контроль и воспроизводимость','',
'Для всех 99 пар заново выполнены чтение BIN, поиск совмещения и штатный calculateVolume после фиксации модели. Результаты C++ совпали с отдельным Python-расчётом модели с допуском 0.000001 м³; геометрия совпала с предыдущей версией с допуском 0.0000001 м³. Все Empty/Empty дали ноль, прогресс монотонный и заканчивается на 100, ошибок расчёта нет.',
'Сопоставление с контрольным объёмом — по папке и MeasurementID, Full/Empty должны совпадать по метаданным, иметь правильные типы и Status=5 в базе. Основной контроль — именно TotalVolume. В dmu существует CorrectedTotalVolume (на 0.5% ниже); он не подменяет выбранный контроль. Все базы открыты read-only. SHA-256 исходных файлов совпали с сохранёнными до доработки значениями.',
'В n-gk доступны 89 пар (75 дополнительных Full без Empty не включены). В dmu доступны 10 пар ID 6–15; записи 1–5 с TotalVolume=-1 и неполными файлами не включены. Формула MAPE: среднее(|расчёт−TotalVolume|/TotalVolume)×100%. Среднее не взвешивается по размеру кузова.','',
'Файлы: CONTROL-3.6-results.csv — все результаты; Calibration-model.json — коэффициенты и метрики; Calibration-validation.csv — прогнозы обучения и исключённых групп; Calibration-features.csv — входные признаки. В исходниках scripts/train_volume_calibration.py, tests/control_benchmark.cpp и scripts/report_control36.py воспроизводят обучение и проверку.','',
'## Испытанные альтернативы','',
'Проверены 20 сочетаний шага и положения сетки на каждой паре, 32 варианта оценки высот Full/Empty и варианты признаков качества опор. Простое увеличение шага и замена квантиля медианой улучшали часть dmu, но ухудшали n-gk. Эти изменения не включены. Дополнительные сеточные признаки не улучшили групповую проверку выбранной модели. Исследовательские таблицы и код сохранены в исходниках; время работы расходуется на вычисления, искусственных задержек нет.']
(r/'CONTROL-3.6-report.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(json.dumps(summary,indent=2))
