from pathlib import Path
import sys
r=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(r/'.tools/science327'))
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
for number in [14375,14365,14372,14387]:
    p=r/f'build/control332/cloud{number}'
    if not (p/'empty.xyz').exists(): continue
    a=np.fromfile(p/'full.xyz',dtype='<f4').reshape(-1,3)/1000
    b=np.fromfile(p/'empty.xyz',dtype='<f4').reshape(-1,3)/1000
    fig,axes=plt.subplots(2,3,figsize=(15,8),layout='constrained')
    for cloud,ax,title in [(a,axes[0,0],'Full'),(b,axes[0,1],'Empty aligned')]:
        s=cloud[::3];ax.scatter(s[:,0],s[:,1],c=s[:,2],s=1,cmap='turbo');ax.set_title(title);ax.set_aspect('equal')
    for cloud,color,title in [(a,'orange','Full'),(b,'deepskyblue','Empty')]:
        q=cloud[np.abs(cloud[:,1]-np.median(b[:,1]))<.15]
        axes[0,2].scatter(q[:,0],q[:,2],s=1,c=color,label=title)
        for x,ax in zip(np.quantile(b[:,0],[.25,.5,.75]),axes[1]):
            q=cloud[np.abs(cloud[:,0]-x)<.15];ax.scatter(q[:,1],q[:,2],s=1,c=color);ax.set_title(f'X={x:.2f}±0.15 m')
    axes[0,2].set_title('Longitudinal section');axes[0,2].legend()
    for ax in axes.flat: ax.grid(alpha=.2)
    fig.suptitle(f'{number}: existing alignment');fig.savefig(p/'sections.png',dpi=110);plt.close(fig)
print('Saved plots')
