from pathlib import Path
import csv,json,statistics
r=Path(__file__).resolve().parents[1];folder=r/'build/perf318';results=[]
for case in ['dmu8','ngk14301','test43279']:
 def rows(prefix):
  files=[folder/(prefix+'-'+case+'.csv')]+sorted(folder.glob(prefix+'-'+case+'-r*.csv'))
  return [next(csv.DictReader(p.open())) for p in files]
 old,new=rows('baseline'),rows('parallel');reference=old[0]
 for row in old+new:
  for key in ['angle','dx','dy','dz','overlap','cargo','step','min','max']:assert float(row[key])==float(reference[key]),(case,key)
  assert abs(float(row['calibrated'])-float(reference['calibrated']))/1e9<1e-9
 before=statistics.median(float(x['total']) for x in old);after=statistics.median(float(x['total']) for x in new)
 results.append(dict(case=case,baseline_runs=len(old),optimized_runs=len(new),before_seconds=before,after_seconds=after,speedup=before/after,reduction_percent=100*(1-after/before),alignment_before=statistics.median(float(x['align']) for x in old),alignment_after=statistics.median(float(x['align']) for x in new),geometry_and_pose='exact match',max_calibrated_difference_m3=max(abs(float(x['calibrated'])-float(reference['calibrated']))/1e9 for x in old+new)))
summary=dict(cases=results,method='Median sequential before/after runs, identical -O2 x64 toolchain; load+triangulate+align+calculateVolume. Local machine, no claim about all hardware.',assembly='x64 SSE2 descriptor norm; x86/other compilers use C++ fallback',assembly_microbenchmark='1000000 calls: C++ 0.131505 s, SSE2 0.0382589 s, 3.43723x; 10000 random vectors within 1e-14 relative error')
(folder/'summary.json').write_text(json.dumps(summary,indent=2))
text=['# Ускорение LaseScanViewer 3.18','', 'Совмещение выполняет первые 144 независимые гипотезы на максимум четырёх рабочих потоках. Итоги собираются в прежнем порядке, последующие 11 гипотез и выбор победителя сохранены. Одинаковые пространственные выборки и KD-деревья трёх разрешений повторно используются при уточнении конкурирующих вариантов. Повороты внутри циклов используют заранее вычисленные sin/cos. Число проверок и правила расчёта груза не уменьшались.','', 'В x64 включён встроенный ассемблер SSE2 для нормы расстояния между 190 геометрическими признаками. Микрозамер: 0.131505 → 0.0382589 с на миллион вызовов (3.44×). Это отдельная небольшая часть, основное ускорение приложения даёт параллельное совмещение. AVX/FMA не нужны. x86 использует переносимый C++-вариант. Разница округления проверена на 10000 случайных векторах; прогнозы проверены по всем 271 сохранённым контрольным случаям.','', '| Пара | До, с | После, с | Ускорение | Сокращение времени |','|---|---:|---:|---:|---:|']
for x in results:text.append(f"| {x['case']} | {x['before_seconds']:.2f} | {x['after_seconds']:.2f} | {x['speedup']:.2f}× | {x['reduction_percent']:.1f}% |")
text+=['','Замер включает чтение BIN, построение поверхности, совмещение и расчёт объёма. Для dmu/n-gk использована медиана двух запусков каждой версии, для пары с разворотом — один запуск каждой. Это измерения на текущем компьютере, а не гарантия для любого оборудования. Во время замеров нагрузка не создавалась дополнительными тестами.','', 'На трёх парах преобразования, совпадение, геометрический объём, шаг и диапазон чувствительности совпадают точно. Разница итогового объёма с моделью меньше 10⁻⁹ м³. Сохранены GUI, прогресс, независимые слои и экспорт; предыдущая 3.17 сохранена.']
(r/'PERFORMANCE-3.18-report.md').write_text('\n'.join(text)+'\n',encoding='utf-8')
print(json.dumps(results,indent=2))
