#!/usr/bin/env python3
"""Crop each book figure to its ink and give it an opaque white backdrop.

The book's SVGs are Inkscape exports on full letter/A4 canvases, so most of
them are a small drawing in the corner of a mostly empty page. Rendered in a
markdown file that reads as a huge blank box. This measures where the ink
actually is, rewrites the root viewBox around it, and inserts a white rect so
black line art stays legible on GitHub's dark theme.

Requires cairosvg and Pillow, neither of which anything else in this tree
needs; see README.md for the one command that reproduces every figure here.

Usage: prep-figures.py <src-dir> <dst-dir> <name.svg> [...]
"""

import io
import re
import sys

import cairosvg
from PIL import Image

RENDER_SCALE = 2.0        # measure at 2x for a tighter bbox
PAD_FRACTION = 0.02       # padding as a fraction of the larger cropped side
MAX_INTRINSIC = 900.0     # cap the declared pixel size of the result

UNITS = {"in": 96.0, "mm": 96.0 / 25.4, "cm": 96.0 / 2.54, "pt": 96.0 / 72.0,
         "pc": 16.0, "px": 1.0, "": 1.0}


def to_px(value):
    m = re.fullmatch(r"\s*([0-9.eE+-]+)\s*([a-z%]*)\s*", value)
    number, unit = float(m.group(1)), m.group(2)
    return number * UNITS[unit]


def root_tag(svg):
    m = re.search(r"<svg\b.*?>", svg, re.S)
    if not m:
        raise SystemExit("no <svg> element")
    return m


def attr(tag, name):
    m = re.search(r'\b' + name + r'\s*=\s*"([^"]*)"', tag)
    return m.group(1) if m else None


def user_box(tag):
    """The figure's user-unit coordinate box, and px-per-user-unit."""
    vb = attr(tag, "viewBox")
    w_px = to_px(attr(tag, "width"))
    h_px = to_px(attr(tag, "height"))
    if vb:
        x, y, w, h = (float(v) for v in re.split(r"[\s,]+", vb.strip()))
    else:
        x, y, w, h = 0.0, 0.0, w_px, h_px
    return (x, y, w, h), (w_px / w, h_px / h)


def ink_box(svg, box):
    """Bounding box of the non-white pixels, in user units.

    Markers are stripped before rendering: cairosvg raises on a zero-width
    marker scale in several of these files, and a marker head never extends
    the drawing beyond the path it decorates.
    """
    measurable = re.sub(r"marker-(start|mid|end)\s*:\s*url\(#[^)]*\)\s*;?", "", svg)
    png = cairosvg.svg2png(bytestring=measurable.encode(), scale=RENDER_SCALE,
                           background_color="white")
    image = Image.open(io.BytesIO(png)).convert("RGB")
    inverted = Image.eval(image, lambda v: 255 - v)
    bbox = inverted.getbbox()
    if bbox is None:
        raise SystemExit("the render is blank")

    x0, y0, w0, h0 = box
    sx = image.width / w0
    sy = image.height / h0
    left, upper, right, lower = bbox
    return (x0 + left / sx, y0 + upper / sy,
            (right - left) / sx, (lower - upper) / sy)


def rewrite(svg, cropped, ppu):
    x, y, w, h = cropped
    pad = max(w, h) * PAD_FRACTION
    x, y, w, h = x - pad, y - pad, w + 2 * pad, h + 2 * pad

    px_w, px_h = w * ppu[0], h * ppu[1]
    if max(px_w, px_h) > MAX_INTRINSIC:
        shrink = MAX_INTRINSIC / max(px_w, px_h)
        px_w, px_h = px_w * shrink, px_h * shrink

    tag = root_tag(svg).group(0)
    new = tag
    for name, value in (("width", f"{px_w:.2f}px"), ("height", f"{px_h:.2f}px"),
                        ("viewBox", f"{x:.3f} {y:.3f} {w:.3f} {h:.3f}")):
        if re.search(r'\b' + name + r'\s*=\s*"[^"]*"', new):
            new = re.sub(r'\b' + name + r'\s*=\s*"[^"]*"', f'{name}="{value}"', new, count=1)
        else:
            new = new[:-1].rstrip() + f'\n   {name}="{value}">'

    backdrop = (f'<rect x="{x:.3f}" y="{y:.3f}" width="{w:.3f}" height="{h:.3f}" '
                f'fill="#ffffff" stroke="none" />')
    return svg.replace(tag, new + "\n" + backdrop, 1)


def main():
    src, dst, names = sys.argv[1], sys.argv[2], sys.argv[3:]
    for name in names:
        svg = open(f"{src}/{name}").read()
        box, ppu = user_box(root_tag(svg).group(0))
        cropped = ink_box(svg, box)
        open(f"{dst}/{name}", "w").write(rewrite(svg, cropped, ppu))
        print(f"{name:22s} {box[2]:8.1f}x{box[3]:8.1f} -> "
              f"{cropped[2]:8.1f}x{cropped[3]:8.1f} user units")


main()
