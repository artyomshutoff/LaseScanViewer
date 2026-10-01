"""Validate the complete audit, immutable inputs, previous release and GUI before packaging."""
from pathlib import Path
import csv,hashlib,json,shutil,sys,zipfile
from PIL import Image
r=Path(__file__).resolve().parents[1];p=r/'build/control327';out=r/'releases/LaseScanViewer-3.27.0';old=r/'releases/LaseScanViewer-3.26.0'
inventory=json.loads((p/'inventory.json').read_text(encoding='utf-8'))
for folder,expected in [('new',73),('old',278),('candidate-new',73),('candidate-old',278)]:
 rows=list(csv.DictReader((p/folder/'raw.csv').open(encoding='utf-8')))
 assert len(rows)==expected,(folder,len(rows))
 assert len({(x['dataset'],x['id']) for x in rows})==expected
for file,record in json.loads((p/'input-hashes.json').read_text(encoding='utf-8')).items():
 f=r/file;assert f.stat().st_size==record['size'] and hashlib.sha256(f.read_bytes()).hexdigest()==record['sha256'],file
assert {str(f.relative_to(r)) for f in (r/'n-gk 2').glob('*_Full.bin')}=={x['full'] for x in inventory if x['dataset']=='n-gk 2'},'Full inventory changed'
assert {str(f.relative_to(r)) for f in (r/'n-gk 2').glob('*_Empty.bin')}=={x['empty'] for x in inventory if x['dataset']=='n-gk 2' and x['empty']},'Empty inventory changed'
for name,digest in json.loads((old/'SHA256.json').read_text()).items():
 if name.endswith('.ini'):continue # Preferences are user state, not release code.
 assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest,name
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as z:
 for name in ['analysis.hpp','cargo.hpp','body_filter.hpp','volume.hpp','calibration_features.hpp','calibration.hpp','calibration_kernel_model.hpp','kernel_estimate.hpp','rim_alignment.hpp','bed_alignment.hpp']:
  assert (r/'src'/name).read_bytes()==z.read('src/'+name),name
summary=json.loads((p/'candidate-summary.json').read_text(encoding='utf-8'))
assert summary['all']['count']==344 and summary['all']['after_mape']<summary['all']['before_mape']
assert summary['n-gk 2']['count']==73 and summary['n-gk 2']['worsened']==0
assert summary['n-gk']['count']==261 and summary['n-gk']['changed']==0
assert summary['dmu']['count']==10 and summary['dmu']['changed']==0
changes=list(csv.DictReader((p/'candidate-changes.csv').open(encoding='utf-8-sig')))
assert [(x['dataset'],int(x['id'])) for x in changes]==[('n-gk 2',12942)]
for folder in ['ui-327-x64','ui-327-x86']:
 f=r/'build'/folder;assert (f/'v2-ok.txt').exists() and not (f/'v2-error.txt').exists()
 assert (f/'theme-light.png').read_bytes()==(f/'theme-dark.png').read_bytes()
 assert Image.open(f/'theme-light.png').getpixel((0,0))==(0,0,0)
 for theme in ['light','dark']:Image.open(f/f'theme-{theme}.bmp').save(f/f'Window-{theme}.png')
