"""Assert unchanged numeric outputs/features and compare independent SQ3 totals."""
from pathlib import Path
import csv,json,statistics,hashlib,math
r=Path(__file__).resolve().parents[1];p=r/'build/perf331';audit=p/'audit'
def load(path):return {(x['dataset'],int(x['id'])):x for x in csv.DictReader(path.open(encoding='utf-8'))} if path.exists() else {}
old=load(audit/'baseline-cached.csv');new=load(audit/'optimized/raw.csv')
if (audit/'baseline-fresh/raw.csv').exists():old.update(load(audit/'baseline-fresh/raw.csv'))
inv=json.loads((audit/'inventory.json').read_text(encoding='utf-8'))
assert len(new)==len(inv)
beforeFeatures={}
for folder in ['candidate-old','candidate-new']:
 beforeFeatures.update(load(r/'build/control327'/folder/'features.csv'))
if (audit/'baseline-fresh/features.csv').exists():beforeFeatures.update(load(audit/'baseline-fresh/features.csv'))
afterFeatures=load(audit/'optimized/features.csv')
rows=[];errors=[];without=[];changed=[]
for entry in inv:
 key=(entry['dataset'],entry['id']);a=old[key];b=new[key]
 if a['error'] or b['error']:
  assert a['error']==b['error'],(key,a['error'],b['error'])
  errors.append(dict(dataset=key[0],id=key[1],reason=b['error']));continue
 for field in a:
  if field not in ['seconds']:assert a[field]==b[field],(key,field,a[field],b[field])
 assert beforeFeatures[key]==afterFeatures[key],(key,'190 feature mismatch')
 controls=entry['controls']
 if len(controls)!=1 or controls[0][3]!=5 or not isinstance(controls[0][2],(int,float)) or not math.isfinite(controls[0][2]) or controls[0][2]<=0 or str(controls[0][1]).strip().upper()!=b['label'].strip().upper():
  without.append(dict(dataset=key[0],id=key[1],reason='No unique completed matching positive TotalVolume'));continue
 t=controls[0][2];v=float(b['calculated_m3']);g=float(b['geometric_m3'])
 rows.append(dict(dataset=key[0],id=key[1],truck=b['label'],total_volume_m3=t,calculated_m3=v,geometric_m3=g,difference_m3=v-t,absolute_percent=abs(v/t-1)*100,geometric_percent=abs(g/t-1)*100,exact_baseline_match=True,accepted=int(b['accepted'])))
for name,data in [('results',rows),('errors',errors),('without-control',without)]:
 if data:
  with (audit/(name+'.csv')).open('w',newline='',encoding='utf-8-sig') as f:
   w=csv.DictWriter(f,fieldnames=data[0].keys());w.writeheader();w.writerows(data)
def metrics(a):
 return dict(count=len(a),mape=statistics.mean(x['absolute_percent'] for x in a),mae_m3=statistics.mean(abs(x['difference_m3']) for x in a),max_percent=max(x['absolute_percent'] for x in a),geometric_mape=statistics.mean(x['geometric_percent'] for x in a),changed=0) if a else dict(count=0)
