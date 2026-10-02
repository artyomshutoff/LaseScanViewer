"""Compare fixed-pose full volume replays with independent TotalVolume records."""
from pathlib import Path
import csv,json,math,statistics,hashlib,zipfile
r=Path(__file__).resolve().parents[1];p=r/'build/control332'
def load(path):return {(x['dataset'],int(x['id'])):x for x in csv.DictReader(path.open(encoding='utf-8'))}
original=load(p/'before-all.csv');before=load(p/'replay-before/raw.csv');after=load(p/'replay-after/raw.csv')
inv=json.loads((p/'inventory.json').read_text(encoding='utf-8'))
assert len(before)==len(after)==len(inv)
assert set(original)==set(before)==set(after)
fb=load(p/'replay-before/features.csv');fa=load(p/'replay-after/features.csv')
full_new=load(p/'full-after-new/raw.csv');full_features=load(p/'full-after-new/features.csv')
assert len(full_new)==30
for key,row in full_new.items():
    assert all(row[k]==after[key][k] for k in row if k!='seconds'),(key,'fresh full alignment differs from pose replay')
    assert full_features[key]==fa[key],(key,'fresh full features differ from pose replay')
rows=[];errors=[];without=[];changed=[]
for item in inv:
    key=(item['dataset'],item['id']);a=before[key];b=after[key]
    assert all(a[k]==original[key][k] for k in a if k!='seconds'),(key,'replay differs from actual 3.31 run')
    fields=[k for k in a if k!='seconds' and a[k]!=b[k]]
    if fields: changed.append(dict(dataset=key[0],id=key[1],fields=fields))
    if key!=('n-gk new',14387):
        assert not fields,(key,fields)
        if not a['error']:assert fb[key]==fa[key],(key,'feature mismatch')
    if b['error']:
        assert b['error']==a['error'];errors.append(dict(dataset=key[0],id=key[1],reason=b['error']));continue
    for k in ['angle','dx','dy','dz','overlap','rms','accepted','structure_refined','bed_shift','bed_spread','bed_variants']:
        assert a[k]==b[k],(key,k)
    controls=item['controls']
    if len(controls)!=1 or controls[0][3]!=5 or not isinstance(controls[0][2],(int,float)) or not math.isfinite(controls[0][2]) or controls[0][2]<=0 or str(controls[0][1]).strip().upper()!=b['label'].strip().upper():
        without.append(dict(dataset=key[0],id=key[1],reason='No unique completed matching positive TotalVolume'));continue
    target=controls[0][2];bv=float(a['calculated_m3']);av=float(b['calculated_m3'])
    rows.append(dict(dataset=key[0],id=key[1],truck=b['label'],control_m3=target,before_m3=bv,after_m3=av,
                     before_error_percent=abs(bv/target-1)*100,after_error_percent=abs(av/target-1)*100,
                     after_difference_m3=av-target,geometric_m3=float(b['geometric_m3']),
                     geometric_error_percent=abs(float(b['geometric_m3'])/target-1)*100,
                     model_used=int(b['calibrated']),accepted=int(b['accepted']),changed=bool(fields)))
assert len(changed)==1 and changed[0]['id']==14387
for name,digest in json.loads((p/'input-hashes.json').read_text(encoding='utf-8')).items():
    assert hashlib.sha256((r/name).read_bytes()).hexdigest()==digest,name
with zipfile.ZipFile(r/'releases/LaseScanViewer-3.31.0/LaseScanViewer-source.zip') as archive:
    for name in ['registration.hpp','structural_alignment.hpp','rim_alignment.hpp','bed_alignment.hpp','registration_focus.hpp','calibration_kernel_model.hpp','kernel_estimate.hpp','calibration_features.hpp','cargo.hpp','body_filter.hpp']:
        assert (r/'src'/name).read_bytes()==archive.read('src/'+name),name
def metrics(values):
    if not values:return dict(count=0)
    return dict(count=len(values),before_mape=statistics.mean(x['before_error_percent'] for x in values),
                after_mape=statistics.mean(x['after_error_percent'] for x in values),
                before_max=max(x['before_error_percent'] for x in values),after_max=max(x['after_error_percent'] for x in values),
                mae_m3=statistics.mean(abs(x['after_difference_m3']) for x in values),
                geometric_mape=statistics.mean(x['geometric_error_percent'] for x in values),
                worsened=sum(x['after_error_percent']>x['before_error_percent']+1e-10 for x in values))
datasets=['n-gk','dmu','тест','n-gk 2','n-gk new']
summary={ds:metrics([x for x in rows if x['dataset']==ds]) for ds in datasets}
summary['all']=metrics(rows)
summary['verification']=dict(attempts=len(inv),successful=len(inv)-len(errors),controls=len(rows),errors=errors,without_control=without,changes=changed,
    method='Fresh 3.31/3.32 volume calculations replaying immutable 3.31 poses; replay baseline exactly matches original complete runs; geometric features exact on 379 unchanged successful pairs; registration and models byte-identical; SQ3 read only outside C++',
    limitation='14387 used for algorithm development. Historical model training data remain in aggregate statistics. No guarantee of accuracy or blind holdout claim.')
