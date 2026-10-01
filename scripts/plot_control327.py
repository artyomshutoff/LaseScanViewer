from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'.tools/science327'))
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
p=Path(__file__).resolve().parents[1]/'build/control327'
for id in [12837,12942,12890]:
 folder=p/f'cloud{id}'
 if not (folder/'empty.xyz').exists():continue
 a=np.fromfile(folder/'full.xyz',dtype='<f4').reshape(-1,3)/1000;b=np.fromfile(folder/'empty.xyz',dtype='<f4').reshape(-1,3)/1000
 fig,axes=plt.subplots(2,2,figsize=(12,7),layout='constrained')
 for cloud,ax,title in [(a,axes[0,0],'Full'),(b,axes[0,1],'Empty aligned')]:
  x=cloud[::3];ax.scatter(x[:,0],x[:,1],c=x[:,2],s=1,cmap='turbo',vmin=.8,vmax=3.7);ax.set_title(title);ax.set_aspect('equal');ax.set_xlabel('X, m');ax.set_ylabel('Y, m')
 for cloud,color,title in [(a,'orange','Full'),(b,'deepskyblue','Empty')]:
  q=cloud[np.abs(cloud[:,1]+.5)<.3];axes[1,0].scatter(q[::2,0],q[::2,2],s=1,c=color,label=title)
  q=cloud[np.abs(cloud[:,0]+2)<.3];axes[1,1].scatter(q[::2,1],q[::2,2],s=1,c=color,label=title)
 axes[1,0].set_title('Longitudinal section Y=-0.5±0.3 m');axes[1,1].set_title('Transverse section X=-2±0.3 m')
 for ax in axes[1]:ax.legend();ax.set_ylabel('Z, m');ax.grid(alpha=.2)
 fig.suptitle(f'Measurement {id} — baseline alignment');fig.savefig(folder/'sections.png',dpi=130);plt.close(fig)
print('Saved section plots')
