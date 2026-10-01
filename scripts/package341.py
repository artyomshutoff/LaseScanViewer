from pathlib import Path
import shutil,zipfile,hashlib,json,csv
r=Path(__file__).resolve().parents[1];out=r/'releases/LaseScanViewer-3.4.1'
rows=list(csv.DictReader((r/'build/body-regression.csv').open()))
assert len(rows)==89 and all(abs(float(x['volume'])-float(x['old_volume']))<1e-5 and int(x['cargo_points'])>0 for x in rows)
for folder in ['body-after-14301','body-x86-14269']:
 p=r/'build'/folder;assert (p/'v2-ok.txt').exists() and not (p/'v2-error.txt').exists()
readme=(r/'README-LaseScanViewer.md').read_text(encoding='utf-8').replace('# LaseScanViewer 3.4.0','# LaseScanViewer 3.4.1',1)
note='''## Отделение кузова в 3.4.1

Исправлен отбор точек для цветного облака груза и его PLY-экспорта. Вместо всех точек выше дна одной XY-ячейки остаются точки возле оценённой поверхности груза, с допуском на её наклон. Точки, близкие к измерениям совмещённого Empty в трёхмерном пространстве, исключаются как совпадающие с кузовом. При включении «Остальные точки» исключённые измерения показаны серыми. Фильтр действует при включённом отсечении бортов в настройках; его можно отключить.

Объём по разности поверхностей и геометрия интеграционных ячеек не изменены: это исправление классификации точек, а не новая формула м³. На всех 89 парах с сохранёнными позами и выбранными шагами 3.4.0 проверено совпадение объёмов и непустое облако груза. На 14301 исключены 41 193 точки вне поверхности груза или вблизи Empty, осталось 122 614 цветных точек; объём 26.694244 м³. В режиме «Поверхность» видны стенки расчётных призм — это границы интеграла, а не измеренные борта.

При плохом совмещении или тонком слое материала у стенок разделение неоднозначно: часть близких к кузову точек груза может быть исключена из цветного облака. Это не вычитает соответствующий объём. Проверены аналитические случаи бортов в одной ячейке с грузом, сдвиг базы, переключение фильтра, сохранение интеграла, x86/x64, серый фон и PLY/PNG/HTML.

'''
readme=readme.replace('## Быстрый старт',note+'## Быстрый старт',1)
(r/'README-LaseScanViewer.md').write_text(readme,encoding='utf-8')
for src,dst in [('README-LaseScanViewer.md','README.md'),('build/body-regression.csv','Body-regression.csv'),('NGK-3.4-results.csv','NGK-3.4-results.csv'),('NGK-3.4-report.md','NGK-3.4-report.md'),('build/body-after-14301/full-cargo.png','Cargo-14301.png')]:shutil.copy2(r/src,out/dst)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
 for folder in ['src','tests','scripts']:
  for p in (r/folder).rglob('*'):
   if p.is_file() and '__pycache__' not in p.parts:z.write(p,p.relative_to(r))
 for name in ['build.ps1','app.rc','app.manifest','logo2.svg','README-LaseScanViewer.md','build/ngk-full-340.csv']:
  z.write(r/name,name)
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(hashes,indent=2))
archive=r/'releases/LaseScanViewer-3.4.1-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in out.iterdir():
  if p.is_file():z.write(p,Path(out.name)/p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print('PASS 3.4.1 package, sources, 89 pairs, x86/x64 UI tests, ZIP CRC',archive)
