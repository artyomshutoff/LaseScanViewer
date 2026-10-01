from pathlib import Path
import csv,statistics,json
root=Path(__file__).resolve().parents[1]
rows=list(csv.DictReader((root/'build/learn-audit-final.csv').open()))
assert len(rows)==69 and all(not r.get('error') for r in rows)
old=list(csv.DictReader((root/'build/learn-baseline.csv').open()))
def val(r,k):return float(r[k])
def span(r):return max(val(r,k) for k in ('v50','cargo_m3','v150'))-min(val(r,k) for k in ('v50','cargo_m3','v150'))
accepted=[r for r in rows if r['accepted']=='1'];blocked=[r for r in rows if r['accepted']!='1'];ratios=[span(r)/val(r,'cargo_m3') for r in accepted if val(r,'cargo_m3')>1]
report=['# Проверка LaseScanViewer на Learn','', 'Контрольных объёмов нет. Ниже — проверка чтения, самосогласованности, совмещения и чувствительности к сетке. Эти результаты не устанавливают абсолютную точность и не подтверждают физическую единицу BIN. Все м³ условны: координаты приняты за миллиметры.','',
f'- Прочитаны все {len(rows)} пар Empty/Full без ошибок.',f'- Автоматически принято совмещение: {len(accepted)}; требует проверки: {len(blocked)}.',f'- Нулевых выделенных объёмов: прежде {sum(val(r,"cargo_m3")==0 for r in old)}, теперь {sum(val(r,"cargo_m3")==0 for r in rows)}. Основная исправленная причина: дно ошибочно считалось дорогой. Ненулевой объём не является доказательством точности.',f'- Сравнение каждого Empty с самим собой: максимальный ложный объём {max(val(r,"null_volume") for r in rows):.9f} ед.³.',f'- Медианный размах по шагам 50/100/150 для принятых оценок больше 1 м³: {statistics.median(ratios)*100:.1f}% от результата при шаге 100. Это чувствительность, не погрешность.', '', 'Результаты перед изменениями: `Learn-before.csv`; итоговые: `Learn-results.csv`. Во входном CSV сохранены условные метки development/validation (52/17); они служат группировкой. Полный набор просматривался при отладке: независимой слепой проверки не было.','', '## Пары, требующие проверки','']
report += [', '.join(r['id'] for r in blocked) or 'Нет отклонённых совмещений.']
report += ['', 'Для этих строк диагностический объём рассчитывался принудительно только в тестовой утилите. Интерфейс не принимает такое совмещение автоматически.', '', '## Результаты по каждой паре', '', '| Номер | Совмещение | Совпадение | Выделено при шаге 100, м³ | Диапазон шагов 50–150, м³ | Отрицательная разность, м³ | Общая сетка |','|---|---|---:|---:|---:|---:|---:|']
for r in sorted(rows,key=lambda r:r['id']):
 values=[val(r,k) for k in ('v50','cargo_m3','v150')];status='Проверить вручную' if r['accepted']!='1' else 'Предварительно' if val(r,'overlap')<.55 else 'Принято';report.append(f"| {r['id']} | {status} | {val(r,'overlap')*100:.1f}% | {val(r,'cargo_m3'):.2f} | {min(values):.2f}–{max(values):.2f} | {val(r,'negative_m3'):.2f} | {val(r,'coverage')*100:.1f}% |")
report += ['', '## Проверки с известным ответом','','Аналитические тесты проверяют объём 200 единиц³, отсечение одиночных выбросов и примыкающей узкой полосы, сохранение дна рядом с высоким бортом, интеграцию после разворота, частичные ячейки и отсутствие парных измерений. Синтетические сканы проверяют повороты 37° и 180° со сдвигом, шумом, пропусками и скрытыми нижними точками. Это проверка реализации, а не калибровка сканера.']
(root/'Learn-audit.md').write_text('\n'.join(report)+'\n',encoding='utf-8')
print(json.dumps({'pairs':len(rows),'accepted':len(accepted),'blocked':[r['id'] for r in blocked],'zero_old':sum(val(r,'cargo_m3')==0 for r in old),'zero_new':sum(val(r,'cargo_m3')==0 for r in rows),'median_grid_span_percent':statistics.median(ratios)*100}))
