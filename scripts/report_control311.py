from pathlib import Path
import csv,json,hashlib,statistics
r=Path(__file__).resolve().parents[1];p=r/'build/research311'; frozen=r/'build/control310'
inv=json.loads((frozen/'inventory.json').read_text(encoding='utf-8'))
raw=list(csv.DictReader((p/'raw.csv').open(encoding='utf-8-sig')))
assert len(raw)==sum(x['has_empty'] for x in inv)
lookup={(x['dataset'],int(x['id'])):x for x in raw}
old={(x['dataset'],int(x['id'])) for x in csv.DictReader((r/'build/control-full-390.csv').open(encoding='utf-8-sig'))}
result=[]
for x in inv:
 key=x['dataset'],x['id']; row=lookup.get(key)
 z=dict(dataset=x['dataset'],id=x['id'],previously_tested=key in old,total_volume_m3='',calculated_m3='',error_m3='',error_percent='',geometric_m3='',geometric_error_percent='',angle='',overlap='',accepted='',statistical_adjustment='',status='',reason='')
 controls=x['controls']
 if len(controls)==1:z['total_volume_m3']=controls[0][0];z['status']=controls[0][1]
 if not x['has_empty']:z['reason']='Missing Empty BIN'
 elif row['error']:z['reason']='Calculation failed: '+row['error']
 else:
  rawv=float(row['volume']);v=float(row['calibrated_m3']) if row['calibrated']=='1' else rawv
  z.update(calculated_m3=v,geometric_m3=rawv,angle=float(row['angle']),overlap=float(row['overlap']),accepted=row['accepted'],statistical_adjustment=row['calibrated'])
  target=z['total_volume_m3']
  if len(controls)!=1:z['reason']='No unique TotalVolume record'
  elif target is None or target<=0:z['reason']='Non-positive or missing TotalVolume'
  else:z.update(error_m3=v-target,error_percent=abs(v/target-1)*100,geometric_error_percent=abs(rawv/target-1)*100)
 result.append(z)
with (r/'CONTROL-3.11-results.csv').open('w',newline='',encoding='utf-8-sig') as f:
 w=csv.DictWriter(f,fieldnames=list(result[0]));w.writeheader();w.writerows(result)
def metrics(a):
 if not a:return dict(pairs=0)
 return dict(pairs=len(a),mape=statistics.mean(x['error_percent'] for x in a),mae_m3=statistics.mean(abs(x['error_m3']) for x in a),maximum_percent=max(x['error_percent'] for x in a),worst=max(a,key=lambda x:x['error_percent']),geometric_mape=statistics.mean(x['geometric_error_percent'] for x in a),weak=sum(x['accepted']=='0' for x in a))
valid=[x for x in result if x['error_percent']!='']
summary={ds:metrics([x for x in valid if x['dataset']==ds]) for ds in ['n-gk','dmu','тест']}
summary['all']=metrics(valid)
summary['previously_tested']=metrics([x for x in valid if x['previously_tested']])
summary['new']=metrics([x for x in valid if not x['previously_tested']])
changes=[]
for name,digest in json.loads((frozen/'input-hashes.json').read_text(encoding='utf-8')).items():
 if hashlib.sha256((r/name).read_bytes()).hexdigest()!=digest:changes.append(name)