results=list(csv.DictReader((p/'candidate-results.csv').open(encoding='utf-8-sig')))
lines=['# LaseScanViewer 3.27 — проверка новых данных n-gk 2','',
 'Дата проверки: 01.10.2026. Сравниваются EXE/численные алгоритмы 3.26 и 3.27 при одинаковых настройках: CPU, автоматическое совмещение с нуля, координаты в миллиметрах, шаг 100 с адаптацией, порог груза 50, очистка кузова, крупнейшая область, встроенная оценочная модель включена. Процент — абсолютная разница с TotalVolume, делённая на TotalVolume; средняя ошибка не взвешивается по объёму.',
 '', '## Состав данных', '',
 '- n-gk 2: 125 Full, 73 парных Empty. Все 73 исправные пары имеют уникальную завершённую запись Status=5 с положительным TotalVolume и совпадающей меткой автомобиля в базе **n-gk/TVM_Measurement_Data_Base.sq3**. Собственной SQ3 в n-gk 2 нет. 52 Full без Empty не рассчитаны.',
 '- n-gk: 262 пары; 13955 содержит повреждённый/неполный BIN и не читается обеими версиями. Сравнивается 261 исправная пара.',
 '- dmu: все 10 пар сравниваются с собственной SQ3.',
 '- тест: все 6 пар рассчитаны обеими версиями; в доступной SQ3 нет соответствующих контрольных записей. Их объёмы не изменились, но процент отклонения не определён.',
 '- Итого: 351 попытка расчёта в каждой версии, 350 успешных, 344 сравнения с контролем. Ненадёжные совмещения и выбросы включены в статистику. SHA256 всех используемых BIN и SQ3 перепроверены после расчётов; состав новой папки не изменился.',
 '', '## Результаты', '', '| Набор | Пары с контролем | Среднее до → после, % | MAE после, м³ | Максимум после, % |', '|---|---:|---:|---:|---:|']
for ds in ['n-gk 2','n-gk','dmu','all']:
 m=summary[ds];lines.append(f"| {'Все' if ds=='all' else ds} | {m['count']} | {m['before_mape']:.4f} → {m['after_mape']:.4f} | {m['after_mae_m3']:.4f} | {m['after_max']:.4f} |")
lines+=['','## Изменение алгоритма','',
 'Для Full/Empty с исходным покрытием ниже 55% после неудачного локального уточнения проверяются ещё восемь продольных стартов ±12/16/20/24% размера сцены. Выделяются локально плоские крутые поверхности; используется соответствие нормалей и устойчивое уточнение XY/угла. Сдвиг принимается только при улучшении двустороннего покрытия минимум на 5 процентных пунктов и на 12% относительно исходного, стоимости минимум на 7%, RMS минимум на 3%, распределённых опорах и согласии хотя бы двух стартов. Прежнее успешно принятое локальное уточнение имеет приоритет. После сдвига независимо уточняется высота. Контрольный объём, имя файла, ID и автомобиль не участвуют в поиске.',
 '', 'На 12942 первоначальный разворот 180° сопровождался ложным продольным совпадением стенок. Сдвиг X базы изменился −2744 → −459 мм; угол −178.347° → −178.945°, покрытие 45.68% → 50.45%, RMS 120.60 → 113.66 мм. Итог 21.542 → 29.027 м³ при TotalVolume 27.657 м³; абсолютная ошибка 22.111% → 4.953%. Метка предварительного результата сохранена: полного надёжного совмещения ещё нет.',
 '', 'Из 350 успешно рассчитанных пар изменились положение и объём только 12942. Все остальные результаты, включая 277 прежних исправных пар, совпадают с 3.26 по числам double в CSV. Встроенная модель не переобучалась. Интегрирование, выделение груза и диапазоны применимости модели побайтово совпадают с предыдущим исходным выпуском.',
 '', '## Нерешённые случаи и границы проверки','',
 'Максимум остался на 12837: 15.388 м³ против TotalVolume 26.988 м³, ошибка 42.984%. Геометрический интеграл — 15.370 м³. При включении/отключении очистки и выбора крупнейшей области он меняется лишь до 15.378 м³; до очистки положительная разность поверхностей — 15.524 м³. Продольные и поперечные сечения показывают согласованные кузова. Сопоставление по ID и метке проверено, TrailerVolume=-1. Эти наблюдения не позволяют установить причину расхождения с контролем или объявить контроль ошибочным. Значение не исключалось и не подгонялось.',
 '', 'На 12890: 34.275 против 38.406 м³ (10.755%). Full содержит большой непросканированный участок в передней части кузова. Расширять заполнение неподдержанных пропусков или автоматически увеличивать все объёмы без проверки оснований не стали.',
 '', 'Это проверка разработки на имеющихся контрольных значениях, не гарантия погрешности. Старые 271 контрольные пары ранее использованы для обучения встроенной модели; новые 73 не добавлялись в её обучение. Проблемная пара 12942 использовалась при разработке расширенного поиска, поэтому итоговое сравнение не является полностью слепым независимым испытанием. Погрешность чистой геометрии указана в CSV отдельно; для новых данных её среднее после изменения — '+f"{summary['n-gk 2']['geometric_mape']:.4f}%.",
 '', '## Проверки поставки','',
 'Синтетические развороты 180°/37°, шум, окклюзия и вырожденная плоскость; прежнее структурное уточнение 13785; новая регрессия 12942; полный повторный прогон обеих версий на всех 351 парах. GUI x64 (12942) и x86 (14301): камера после совмещения/расчёта, предварительная оценка, курсоры, тент и независимые слои, обе темы с одинаковым чёрным просмотром, PNG/HTML/PDF/PLY. Отдельная проверка CPU/GPU записана в Tests-summary.json.',
 '', '## Воспроизведение','',
 'Инвентаризация: scripts/inventory327.py. C++-прогон: tests/control327_benchmark.cpp; исполняемый файл принимает UTF-8 TSV (набор<TAB>полный путь к Full), выходной каталог и число работников. Он читает только BIN, а не SQ3 или TotalVolume. Сверка: scripts/report_control327.py и scripts/compare_control327.py. Протоколы содержат положения, покрытие, RMS, геометрический интеграл и итоговую оценку. BIN/SQ3 в архив исходников не включены.', '']
