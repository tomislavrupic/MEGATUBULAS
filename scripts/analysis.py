#!/usr/bin/env python3
"""Reproduce SVG measurements using numpy. No plotting GUI or cloud dependency."""
from pathlib import Path
import numpy as np
import json, html
root=Path(__file__).resolve().parents[1]/'analysis'
a=np.genfromtxt(root/'measurements.csv',delimiter=',',names=True)
fs=48000
colors=['#e5a349','#ac80ff','#63dfff','#aaaaaa']
def plot(name,title,curves,xlabel,ylabel,xlimits,ylimits):
    W,H=1100,440;l,r,t,b=80,30,45,65
    def px(x):return l+(x-xlimits[0])/(xlimits[1]-xlimits[0])*(W-l-r)
    def py(y):return H-b-(y-ylimits[0])/(ylimits[1]-ylimits[0])*(H-t-b)
    s=[f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}"><rect width="100%" height="100%" fill="#151719"/><style>text{{font:13px sans-serif;fill:#e9dfce}}</style>',f'<text x="80" y="25">{html.escape(title)}</text>']
    for x in np.linspace(*xlimits,7):s += [f'<path d="M {px(x)} {t} V {H-b}" stroke="#393b3e"/>',f'<text x="{px(x)}" y="{H-b+22}">{x:g}</text>']
    for y in np.linspace(*ylimits,6):s += [f'<path d="M {l} {py(y)} H {W-r}" stroke="#393b3e"/>',f'<text x="10" y="{py(y)}">{y:g}</text>']
    for i,(label,x,y) in enumerate(curves):
        color=colors[i%len(colors)];points=' '.join(f'{px(xx):.1f},{py(yy):.1f}' for xx,yy in zip(x,np.clip(y,*ylimits)))
        s += [f'<polyline points="{points}" stroke="{color}" fill="none" stroke-width="1.3"/>',f'<text x="{l+260*i}" y="{H-10}" style="fill:{color}">{html.escape(label)}</text>']
    s += [f'<text x="{W/2}" y="{H-32}">{html.escape(xlabel)}</text>',f'<text x="10" y="25">{html.escape(ylabel)}</text></svg>']
    (root/name).write_text('\n'.join(s))
def spectrum(y):
    y=y[-32768:];z=np.fft.rfft(y*np.hanning(len(y)));return np.fft.rfftfreq(len(y),1/fs),20*np.log10(np.maximum(np.abs(z)/(len(y)/4),1e-9))
curves=[]
for k in ['source','wet4','wet8','reference32']:
    x,y=spectrum(a[k]);idx=np.arange(0,len(x),4);curves.append((k,x[idx],y[idx]))
plot('spectrum.svg','Two-tone spectrum: 701 Hz + 7001 Hz, Drive 80, Memory 80, Coupling 80',curves,'Frequency (Hz)','dBFS',(0,24000),(-140,0))
curves=[]
for k in ['wet4','wet8','blend50']:
    y=a[k];x=np.arange(600,1000)/fs*1000;curves.append((k,x,y[600:1000]))
plot('blend.svg','Aligned dry/wet phase illustration (nonlinear two-tone signal)',curves,'Time (ms)','Amplitude',(12.5,20.8),(-.5,.5))
y=a['burst'];size=480;env=np.sqrt(np.mean(y[:len(y)//size*size].reshape(-1,size)**2,axis=1));x=(np.arange(len(env))+.5)*size/fs
plot('recovery.svg','Burst to quiet probe: state-dependent recovery, Drive 80, Memory 80', [('Output RMS',x,20*np.log10(np.maximum(env,1e-9)))],'Time (s)','dBFS',(0,1.365),(-45,0))
# Align each render to its measured latency before comparing to the 32x render.
meta=(root/'measurement-meta.txt').read_text();ref_latency=int(meta.split('latency: ')[1].split()[0])
metrics={}
for key,latency in [('wet4',61),('wet8',65)]:
    n=min(len(a)-latency,len(a)-ref_latency);signal=a[key][latency:latency+n];reference=a['reference32'][ref_latency:ref_latency+n];residual=signal[4096:]-reference[4096:]
    metrics[key+'_aligned_reference_residual_rms_dbfs']=float(20*np.log10(np.sqrt(np.mean(residual**2))+1e-20))
metrics['note']='Residual includes conversion-filter and state-discretisation differences, not a pure alias-only measurement. No universal quality threshold is applied.'
(root/'metrics.json').write_text(json.dumps(metrics,indent=2))
print(json.dumps(metrics,indent=2))

# Low/mid/high input-level harmonic sweeps, using the settled final half second.
h=np.genfromtxt(root/'harmonics.csv',delimiter=',',names=True)
for quality in [4,8]:
    curves=[]
    for prefix,label in [('low','-30 dBFS input'),('medium','-15 dBFS input'),('high','-3 dBFS input')]:
        x,y=spectrum(h[prefix+str(quality)][-16384:]);idx=np.arange(0,len(x),2);curves.append((label,x[idx],y[idx]))
    plot(f'harmonics-{quality}x.svg',f'997 Hz harmonic spectra / {quality}x / Drive 80 / Warm',curves,'Frequency (Hz)','dBFS',(0,16000),(-140,0))