summary={ds:metrics([x for x in rows if x['dataset']==ds]) for ds in ['n-gk','dmu','тест','n-gk 2']}
summary['all']=metrics(rows)
summary['verification']=dict(attempts=len(inv),successful=len(new)-len(errors),controls=len(rows),without_control=without,errors=errors,exact='All poses, volumes, sensitivity ranges, cell counts, exclusion counts, flags and 190 descriptor values identical',baseline='Fresh 3.30.2 runs and/or SHA256-verified historical runs with identical transitive numerical headers; same input BIN hashes',models='Unchanged; no retraining; benchmark executable never reads SQ3')
for file,digest in json.loads((audit/'input-hashes.json').read_text(encoding='utf-8')).items():assert hashlib.sha256((r/file).read_bytes()).hexdigest()==digest,file
(audit/'summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
timings=json.loads((p/'timing-summary.json').read_text())
lines=['# LaseScanViewer 3.31 — ускорение без изменения результатов','',
'Сравнение с 3.30.2 на AMD Ryzen 5 4600H (6 ядер / 12 потоков), x64 Clang -O2. Для каждой из шести пар — три последовательных запуска каждой версии; порядок версий чередовался. В таблице медиана времени чтения, построения поверхности, совмещения и расчёта. Во время этих замеров полный аудит и другие тяжёлые тесты не выполнялись. Это локальные измерения, а не гарантия скорости на другом компьютере.','',
'## Изменения','',
'- Итеративный обход KD-дерева с фиксированным стеком; порядок обхода и строгие сравнения при равных расстояниях сохранены.','- Уточнение финальных кандидатов и независимых смещений по стенкам выполняется параллельно, результаты объединяются в прежнем порядке.','- Диагностические варианты объёма не строят ненужные сетки отображения и облака. Все маски, интегралы и проверки сохраняются.','- Независимые варианты высоты опор распределяются на четыре потока.','- Повторно используются рабочие буферы уточнения, значения sin/cos и последняя найденная ячейка. Допуски поверхности груза вычисляются один раз для каждой ячейки вместо каждого её скана.','- SSE2-ассемблер дескрипторов сохранён. Новых требований AVX/AVX2 нет. Дополнительная хеш-таблица проверена, но оказалась медленнее на трёх измеренных парах и не включена.','',
'## Время','', '| Пара | Полный цикл до → после, с | Сокращение | Совмещение до → после, с | Объём до → после, с |', '|---|---:|---:|---:|---:|']
for x in timings:
 a=x['total'];b=x['align'];v=x['volume'];lines.append(f"| {x['case']} | {a['before']:.2f} → {a['after']:.2f} | {a['reduction_percent']:.1f}% | {b['before']:.2f} → {b['after']:.2f} | {v['before']:.2f} → {v['after']:.2f} |")
lines+=['','На всех шести парах точно совпали поворот, XYZ-сдвиги, RMS, покрытие, признаки надёжности, объёмы, диапазоны чувствительности, ячейки и хеш геометрии/облака точек. Проверено 100 000 запросов ближайших соседей с дубликатами и равными расстояниями на x64 и x86.','', 'Проверены обе портативные сборки: калькулятор, единицы, наложение, область, независимые слои, вода/тент, экспорт и сохранение камеры. Фон просмотра чёрный в обеих темах; шесть немедленных переключений цвета заголовка проходят без изменения размера, камеры или фокуса. CPU и GPU на скане 13785 дали одинаковые совмещение и объём. На GTX 1650 Ti GPU в этой проверке медленнее CPU; автоматического обещания ускорения от выбора GPU нет.','', '## Полная проверка TotalVolume','', '| Набор | Сравнений | Среднее абсолютное отклонение, % | Максимальное, % |', '|---|---:|---:|---:|']
for ds in ['n-gk','dmu','n-gk 2','all']:
 x=summary[ds];lines.append(f"| {'Все' if ds=='all' else ds} | {x['count']} | {x['mape']:.4f} | {x['max_percent']:.4f} |")
lines+=['',f"Рассчитаны {len(new)-len(errors)} из {len(inv)} доступных пар; {len(rows)} сравнений с контролем. Значения TotalVolume проверялись отдельно через SQLite в режиме только чтения. Все численные поля и 190 признаков совпали с исходной версией для всех успешных пар. Для папки «тест» нет подходящих контрольных записей; один повреждённый BIN даёт прежнюю ошибку чтения. Исходные файлы и базы перепроверены по SHA256.", '',
'Оценочная модель и её ограничения не изменены. Старые контрольные данные участвовали в обучении модели; этот аудит проверяет сохранение результатов и не является новым независимым испытанием точности. Известные выбросы, включая 12837, не исключались.', '',
'## Воспроизведение','', '[Пошаговая инструкция](docs/PERFORMANCE-3.31.md). tests/performance331.cpp и scripts/benchmark331.py: замеры и точное сравнение геометрии. tests/nearest331_test.cpp: точный обход KD-дерева. tests/control327_benchmark.cpp: TSV с BIN-парами, выходной каталог и число работников; scripts/inventory331.py и scripts/report331.py: фиксация входов и независимая сверка SQ3. Снимок исходников baseline берётся из сохранённого архива 3.30.2 в build/perf331/baseline/src. BIN/SQ3 в поставку не включаются.','']
(r/'PERFORMANCE-3.31-report.md').write_text('\n'.join(lines),encoding='utf-8')
(r/'CONTROL-3.31-results.csv').write_bytes((audit/'results.csv').read_bytes())
print(json.dumps({k:v for k,v in summary.items() if k!='verification'},ensure_ascii=False,indent=2))
print('PASS exact full audit and unchanged input SHA256',len(new)-len(errors),'successful pairs',flush=True)
