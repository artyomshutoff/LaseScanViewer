from pathlib import Path
import ctypes as c, struct
r=Path(__file__).resolve().parents[1]
k=c.WinDLL('kernel32',use_last_error=True)
k.LoadLibraryExW.argtypes=[c.c_wchar_p,c.c_void_p,c.c_uint];k.LoadLibraryExW.restype=c.c_void_p
k.FindResourceW.argtypes=[c.c_void_p,c.c_void_p,c.c_void_p];k.FindResourceW.restype=c.c_void_p
k.LoadResource.argtypes=[c.c_void_p,c.c_void_p];k.LoadResource.restype=c.c_void_p
k.LockResource.argtypes=[c.c_void_p];k.LockResource.restype=c.c_void_p
k.SizeofResource.argtypes=[c.c_void_p,c.c_void_p];k.SizeofResource.restype=c.c_uint
k.FreeLibrary.argtypes=[c.c_void_p]
for name in ['LaseScanViewer.exe','LaseScanViewer-Portable.exe']:
 h=k.LoadLibraryExW(str(r/'releases/LaseScanViewer-3.22.0'/name),None,2);assert h
 try:
  resource=k.FindResourceW(h,101,14);assert resource
  data=c.string_at(k.LockResource(k.LoadResource(h,resource)),k.SizeofResource(h,resource))
  reserved,kind,count=struct.unpack_from('<HHH',data);assert (reserved,kind,count)==(0,1,7)
  sizes=[]
  for i in range(count):
   w,height,colors,reserved,planes,bits,length,identifier=struct.unpack_from('<BBBBHHIH',data,6+14*i)
   assert (w or 256)==(height or 256)
   assert k.FindResourceW(h,identifier,3)
   sizes.append(w or 256)
  assert sizes==[16,24,32,48,64,128,256]
  print(name+': PASS square icon resources '+str(sizes))
 finally:k.FreeLibrary(h)