(p/'summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
with (r/'CONTROL-3.32-results.csv').open('w',newline='',encoding='utf-8-sig') as file:
    writer=csv.DictWriter(file,fieldnames=rows[0].keys());writer.writeheader();writer.writerows(rows)
counts=json.loads((p/'counts.json').read_text(encoding='utf-8'))
lines=['# LaseScanViewer 3.32 — проверка n-gk new','',
 '## Данные и методика','',
 f"n-gk new: {counts['n-gk new']['full']} Full, {counts['n-gk new']['pairs']} доступных пар Full/Empty и 44 Full без парного Empty. Все 30 пар сопоставлены с уникальными завершёнными записями собственной TVM_Measurement_Data_Base.sq3: Status=5, положительный TotalVolume и совпадающая метка автомобиля. Файлы без Empty не включаются в процент отклонения.",
 '',f"Всего {len(inv)} пар, {len(inv)-len(errors)} успешных расчётов и {len(rows)} сравнений с контролем. Прежний повреждённый BIN 13955 сохраняет ошибку чтения. Для шести пар «тест» подходящих контрольных записей нет. Выбросы и ненадёжные совмещения включены в статистику.",'',
 'Параметры: координаты в мм, CPU, исходный шаг 100, адаптация сетки, порог 50, очистка кузова, крупнейшая область, встроенная оценочная модель включена. Все новые пары рассчитаны полностью с поиском совмещения с нуля в 3.31 и 3.32. Для полной повторной проверки объёма обе версии заново читают все BIN и используют неизменные найденные положения базы. Повтор 3.31 точно совпал с исходными полными расчётами, а повтор 3.32 совпал с новым полным прогоном 30 пар; алгоритмы совмещения и коэффициенты модели побайтово сохранены. SQ3 используется отдельным проверяющим скриптом, а не расчётным C++ кодом. SHA256 всех использованных BIN/SQ3 перепроверены.','',
 '## Результаты','', '| Набор | Контрольных пар | Среднее абсолютное отклонение до → после, % | Максимум до → после, % | Ухудшений |', '|---|---:|---:|---:|---:|']
for ds in ['n-gk new','n-gk','dmu','n-gk 2','all']:
    m=summary[ds];lines.append(f"| {'Все' if ds=='all' else ds} | {m['count']} | {m['before_mape']:.4f} → {m['after_mape']:.4f} | {m['before_max']:.4f} → {m['after_max']:.4f} | {m['worsened']} |")
case=next(x for x in rows if x['dataset']=='n-gk new' and x['id']==14387)
lines+=['','## Изменение алгоритма','',
 'При уточнении сетки сравниваются исходный шаг, 0.75 исходного шага и, при резкой потере объёма, 1.25 исходного шага. Если мелкая сетка теряет больше четверти объёма, а исходная и более крупная согласуются в пределах четверти исходного объёма, сохраняется исходный шаг. Это защита от дробления связной области на малые компоненты; выбирается исходный интеграл, а не максимальный вариант. Результат помечается предварительным, выводится предупреждение. Модельная поправка для этого случая отключена. Контрольные объёмы, ID и метки автомобилей в правило не входят.','',
 f"На 14387 шаг 75 терял большую часть груза: {case['before_m3']:.6f} м³. Исходный шаг 100 даёт {case['after_m3']:.6f} м³, а шаг 125 — 8.141316 м³. Контроль: {case['control_m3']:.6f} м³; ошибка {case['before_error_percent']:.4f}% → {case['after_error_percent']:.4f}%. Поворот и сдвиг не изменены, совмещение остаётся ненадёжным. Изменены только объём/сетка этого случая; все 350 прежних исправных пар и остальные 29 новых совпали по числам и 190 признакам.",'',
 '## Проверенные альтернативы','',
 '- Расширенный поиск совмещения по крутым поверхностям: для 14387 жёсткое совмещение не устраняет искажение Empty. Этот эксперимент в программу не добавлен.','- Усреднение фаз сетки немного улучшило новые пары, но ухудшило прежние; не внедрено.','- Среднее/медиана высот и глобальная смена шага также ухудшили прежние данные; не внедрены.','- Переобучение геометрической модели уменьшило ошибку обучения, но не подтвердило улучшение при исключении целых автомобилей. Коэффициенты исходной модели сохранены.','',
 '## Ограничения','',
 'Это улучшение разработки на имеющихся данных. Скан 14387 использован при разработке защиты; итоговая проверка не является слепым испытанием. Старые контрольные пары частично участвовали в обучении встроенной модели. Основные оставшиеся отклонения на новых данных: 14375 — 12.739%, 14372 — 8.137%, 14365 — 6.189%. По данным невозможно утверждать, что ошибка во всех случаях вызвана только алгоритмом. Сильно искажённый или неполный Empty не восстанавливает истинную геометрию кузова. Прежний максимум 42.984% на 12837 остаётся в общей статистике.','',
 '## Воспроизведение','',
 'scripts/inventory332.py фиксирует пары, SHA256 и контрольные записи. tests/control327_benchmark.cpp принимает TSV с путями BIN, выходной каталог и число работников. Макрос VOLUME_CACHE_332 и дополнительный аргумент CSV включают проверочный повтор по сохранённым положениям; PERF_332_BASELINE использует снимок src из архива 3.31 в build/control332/baseline/src. scripts/report_control332.py сравнивает результаты и TotalVolume. tests/refinement332_test.cpp проверяет защитные условия, настоящий 14387, предупреждение, прогресс и отключение адаптации. Исходные сканы и SQ3 в поставку не включаются.','']
(r/'CONTROL-3.32-report.md').write_text('\n'.join(lines),encoding='utf-8')
print(json.dumps({ds:summary[ds] for ds in ['n-gk new','all']},ensure_ascii=False,indent=2))
print('PASS full volume replay, 379 unchanged successful cases, one improvement, unchanged registration/model and input SHA256')
