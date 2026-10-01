"""Build a square Windows icon from the project's existing LASE vector logo."""
from pathlib import Path
from PIL import Image
import subprocess
r=Path(__file__).resolve().parents[1]
(r/'build/icon314').mkdir(exist_ok=True)
(r/'assets').mkdir(exist_ok=True)
compiler=r/'.tools/llvm-mingw-20260922-ucrt-x86_64/bin/clang++.exe'
subprocess.run([str(compiler),'-std=c++17','-O2','-static','scripts/icon_raster.cpp','-o','build/icon314/raster.exe','-lgdi32'],cwd=r,check=True)
subprocess.run([str(r/'build/icon314/raster.exe')],cwd=r,check=True)
image=Image.open(r/'build/icon314/icon.bmp').convert('RGB').resize((256,256),Image.Resampling.LANCZOS)
image.save(r/'assets/LaseScanViewer.png')
image.save(r/'assets/LaseScanViewer.ico',sizes=[(16,16),(24,24),(32,32),(48,48),(64,64),(128,128),(256,256)])
print('Square icon: 16, 24, 32, 48, 64, 128, 256 pixels')
