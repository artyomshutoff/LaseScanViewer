"""Evaluate the frozen 3.7 model on additional pairs, without fitting anything."""
from pathlib import Path
import csv, json, hashlib, sqlite3, statistics

r = Path(__file__).resolve().parents[1]
for manifest in ['build/model37-frozen-before-new-data.json', 'build/new-data-input-hashes.json']:
    for path, digest in json.loads((r/manifest).read_text()).items():
        assert hashlib.sha256((r/path).read_bytes()).hexdigest() == digest, path
keys = {(z['dataset'], int(z['id'])) for z in csv.DictReader((r/'build/control37-new.csv').open())}
assert len(keys) == 33
runs = {}
for version in ['360', '370']:
    rows = list(csv.DictReader((r/f'build/new-data-{version}.csv').open(encoding='utf-8')))
    assert len(rows) == 33
    runs[version] = {(z['dataset'], int(z['id'])): z for z in rows}
    assert set(runs[version]) == keys
    assert all(not z['error'] and float(z['null_volume']) == 0 for z in rows)
known = list(csv.DictReader((r/'build/volume37-validation.csv').open()))
trucks = {(z['dataset'], z['truck']) for z in known}
db = sqlite3.connect((r/'n-gk/TVM_Measurement_Data_Base.sq3').as_uri()+'?mode=ro', uri=True)
results = []
for key in sorted(keys):
    ds, ident = key
    target, status, truck = db.execute('SELECT TotalVolume,Status,TruckID FROM LaseTVM WHERE MeasurementID=?', (ident,)).fetchone()
    assert target > 0 and status == 5, key
    a, b = runs['360'][key], runs['370'][key]
    old, new, old_raw, new_raw = map(float, [a['displayed'], b['displayed'], a['geometric'], b['geometric']])
    results.append(dict(dataset=ds, id=ident, truck=truck, seen_vehicle=(ds, truck) in trucks,
        total_volume_m3=target, previous_displayed_m3=old, displayed_m3=new,
        previous_geometric_m3=old_raw, geometric_m3=new_raw,
        previous_error_percent=abs(old/target-1)*100, error_percent=abs(new/target-1)*100,
        previous_geometric_error_percent=abs(old_raw/target-1)*100, geometric_error_percent=abs(new_raw/target-1)*100,
        previous_calibrated=a['calibrated']=='1', calibrated=b['calibrated']=='1',
        alignment_accepted=b['accepted']=='1', error_m3=new-target, note=b['note']))
db.close()
def metrics(rows):
    return dict(pairs=len(rows), previous_mape=statistics.mean(z['previous_error_percent'] for z in rows),
        mape=statistics.mean(z['error_percent'] for z in rows),
        previous_geometric_mape=statistics.mean(z['previous_geometric_error_percent'] for z in rows),
        geometric_mape=statistics.mean(z['geometric_error_percent'] for z in rows),
        previous_max=max(z['previous_error_percent'] for z in rows), worst=max(rows,key=lambda z:z['error_percent']),
        mae_m3=statistics.mean(abs(z['error_m3']) for z in rows),
        improved=sum(z['error_percent']<z['previous_error_percent'] for z in rows),
        calibrated=sum(z['calibrated'] for z in rows),previous_calibrated=sum(z['previous_calibrated'] for z in rows),
        within1=sum(z['error_percent']<1 for z in rows),weak=sum(not z['alignment_accepted'] for z in rows))
summary={'all':metrics(results)}
for name, flag in [('seen_vehicles', True), ('unseen_vehicles', False)]:
    subset=[z for z in results if z['seen_vehicle']==flag]
    if subset:summary[name]=metrics(subset)
with (r/'NEW-3.7-results.csv').open('w', newline='', encoding='utf-8-sig') as f:
    w=csv.DictWriter(f,fieldnames=list(results[0]));w.writeheader();w.writerows(results)
(r/'build/new37-summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
s=summary['all']; worst=s['worst']
lines=['# LaseScanViewer 3.7 — новые пары после фиксации модели','',
    'Проверены 33 дополнительные пары n-gk из зафиксированного списка control37-new.csv. Копирование данных пользователем продолжается; это не полный перечень новых файлов. Все 33 пары имеют Status=5 и положительный TotalVolume. Ошибок чтения/расчёта и исключённых результатов нет.', '',
    'Код модели и локального совмещения, а также 66 BIN зафиксированы SHA256 до сверки новых контрольных значений. Версии 3.6 и 3.7 заново выполнили штатное совмещение и расчёт без доступа к базе. Контроль прочитан только после завершения прогнозов; по этим 33 парам модель не переобучалась. Empty/Empty дал ноль во всех случаях.','',
    '| Выборка | Пар | MAPE 3.6 | MAPE 3.7 | Геометрия 3.6 | Геометрия 3.7 |',
    '|---|---:|---:|---:|---:|---:|']
for name,x in summary.items():
    lines.append(f'| {name} | {x["pairs"]} | {x["previous_mape"]:.4f}% | {x["mape"]:.4f}% | {x["previous_geometric_mape"]:.4f}% | {x["geometric_mape"]:.4f}% |')
lines+=['',f'Максимум 3.6: {s["previous_max"]:.4f}%; максимум 3.7: {worst["error_percent"]:.4f}%, ID {worst["id"]}. Расчёт {worst["displayed_m3"]:.6f} м³, контроль {worst["total_volume_m3"]:.6f} м³, разница {worst["error_m3"]:+.6f} м³.',
    f'Среднее абсолютное отклонение: {s["mae_m3"]:.6f} м³. Улучшены {s["improved"]}/33 пары; ошибка меньше 1% у {s["within1"]}/33. Слабых совмещений {s["weak"]}; они включены в статистику.',
    f'Статистическая поправка применена в 3.6 к {s["previous_calibrated"]}/33, в 3.7 к {s["calibrated"]}/33. В остальных случаях программа показывает геометрический объём, такие результаты также учтены.', '',
    'seen_vehicles — автомобили, встречавшиеся в исходных 99 парах; unseen_vehicles — отсутствовавшие в них. Новые измерения известных автомобилей не равнозначны проверке на новых автомобилях или другом сканере. Набор небольшой и из той же папки n-gk; его результат не гарантирует точность всех будущих сканов.',
    'MAPE = среднее(|расчёт−TotalVolume|/TotalVolume)×100%. Каждая пара имеет одинаковый вес. Детали — NEW-3.7-results.csv; исходные прогнозы — new-data-360.csv и new-data-370.csv.']
(r/'NEW-3.7-report.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(json.dumps(summary,ensure_ascii=False,indent=2))
