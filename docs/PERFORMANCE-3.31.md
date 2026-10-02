# Воспроизведение проверки 3.31

Нужны исходники 3.31, Python 3.10+ и Clang/LLVM-MinGW с C++17. Сканы и SQ3 пользователь предоставляет отдельно в папках `n-gk`, `dmu`, `тест`, `n-gk 2`. Для упаковки со снимками интерфейса дополнительно требуется Pillow.

1. Скачайте `LaseScanViewer-source.zip` из релиза 3.30.2 и сохраните под отдельным именем, например `baseline-3.30.2.zip`.
2. Выполните из корня проекта:

```powershell
python scripts/prepare_performance331.py baseline-3.30.2.zip
python scripts/inventory331.py
$compiler = '.tools/llvm-mingw-20260922-ucrt-x86_64/bin/clang++.exe'
& $compiler -std=c++17 -O2 -static -municode -DPERF_BASELINE tests/performance331.cpp -o build/perf331/baseline.exe
& $compiler -std=c++17 -O2 -static -municode tests/performance331.cpp -o build/perf331/optimized.exe
python scripts/benchmark331.py --runs 3
python scripts/verify331.py
& $compiler -std=c++17 -O2 -static -municode -DPERF_331_BASELINE tests/control327_benchmark.cpp -o build/perf331/control-baseline.exe
& $compiler -std=c++17 -O2 -static -municode tests/control327_benchmark.cpp -o build/perf331/control.exe
& build/perf331/control-baseline.exe build/perf331/baseline-needed.tsv build/perf331/audit/baseline-fresh 2
& build/perf331/control.exe build/perf331/all.tsv build/perf331/audit/optimized 2
python scripts/report331.py
```

Исправьте путь `$compiler`, если используете другой компилятор. `verify331.py` также использует фиксированный путь к LLVM-MinGW и компилирует x86-варианты; его можно изменить под локальную установку. Замеры шести пар требуют тех же номеров сканов; для других файлов измените список `cases` в `benchmark331.py` и соответствующие входы в `verify331.py`.

На чистом проекте все исходные пары рассчитываются заново обеими версиями. Локальные исторические результаты допускаются только при совпадении SHA256 входных файлов и всех транзитивных численных заголовков исходников. Сверка требует точного совпадения полей и 190 геометрических признаков; время сравнивается отдельно. `inventory331.py` фиксирует контрольные записи через read-only SQLite, а C++-исполняемые файлы не читают SQ3.

Запускайте измерение времени без полного параллельного аудита и других тяжёлых задач. `benchmark331.py` чередует порядок версий и сравнивает медианы. Полный аудит служит проверкой результатов; его время не используется для оценки ускорения. Сканы и базы должны оставаться неизменными до завершения проверки: `report331.py` повторно проверяет их SHA256.

В этой версии модель объёма и её коэффициенты сохранены. Старые контрольные данные частично использовались при обучении, поэтому такая проверка подтверждает сохранение результатов, а не независимую оценку точности новой модели.
