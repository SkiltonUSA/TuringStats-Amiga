#!/usr/bin/env python3
"""Convert Amiga CAPTURE buffers (RGB565 little endian, 320x480) to PNG."""
from pathlib import Path
import argparse
from PIL import Image
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('destination',type=Path)
a=p.parse_args();a.destination.mkdir(parents=True,exist_ok=True)
for path in sorted(a.source.glob('page-*.rgb565')):
 data=path.read_bytes()
 if len(data)!=320*480*2:raise ValueError(f'Invalid framebuffer size: {path}')
 pixels=bytearray()
 for i in range(0,len(data),2):
  v=data[i]|data[i+1]<<8
  pixels.extend((((v>>11)&31)*255//31,((v>>5)&63)*255//63,(v&31)*255//31))
 Image.frombytes('RGB',(320,480),bytes(pixels)).save(a.destination/(path.stem+'.png'))
 print(path.name)
