"""Lossless-source icon conversion; pip install Pillow before packaging."""
from pathlib import Path
from PIL import Image, ImageOps

root = Path(__file__).resolve().parents[1]
out = root / 'res' / 'appIcon'
out.mkdir(parents=True, exist_ok=True)
flag = Image.open(root / 'res' / 'flag-bhs.png').convert('RGBA')
icon = Image.new('RGBA', (1024, 1024))
flag = ImageOps.contain(flag, (1024, 1024), Image.Resampling.LANCZOS)
icon.alpha_composite(flag, ((1024-flag.width)//2, (1024-flag.height)//2))
icon.save(out / 'winApp.ico', sizes=[(s,s) for s in (16,24,32,48,64,128,256)])
icon.save(out / 'macApp.icns')
for size in (16,32,48,128,256):
    icon.resize((size,size), Image.Resampling.LANCZOS).save(out / f'lnxApp{size}.png')
