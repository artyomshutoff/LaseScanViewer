"""Convert the supplied SVG path into a self-contained native GDI path."""
from pathlib import Path
import re, xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[1]
d=ET.parse(root/'logo2.svg').getroot().find('{http://www.w3.org/2000/svg}path').attrib['d']
t=re.findall(r'[A-Za-z]|[-+]?(?:\d*\.\d+|\d+)(?:[eE][-+]?\d+)?',d)
i=0;x=y=sx=sy=0.;prev='';ctrl=None;ops=[];cmd=''
while i<len(t):
 if t[i].isalpha():cmd=t[i];i+=1
 c=cmd.upper();rel=cmd.islower()
 if c=='Z':ops.append(('Z',[]));x,y=sx,sy;prev=c;ctrl=None;cmd='';continue
 n={'M':2,'L':2,'H':1,'V':1,'C':6,'S':4}[c];v=list(map(float,t[i:i+n]));i+=n
 def pt(a,b):return (a+x if rel else a,b+y if rel else b)
 if c in ('M','L'):
  q=pt(*v);ops.append((c,list(q)));x,y=q
  if c=='M':sx,sy=q;cmd='l' if rel else 'L'
 elif c=='H':x=v[0]+x if rel else v[0];ops.append(('L',[x,y]))
 elif c=='V':y=v[0]+y if rel else v[0];ops.append(('L',[x,y]))
 else:
  if c=='C':p1=pt(*v[:2]);p2=pt(*v[2:4]);q=pt(*v[4:])
  else:p1=(2*x-ctrl[0],2*y-ctrl[1]) if prev in ('C','S') else (x,y);p2=pt(*v[:2]);q=pt(*v[2:])
  ops.append(('C',list(p1+p2+q)));ctrl=p2;x,y=q
 if c not in ('C','S'):ctrl=None
 prev=c
s='#pragma once\n// Generated from logo2.svg; do not hand edit.\ninline void drawBrandLogo(HDC dc,RECT r,COLORREF logoColor=RGB(99,172,80)){\n auto point=[&](double x,double y){return POINT{r.left+LONG(std::lround(x*(r.right-r.left)/142.26)),r.top+LONG(std::lround(y*(r.bottom-r.top)/35.26))};};\n SaveDC(dc);SetPolyFillMode(dc,WINDING);BeginPath(dc);\n'
for c,v in ops:
 if c=='Z':s+=' CloseFigure(dc);\n'
 elif c in ('M','L'):s+=f' {{auto p=point({v[0]:.7g},{v[1]:.7g});'+('MoveToEx(dc,p.x,p.y,nullptr);' if c=='M' else 'LineTo(dc,p.x,p.y);')+'}\n'
 else:s+=' {POINT p[]={'+','.join(f'point({v[k]:.7g},{v[k+1]:.7g})' for k in (0,2,4))+'};PolyBezierTo(dc,p,3);}\n'
s+=' EndPath(dc);SelectObject(dc,GetStockObject(DC_BRUSH));SetDCBrushColor(dc,logoColor);FillPath(dc);RestoreDC(dc,-1);}\n'
(root/'src/logo.hpp').write_text(s,encoding='utf-8')
print('logo vector commands',len(ops))