(r/'CONTROL-3.27-report.md').write_text('\n'.join(lines),encoding='utf-8')
shutil.copy2(p/'candidate-results.csv',r/'CONTROL-3.27-results.csv')
for src,dst in [('README.md','README.md'),('README-LaseScanViewer.md','README-detailed.md'),('CONTROL-3.27-report.md','CONTROL-3.27-report.md'),('CONTROL-3.27-results.csv','CONTROL-3.27-results.csv'),('assets/LaseScanViewer.ico','LaseScanViewer.ico'),('build/ui-327-x64/Window-dark.png','Viewer-dark.png'),('build/ui-327-x64/Window-light.png','Viewer-light.png')]:shutil.copy2(r/src,out/dst)
tests=dict(control=summary,compared=344,successful=350,attempted=351,changed_pairs=['n-gk 2/12942'],model='Unchanged from 3.26; no new training',inputs='SHA256 checked; new inventory stable',old_release='3.26 immutable code/assets verified',gui_x64='PASS 12942',gui_x86='PASS 14301',cpu_gpu=(p/'gpu-result.txt').read_text(encoding='utf-8') if (p/'gpu-result.txt').exists() else 'Not recorded')
(out/'Tests-summary.json').write_text(json.dumps(tests,ensure_ascii=False,indent=2),encoding='utf-8')
names={'README.md','README-LaseScanViewer.md','CONTROL-3.27-report.md','CONTROL-3.27-results.csv','build.ps1','app.rc','app.manifest','logo2.svg'}
for folder in ['src','tests','scripts','assets','docs']:
 names.update(f.relative_to(r).as_posix() for f in (r/folder).rglob('*') if f.is_file() and '__pycache__' not in f.parts)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
 for name in sorted(names):z.write(r/name,name)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as z:assert z.testzip() is None
# The portable binaries may have created preferences during tests; fresh copies
# receive the documented defaults instead of distributing test-specific state.
if (out/'LaseScanViewer.ini').exists():(out/'LaseScanViewer.ini').unlink()
hashes={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in out.iterdir() if f.is_file() and f.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(hashes,indent=2),encoding='utf-8')
archive=r/'releases/LaseScanViewer-3.27.0-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for f in out.iterdir():
  if f.is_file():z.write(f,out.name+'/'+f.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
for name in ['LaseScanViewer.exe','LaseScanViewer-Portable.exe']:shutil.copy2(out/name,r/'dist'/name)
print('PASS complete 344-control audit, unchanged old results/model, input hashes, previous release, both GUIs and archive CRC')
print(archive)
