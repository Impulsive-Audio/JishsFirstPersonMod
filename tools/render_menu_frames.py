import json,numpy as np
from pathlib import Path
from PIL import Image
for p in Path('build').glob('menu-*.json'):
 d=json.loads(p.read_text());w=int(d['width']);h=int(d['height']);aw=d['atlasWidth'];ah=d['atlasHeight'];tex=np.frombuffer(p.with_suffix('.rgba').read_bytes(),np.uint8).reshape(ah,aw,4)/255.;img=np.zeros((h,w,4));img[:,:,3]=1
 for t in d['commands']:
  v=np.array(t[4:]).reshape(3,5);x0=max(0,int(np.floor(v[:,0].min())),int(t[0]));x1=min(w,int(np.ceil(v[:,0].max())),int(t[2]));y0=max(0,int(np.floor(v[:,1].min())),int(t[1]));y1=min(h,int(np.ceil(v[:,1].max())),int(t[3]));
  if x1<=x0 or y1<=y0:continue
  a,b,c=v[:,:2];den=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
  if abs(den)<1e-8:continue
  yy,xx=np.mgrid[y0:y1,x0:x1];xx=xx+.5;yy=yy+.5
  u=((b[1]-c[1])*(xx-c[0])+(c[0]-b[0])*(yy-c[1]))/den;vv=((c[1]-a[1])*(xx-c[0])+(a[0]-c[0])*(yy-c[1]))/den;z=1-u-vv;mask=(u>=0)&(vv>=0)&(z>=0)
  weights=np.stack([u,vv,z],-1);uv=weights@v[:,2:4];colors=np.array([[(int(k)>>shift)&255 for shift in (0,8,16,24)] for k in v[:,4]])/255.;col=weights@colors;sample=tex[np.clip((uv[:,:,1]*ah).astype(int),0,ah-1),np.clip((uv[:,:,0]*aw).astype(int),0,aw-1)];src=col*sample;alpha=(src[:,:,3]*mask)[:,:,None];dst=img[y0:y1,x0:x1,:3];dst[:]=src[:,:,:3]*alpha+dst*(1-alpha)
 Image.fromarray((img*255).astype(np.uint8)).save(p.with_suffix('.png'))