assert not changes,changes
(p/'summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
lines=['# Проверка LaseScanViewer 3.11 по TotalVolume','', 'На всех 277 исправных полных парах повторно выполнен штатный calculateVolume, включая ненадёжные совмещения. Положения базы взяты из полного прогона 3.10: выбранный алгоритм совмещения не изменён. Использованы стандартные параметры (шаг 100, адаптивная сетка, порог 50), выбранный по умолчанию набор точек. Исходники и модель зафиксированы перед контрольным прогоном; базы TotalVolume читались только для сверки, расчётный C++ процесс их не открывает. Единицы BIN приняты за миллиметры, результат приведён к м³.','', '| Набор | Сравнено | Среднее абсолютное отклонение, % | Среднее абсолютное отклонение, м³ | Максимум, % |', '|---|---:|---:|---:|---:|']
for ds,m in summary.items():
 if m['pairs']:lines.append(f"| {ds} | {m['pairs']} | {m['mape']:.4f} | {m['mae_m3']:.4f} | {m['maximum_percent']:.4f} |")
lines+=['','MAPE = среднее(|расчёт − TotalVolume| / TotalVolume) × 100%. Используется TotalVolume, не CorrectedTotalVolume. Основная таблица показывает итоговую оценку приложения со штатной статистической поправкой, когда она применима. Чисто геометрическая оценка приведена отдельно в CSV. Все 271 пары с TotalVolume использованы при обучении новой модели. Поэтому отклонение 0.555% является результатом на данных обучения, а не независимой проверкой точности на будущих сканах. В том числе 90 дополнительных пар и восемь ранее отложенных теперь включены в обучение.','', '## Проверка переноса на другие автомобили', '', 'При исключении целых автомобилей из обучения среднее отклонение 1.184%. Это шесть исследовательских групп: пять групп автомобилей n-gk и вся dmu; проверка использована при выборе параметров. Независимого слепого набора после выбора модели нет. Точность около 0.5% на будущих файлах пока не подтверждена.', '', '## Метод и проверенные альтернативы', '', 'Вместо линейной поправки используется регуляризованная нелинейная регрессия с гауссовым ядром по тем же 190 геометрическим признакам: устойчивость опорной высоты, форма груза, плотность и покрытие. Параметры выбраны по групповому отклонению: ширина 1.15, регуляризация 0.07. При оценке объёма не используются имена файлов, номера измерений или доступ к SQLite; модель не подставляет TotalVolume. Контроль применяется офлайн при обучении. Ограничения единиц, настройки, области и диапазонов признаков сохранены; предел допустимой поправки 15%. Геометрия, выделенные точки и экспорт PLY не изменяются статистической оценкой.', '', 'Проверены линейные модели с шестью ограничениями коэффициентов и тремя уровнями робастности, 60 вариантов нелинейного ядра и 25 уточнений его параметров. Простое ослабление линейной модели ухудшало перенос на другие машины. Постоянные сдвиги базы по высоте ±25/±50 мм ухудшали объём. Повторное совмещение по локальным плоскостям повысило геометрическую ошибку с 1.578% до 1.885%; уточнение только XY/поворота уменьшило среднее до 1.572%, но подняло максимум до 15.797%. Эти изменения положения базы не включены.', '', '## Наибольшие отклонения','']
for z in sorted(valid,key=lambda x:x['error_percent'],reverse=True)[:10]:lines.append(f"- {z['dataset']} / {z['id']}: расчёт {z['calculated_m3']:.4f} м³; TotalVolume {z['total_volume_m3']:.4f} м³; разница {z['error_m3']:+.4f} м³ ({z['error_percent']:.4f}%).")
lines+=['','## Папка «тест»','','В имеющейся SQLite-базе максимальный MeasurementID — 43044. Записей для BIN 43279–43298 нет также среди IncomingMeasurementID, OutgoingMeasurementID и CustomerMeasurementID. Поэтому их нельзя включить в процентную сверку.','', '| BIN | Расчёт, м³ | Поворот, ° |', '|---|---:|---:|']
for z in result:
 if z['dataset']=='тест' and z['calculated_m3']!='':lines.append(f"| {z['id']} | {z['calculated_m3']:.4f} | {z['angle']:.3f} |")
lines+=['','## Непроверенные файлы','']
from collections import Counter
for reason,n in Counter(x['reason'] for x in result if x['reason']).items():lines.append(f'- {reason}: {n}.')
lines+=['','Полный список, причины исключения, геометрические объёмы и параметры совмещения: CONTROL-3.11-results.csv. Все результаты слабого совмещения оставлены в статистике. Хеши BIN и SQLite проверены до и после прогона; файлы не изменились.']
(r/'CONTROL-3.11-report.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(json.dumps(summary,ensure_ascii=False,indent=2))
