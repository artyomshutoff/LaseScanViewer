from pathlib import Path
import csv,json,statistics
r=Path(__file__).resolve().parents[1];folder=r/'build/perf320';results=[]
for case in ['dmu8','ngk14301','test43279']:
 def rows(prefix):
  files=[folder/(prefix+'-'+case+'.csv')]+sorted(folder.glob(prefix+'-'+case+'-r*.csv'))
  return [next(csv.DictReader(p.open())) for p in files]
 old,new=rows('baseline'),rows('optimized');reference=old[0]
 for row in old+new:
  for key in ['angle','dx','dy','dz','overlap','cargo','step','min','max']:assert float(row[key])==float(reference[key]),(case,key)
  assert abs(float(row['calibrated'])-float(reference['calibrated']))/1e9<1e-9
 before=statistics.median(float(x['total']) for x in old);after=statistics.median(float(x['total']) for x in new)
 results.append(dict(case=case,baseline_runs=len(old),optimized_runs=len(new),before_seconds=before,after_seconds=after,speedup=before/after,reduction_percent=100*(1-after/before),alignment_before=statistics.median(float(x['align']) for x in old),alignment_after=statistics.median(float(x['align']) for x in new),geometry_and_pose='exact match',max_calibrated_difference_m3=max(abs(float(x['calibrated'])-float(reference['calibrated']))/1e9 for x in old+new)))
for case in ['dmu8','ngk14301']:
 old=list(csv.DictReader((folder/('modes-baseline-'+case+'.csv')).open()))
 new=list(csv.DictReader((folder/('modes-optimized-'+case+'.csv')).open()))
 assert old==new,(case,'pipeline mode mismatch')
summary=dict(cases=results,mode_checks='12 before/after cases: default, ROI, no adaptive grid/gap reconstruction, median estimator, no filters, Empty/Empty; every output and geometry digest exactly equal',method='Median sequential before/after x64 -O2 runs vs 3.19; two runs per dmu/n-gk, one per reversed scan, on this machine.')
(folder/'summary.json').write_text(json.dumps(summary,indent=2))
text=['# Ускорение LaseScanViewer 3.20 относительно 3.19','', 'Повторные результаты расчёта на шагах 1.0/0.75/1.25 переиспользуются при проверке чувствительности. Остальные независимые проверки пяти сеток и трёх фаз выполняются на максимум четырёх потоках; результаты объединяются в исходном порядке. Геометрические признаки четырёх разрешений также вычисляются независимо в четырёх потоках. Поворот в циклах построения сеток использует заранее рассчитанные sin/cos. Все проверки и правила выбора объёма сохранены.','', '| Пара | До, с | После, с | Сокращение всего цикла | Расчёт объёма: до → после |','|---|---:|---:|---:|---|']
for x in results:
 case=x['case'];old,new=rows('baseline'),rows('optimized')
 # rows() uses the enclosing current case.
 volume_before=statistics.median(float(v['volume']) for v in old);volume_after=statistics.median(float(v['volume']) for v in new)
 x['volume_before']=volume_before;x['volume_after']=volume_after
 text.append(f"| {case} | {x['before_seconds']:.2f} | {x['after_seconds']:.2f} | {x['reduction_percent']:.1f}% | {volume_before:.2f} → {volume_after:.2f} с |")
text+=['','Замер включает чтение BIN, построение поверхности, совмещение и расчёт объёма. Использована та же сборка x64 -O2 на текущем компьютере; во время замеров тяжёлые тесты не запускались. Для dmu/n-gk медиана двух запусков каждой версии, для пары с разворотом — один запуск каждой. Это не гарантия для любого оборудования.','', 'На трёх парах совмещение, объём и диапазон чувствительности совпали. Дополнительно 12 случаев на dmu/n-gk с ROI, отключением адаптивной сетки и восстановления, медианной поверхностью, выключенными фильтрами и Empty/Empty совпадают полностью: точки, поверхности, ячейки, численные результаты, признаки предварительности и чувствительности. Прогресс монотонен и достигает 100%. Проверены калибровочные ограничения и независимые значения n-gk/dmu. Интерфейс и экспорт проверены в x64/x86. Предыдущая 3.19 сохранена.']
(r/'PERFORMANCE-3.20-report.md').write_text('\n'.join(text)+'\n',encoding='utf-8')
(folder/'summary.json').write_text(json.dumps(summary,indent=2))
print(json.dumps(results,indent=2))
