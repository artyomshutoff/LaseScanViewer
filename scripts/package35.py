from pathlib import Path
import csv,json,hashlib,zipfile,shutil
r=Path(__file__).resolve().parents[1];out=r/'releases/LaseScanViewer-3.5.0'
summary=json.loads((r/'build/ngk35-summary.json').read_text());assert summary['training_mape']<1 and summary['pairs']==89
learn=list(csv.DictReader((r/'build/volume-audit-3.5.csv').open()));assert len(learn)==69 and all(not x['error'] and float(x['self_m3'])==0 for x in learn)
for id in ['14301','14269']:
 p=r/'build'/('ngk35-ui-'+id);assert (p/'v2-ok.txt').exists() and not (p/'v2-error.txt').exists()
readme=(r/'README-LaseScanViewer.md').read_text(encoding='utf-8').replace('# LaseScanViewer 3.4.1','# LaseScanViewer 3.5.0',1)
section='''## Калиброванная оценка n-gk в 3.5

Добавлена отдельная оценка, обученная по 89 парам n-gk: среднее абсолютное относительное отклонение от TotalVolume на этих данных **0.87%**. При пятикратной групповой проверке с исключением автомобилей из обучения линейной модели — **1.42%**. Это разные показатели: точность ниже 1% на новых сканах не подтверждена. Все эти данные ранее использовались при выборе алгоритма; независимый набор ещё нужен.

В просмотре и HTML калибровка явно подписана, рядом показан исходный геометрический объём. Его среднее отклонение по n-gk остаётся 1.47%. Геометрия, точки и PLY относятся к исходному интегралу и не подгоняются под калиброванное число. База не читается приложением; номера измерений, имена файлов и автомобилей не участвуют в прогнозе. Встроенная регуляризованная модель использует 48 оценок чувствительности объёма к выбору опорных участков кузова. Это дополнительная статистическая оценка, не сертифицированное измерение.

Калибровка включена по умолчанию для подходящих Full/Empty группы 0 при стандартных параметрах и миллиметрах. Для Reference, ручной области, изменённых параметров и выхода признаков за обученный диапазон она не применяется. Её можно отключить в настройках: «Отдельная калиброванная оценка n-gk». В конце расчёта прогресс показывает обработку 48 вариантов. Искусственных задержек нет.

Полный свежий прогон: **NGK-3.5-report.md**, все 89 результатов: **NGK-3.5-results.csv**. Коэффициенты и границы признаков: **Calibration-model.json**, прогнозы обучения и групповой проверки: **Calibration-validation.csv**. Для воспроизводимости исходники содержат генератор модели и таблицу геометрических признаков. Предыдущие версии сохранены.

'''
if '## Калиброванная оценка n-gk в 3.5' not in readme:readme=readme.replace('## Отделение кузова в 3.4.1',section+'## Отделение кузова в 3.4.1',1)
(r/'README-LaseScanViewer.md').write_text(readme,encoding='utf-8')
for src,dst in [('README-LaseScanViewer.md','README.md'),('NGK-3.5-report.md','NGK-3.5-report.md'),('NGK-3.5-results.csv','NGK-3.5-results.csv'),('build/ngk35-summary.json','NGK-summary.json'),('build/ngk-full-350.csv','NGK-raw.csv'),('build/ngk-calibration-model.json','Calibration-model.json'),('build/ngk-calibration-validation.csv','Calibration-validation.csv'),('build/volume-audit-3.5.csv','Learn-regression.csv'),('build/ngk35-ui-14301/full-cargo.png','Calibration-14301.png')]:shutil.copy2(r/src,out/dst)
with zipfile.ZipFile(out/'LaseScanViewer-source.zip','w',zipfile.ZIP_DEFLATED) as z:
 for folder in ['src','tests','scripts']:
  for p in (r/folder).rglob('*'):
   if p.is_file() and '__pycache__' not in p.parts:z.write(p,p.relative_to(r))
 for name in ['build.ps1','app.rc','app.manifest','logo2.svg','README-LaseScanViewer.md','NGK-3.5-report.md','NGK-3.5-results.csv','build/ngk-controls.csv','build/ngk-full-341-retest.csv','build/ngk-full-350.csv','build/ngk-calibration-model.json','build/ngk-calibration-validation.csv','build/research35/rim.cpp','build/research35/rim.csv','build/learn-volume35.cpp','build/learn-audit-3.1.csv']:
  z.write(r/name,name)
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='SHA256.json'};(out/'SHA256.json').write_text(json.dumps(hashes,indent=2))
archive=r/'releases/LaseScanViewer-3.5.0-Windows.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in out.iterdir():
  if p.is_file():z.write(p,Path(out.name)/p.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print('PASS 3.5.0 package, calibration provenance, 89 pairs, 69 regressions, x86/x64, ZIP CRC',archive)
