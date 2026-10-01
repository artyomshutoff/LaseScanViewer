from pathlib import Path
import json,hashlib,zipfile,shutil,subprocess,sys
from PIL import Image
r=Path(__file__).resolve().parents[1];out=r/'releases/LaseScanViewer-3.25.0';old=r/'releases/LaseScanViewer-3.24.0'
for name,digest in json.loads((old/'SHA256.json').read_text()).items():assert hashlib.sha256((old/name).read_bytes()).hexdigest()==digest,name
results=json.loads((r/'build/perf325/results.json').read_text());assert len(results)==7
for row in results:assert row['runs'][0]['csv']==row['runs'][1]['csv']
for folder in ['ui-325-x64-final','ui-325-x86-final']:
 p=r/'build'/folder;assert (p/'v2-ok.txt').exists() and not (p/'v2-error.txt').exists()
 assert (p/'theme-light.png').read_bytes()==(p/'theme-dark.png').read_bytes()
 assert Image.open(p/'theme-light.png').getpixel((0,0))==(0,0,0)
 assert sum(Image.open(p/'caption-light.png').getpixel((600,15)))>600
 assert sum(Image.open(p/'caption-dark.png').getpixel((600,15)))<120
 for theme in ['light','dark']:Image.open(p/f'theme-{theme}.bmp').save(p/f'Window-{theme}.png')
src=(r/'scripts/check_icon324.py').read_text().replace('3.24.0','3.25.0');(r/'scripts/check_icon325.py').write_text(src)
subprocess.run([sys.executable,str(r/'scripts/check_icon325.py')],check=True)
before=sum(x['runs'][0]['seconds'] for x in results);after=sum(x['runs'][1]['seconds'] for x in results)
lines=['# LaseScanViewer 3.25: интерфейс, тент и скорость','',
 'Верхнее меню удалено; все команды сохранены в кнопках и настройках. Заголовок содержит [Version: 3.25.0]. Светлая/тёмная тема задаёт оформление заголовка через DWM (цвета на поддерживаемых версиях Windows, светлый/тёмный режим на Windows 10). Окно скана остаётся чёрным; PNG при переключении тем совпадают побайтово.',
 '', 'Виртуальный тент — горизонтальная плоскость на уровне по воде Empty. Геометрический объём выше этой плоскости интегрируется по уже выделенным ячейкам груза с исходными double-высотами и площадями. Показываются только точки выше плоскости либо отсечённые призмы, остальные слои временно скрыты. Камера, общий объём, TotalVolume и исходные сканы не изменяются. Это модель без провисания, не физическая симуляция ткани. При отсутствии надёжно определённого уровня бортов опция недоступна.',
 '', 'Поиск ближайшего соседа структурного уточнения использует прежний порядок обхода KD-дерева и строгие сравнения расстояния, но больше не создаёт динамический heap на каждый запрос. 50 000 запросов дали точно прежние индексы; один измеренный проход 30.513 → 25.668 мс (около 16% быстрее для этого участка).',
 '', 'Сохранено x64 SSE2-ассемблерное ядро дескрипторов: 10 000 сравнений с относительной разницей ≤1e-14; тест 133.781 → 45.472 мс, 2.94×. Новый эксперимент SSE2 для расстояния XYZ дал 16.532–16.749 мс против C++ 14.001–14.117 мс; его намеренно не подключали к EXE. На x86 действует прежний C++ путь.',
 '', '| Данные | ID | 3.24, с | 3.25, с | Численный результат |','|---|---:|---:|---:|---|']
for x in results:lines.append(f"| {x['dataset']} | {x['id']} | {x['runs'][0]['seconds']:.3f} | {x['runs'][1]['seconds']:.3f} | Точно совпал |")
lines += ['',f'Один проход всех семи пар: {before:.3f} → {after:.3f} с. Это измерение без повторений; фоновые процессы и компиляция могли влиять на время. Существенное ускорение всего конвейера не подтверждено. Гипотезы, точность, совмещение и итоговый объём не сокращались.',
 '', 'Проверены x64/x86 GUI: применение и отмена настроек, тент, вода, камера, тема, версия и отсутствие меню, PNG/HTML/PDF/PLY, база SQ3. Проверены синтетические повороты 180°/37°, шум, частично скрытое дно; CPU/GPU на 13785 совпали точно (9242 GPU-пакета, без fallback). x86 13785: объёмы совпали с 3.24, небольшое платформенное округление Z сохранено. Тесты тента: известный объём, нулевое превышение, неизвестные борта, double-ячейки и неизменность груза. Предыдущие релизы сохранены и проверены SHA256.',
 '', 'Подробные измерения: Performance-325-results.json. Исходный контроль семи пар: ALIGNMENT-3.24-results.csv.']
(r/'PERFORMANCE-3.25-report.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
copies=[('README-LaseScanViewer.md','README.md'),('PERFORMANCE-3.25-report.md','PERFORMANCE-3.25-report.md'),('build/perf325/results.json','Performance-325-results.json'),('ALIGNMENT-3.24-results.csv','ALIGNMENT-3.24-results.csv'),('ALIGNMENT-3.24-report.md','ALIGNMENT-3.24-report.md'),('build/ui-325-x64-final/Window-light.png','Viewer-light.png'),('build/ui-325-x64-final/Window-dark.png','Viewer-dark.png'),('build/ui-325-x64-final/virtual-tarp.png','Viewer-virtual-tarp.png'),('build/ui-325-x64-final/layers-all.png','Viewer-layers.png'),('assets/LaseScanViewer.ico','LaseScanViewer.ico'),('assets/LaseScanViewer.png','LaseScanViewer-icon.png')]
copies.extend([('build/ui-325-x64-final/caption-light.png','Title-light.png'),('build/ui-325-x64-final/caption-dark.png','Title-dark.png')])
for src,dst in copies:shutil.copy2(r/src,out/dst)
(out/'Tests-summary.json').write_text(json.dumps(dict(real_pairs=7,numeric='exact CSV equality with 3.24',gui_x64='PASS',gui_x86='PASS',tarp='PASS',cpu_gpu='exact on 13785',assembly='existing SSE2 retained; slower new experiment excluded',previous_release='3.24 SHA256 verified'),indent=2))
names={src for src,_ in copies};names.update(['build.ps1','app.rc','app.manifest','logo2.svg'])
for folder in ['src','tests','scripts','assets']:names.update(p.relative_to(r).as_posix() for p in (r/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
with zipfile.ZipFile(old/'LaseScanViewer-source.zip') as previous:
 with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
  for name in previous.namelist():
   if name.startswith('build/') and name not in names:z.writestr(name,previous.read(name))
  for name in sorted(names):z.write(r/name,name)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip') as z:assert z.testzip() is None
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='SHA256.json'}
(out/'SHA256.json').write_text(json.dumps(hashes,indent=2))
archive=r/'releases/LaseScanViewer-3.25.0-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in out.iterdir():
  if p.is_file():z.write(p,out.name+'/'+p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print('PASS package, previous hashes, numerical equality, x64/x86 GUI, black viewport, embedded icons, ZIP CRC')
print(archive)
