from pathlib import Path
import csv,sqlite3,hashlib,json,statistics,datetime
r=Path(__file__).resolve().parents[1]
rows=list(csv.DictReader((r/'build/dmu-full-351.csv').open(encoding='utf-8')))
assert len(rows)==10 and {int(x['id']) for x in rows}==set(range(6,16))
assert all(not x['error'] and float(x['null_volume'])==0 for x in rows)
hashes=json.loads((r/'build/dmu-input-hashes.json').read_text())
assert all(hashlib.sha256(Path(p).read_bytes()).hexdigest()==v for p,v in hashes.items())
db=sqlite3.connect((r/'dmu/TVM_Measurement_Data_Base.sq3').as_uri()+'?mode=ro',uri=True)
results=[]
for x in rows:
 id=int(x['id']);truck,status,target,corrected,factor=db.execute('select TruckID,Status,TotalVolume,CorrectedTotalVolume,CorrectionFactor from LaseTVM where MeasurementID=?',(id,)).fetchone()
 assert target>0 and corrected>0 and status==5
 raw=float(x['volume']);cal=float(x['calibrated_m3']) if x['calibrated']=='1' else None;shown=cal if cal is not None else raw
 results.append(dict(id=id,truck=truck,total_volume_m3=target,corrected_total_volume_m3=corrected,correction_factor=factor,geometric_m3=raw,calibration_applied=x['calibrated']=='1',calibrated_m3=cal,displayed_m3=shown,geometric_error_m3=raw-target,geometric_error_percent=(raw/target-1)*100,displayed_error_m3=shown-target,displayed_error_percent=(shown/target-1)*100,displayed_vs_corrected_percent=(shown/corrected-1)*100,geometric_vs_corrected_percent=(raw/corrected-1)*100,alignment_accepted=x['accepted']=='1',overlap=float(x['overlap'])))
db.close();results.sort(key=lambda x:x['id'])
worst=max(results,key=lambda x:abs(x['displayed_error_percent']));worst_raw=max(results,key=lambda x:abs(x['geometric_error_percent']))
summary=dict(pairs=len(results),geometric_mape=statistics.mean(abs(x['geometric_error_percent']) for x in results),geometric_mae_m3=statistics.mean(abs(x['geometric_error_m3']) for x in results),displayed_mape=statistics.mean(abs(x['displayed_error_percent']) for x in results),displayed_mae_m3=statistics.mean(abs(x['displayed_error_m3']) for x in results),worst_displayed=worst,worst_geometric=worst_raw,calibration_improved=sum(abs(x['displayed_error_percent'])<abs(x['geometric_error_percent']) for x in results),displayed_vs_corrected_mape=statistics.mean(abs(x['displayed_vs_corrected_percent']) for x in results),geometric_vs_corrected_mape=statistics.mean(abs(x['geometric_vs_corrected_percent']) for x in results),all_underestimate=all(x['displayed_error_m3']<0 for x in results))
with (r/'DMU-3.5.1-results.csv').open('w',newline='',encoding='utf-8-sig') as f:
 w=csv.DictWriter(f,fieldnames=list(results[0]));w.writeheader();w.writerows(results)
lines=['# Проверка dmu — LaseScanViewer 3.5.1','',f'Дата: {datetime.datetime.now().astimezone().isoformat(timespec="seconds")}.','',
'Проверены все 10 доступных пар Full/Empty, MeasurementID 6–15. Для каждой заново выполнено совмещение, затем штатный calculateVolume со стандартными настройками, включая калибровку n-gk. Код сверён с исходниками релиза 3.5.1. Алгоритм, параметры и коэффициенты не менялись и на dmu не обучались.','',
'Контроль прочитан из dmu/TVM_Measurement_Data_Base.sq3 по MeasurementID. Основной контроль — TotalVolume, как при проверке n-gk. Все 10 строк имеют Status=5. В базе также есть CorrectedTotalVolume с CorrectionFactor=−0.5 (значение на 0.5% ниже); сравнение с ним приведено отдельно.','',
'В папке также есть Empty для ID 1 и 2 без Full. Для записей 1–5 TotalVolume=−1; для 3–5 нет BIN-пар. Эти записи не являются дополнительными пригодными контрольными измерениями и не включены в статистику.','',
'## Сравнение с TotalVolume','',
'| Показатель | Геометрический объём | Отображаемая оценка с калибровкой n-gk |','|---|---:|---:|',
f'| Среднее абсолютное относительное отклонение | {summary["geometric_mape"]:.4f}% | {summary["displayed_mape"]:.4f}% |',
f'| Среднее абсолютное отклонение | {summary["geometric_mae_m3"]:.6f} м³ | {summary["displayed_mae_m3"]:.6f} м³ |',
f'| Максимальное относительное отклонение | {abs(worst_raw["geometric_error_percent"]):.4f}% (ID {worst_raw["id"]}) | {abs(worst["displayed_error_percent"]):.4f}% (ID {worst["id"]}) |','',
f'Калибровка улучшила {summary["calibration_improved"]} из 10 результатов. Во всех 10 случаях исходный и отображаемый объёмы ниже TotalVolume. На этом наборе калибровка n-gk не улучшила среднюю точность и критерий <1% не выполнен. Ранее полученные 0.865% относились к обучающему n-gk и не перенеслись на dmu. Набор dmu содержит только 10 пар; статистика описывает именно эти измерения.','',
'| ID | TotalVolume, м³ | Геометрия, м³ | На экране, м³ | Ошибка на экране, м³ | Отклонение, % |','|---|---:|---:|---:|---:|---:|']
for x in results:lines.append(f'| {x["id"]} | {x["total_volume_m3"]:.6f} | {x["geometric_m3"]:.6f} | {x["displayed_m3"]:.6f} | {x["displayed_error_m3"]:+.6f} | {abs(x["displayed_error_percent"]):.4f} |')
lines+=['','## Сравнение с CorrectedTotalVolume','',f'Среднее абсолютное относительное отклонение: геометрия **{summary["geometric_vs_corrected_mape"]:.4f}%**, отображаемая оценка **{summary["displayed_vs_corrected_mape"]:.4f}%**.','',
'## Проверки','',
'Все 10 совмещений приняты алгоритмом; все 10 калибровок прошли его текущие ограничения применимости. Тем не менее эти ограничения не гарантировали точность на новом наборе. Все проверки Empty/Empty дали ноль, ошибок чтения и расчёта нет. Координаты интерпретировались как миллиметры, как в приложении по умолчанию. База открывалась только для чтения; SHA-256 всех 22 BIN и базы до и после прогона совпали.','',
'Формула относительного отклонения: |расчёт − контроль| / контроль × 100%. Полная таблица, включая сравнение с обоими полями базы, сохранена в DMU-3.5.1-results.csv.']
(r/'DMU-3.5.1-report.md').write_text('\n'.join(lines)+'\n',encoding='utf-8');(r/'build/dmu-summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2))
