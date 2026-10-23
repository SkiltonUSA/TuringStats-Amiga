#!/usr/bin/env python3
"""Package locally built executables and generated Workbench icons."""
from pathlib import Path
import argparse, hashlib, os, shutil, subprocess, zipfile
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--lha',help='optional path to an LHa archiver');a=p.parse_args()
name='TuringStats-0.1.0-Beta1';dist=root/'dist';stage=dist/'stage';app=stage/name
if stage.exists():shutil.rmtree(stage)
(app/'Files').mkdir(parents=True)
for f in ['TuringStats','TuringStatsLaunch']:shutil.copyfile(root/'build'/f,app/'Files'/f)
for f in ['Start','Stop']:shutil.copyfile(root/'amiga/turing'/f,app/'Files'/f)
for f in ['Install','Enable-Autostart']:shutil.copyfile(root/'amiga/turing'/f,app/f)
for f in ['App.info','Start.info','Stop.info','Drawer.info']:shutil.copyfile(root/'packaging/icons'/f,app/'Files'/f)
for f in ['Install','Enable-Autostart']:shutil.copyfile(app/'Files/Start.info',app/(f+'.info'))
shutil.copyfile(app/'Files/Drawer.info',stage/(name+'.info'))
shutil.copyfile(root/'packaging/ReadMe.txt',app/'ReadMe.txt')
shutil.copyfile(root/'COPYING',app/'COPYING');shutil.copyfile(root/'CHANGELOG.md',app/'Changes.txt')
shutil.copytree(root/'docs/screenshots',app/'Screenshots')
for f in app.rglob('*'):
 if f.is_file():f.chmod(0o755 if f.name in ['Install','Enable-Autostart','Start','Stop','TuringStats','TuringStatsLaunch'] else 0o644)
archives=[dist/(name+'.zip')]
with zipfile.ZipFile(archives[0],'w',zipfile.ZIP_DEFLATED) as z:
 for f in sorted(stage.rglob('*')):
  if f.is_file():z.write(f,f.relative_to(stage))
if a.lha:
 archive=dist/(name+'.lha');archive.unlink(missing_ok=True)
 subprocess.run([str(Path(a.lha).resolve()),'aq',str(archive),name,name+'.info'],cwd=stage,check=True)
 archives.append(archive)
(dist/'SHA256SUMS').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.name+'\n' for f in archives))
for f in archives:print(f.name,f.stat().st_size)
