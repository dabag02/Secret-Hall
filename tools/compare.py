#!/usr/bin/env python3
"""Confronto di screenshot.
  compare.py <dirA> <dirB> [--tol N]   confronta tutti i PNG di A con quelli di B (pixel per pixel); exit 1 se differiscono
  compare.py --topng <dir>             converte i .ppm in .png (e li elimina)
--tol N: differenza massima ammessa per canale (default 0 = identici bit a bit).
Per ogni scenario diverso scrive <dirB>/diff_<nome>.png (le differenze amplificate)."""
import sys, os, glob
from PIL import Image, ImageChops

def topng(d):
    for p in sorted(glob.glob(os.path.join(d, "*.ppm"))):
        Image.open(p).convert("RGB").save(p[:-4] + ".png"); os.remove(p)

def main(argv):
    if len(argv) >= 2 and argv[0] == "--topng":
        topng(argv[1]); return 0
    tol = 0
    if "--tol" in argv:
        i = argv.index("--tol"); tol = int(argv[i + 1]); del argv[i:i + 2]
    if len(argv) != 2:
        print(__doc__); return 2
    a, b = argv
    bad = 0; n = 0
    for pa in sorted(glob.glob(os.path.join(a, "*.png"))):
        name = os.path.basename(pa)
        if name.startswith("diff_"): continue
        n += 1; pb = os.path.join(b, name)
        if not os.path.exists(pb):
            print(f"MANCA   {name}"); bad += 1; continue
        ia, ib = Image.open(pa).convert("RGB"), Image.open(pb).convert("RGB")
        if ia.size != ib.size:
            print(f"DIMENS. {name} {ia.size} != {ib.size}"); bad += 1; continue
        diff = ImageChops.difference(ia, ib)
        if diff.getbbox() is None:
            print(f"ok      {name}"); continue
        mx = max(h for h in (max(i for i, c in enumerate(ch) if c) for ch in
                 (diff.getchannel(k).histogram() for k in range(3))))
        npx = sum(1 for px in diff.getdata() if max(px) > tol)
        if mx <= tol:
            print(f"ok(tol) {name}  diff max {mx} <= {tol}"); continue
        bad += 1
        print(f"DIVERSO {name}  diff max {mx}, pixel oltre soglia: {npx}")
        diff.point(lambda v: min(255, v * 8)).save(os.path.join(b, "diff_" + name))
    print(f"\n{n - bad}/{n} scenari uguali" + ("" if bad == 0 else f", {bad} DIVERSI"))
    return 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
