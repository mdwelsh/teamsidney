#!/usr/bin/env python

import argparse
import os
import random
import subprocess
import tempfile

from ldraw.figure import *
from ldraw.library.colours import *
from ldraw.library.parts.brick import *
from ldraw.pieces import Piece
from ldraw.tools import get_model
from ldraw.writers.povray import POVRayWriter


PARTS = [
    Brick1X1,
    Brick1X2,
    Brick1X3,
    Brick1X6,
    Brick2X2,
    Brick2X3,
    Brick2X4,
]

NUM_COLORS=1
COLORS = list(ColoursByCode.values())[0:NUM_COLORS]


POV_TRAILER = """
light_source
{ <0, 1900, 0>, 1
  fade_distance 1600 fade_power 2
  area_light x*70, y*70, 20, 20 circular orient adaptive 0 jitter
}

light_source
{ <2000, 1700, 0>, <1,.8,.4>
  fade_distance 1000 fade_power 2
  area_light x*70, y*70, 20, 20 circular orient adaptive 0 jitter
}

background { color Black }

camera {
  location <-200.000000, 200.000000, -200.000000>
  look_at <0.000000, 0.000000, 0.000000>
  angle 40
}
"""


def gen_piece(part, color, x=0, y=0, z=0):
    yrot = random.randint(0, 360) - 180
    zrot = random.randint(0, 360) - 180
    rot = Identity().rotate(x, XAxis).rotate(yrot, YAxis).rotate(zrot, ZAxis)
    return Piece(color, Vector(x, y, z), rot, part)


def gen_pov(ldraw_path, pov_path):
    model, parts = get_model(ldraw_path)

    with open(pov_path, "w") as pov_file:
        pov_file.write('#include "colors.inc"\n\n')
        writer = POVRayWriter(parts, pov_file)
        writer.write(model)
        pov_file.write(POV_TRAILER + "\n")


def run_pov(pov_path, image_path, image_width=1024, image_height=768):
    cmd = [
        "povray",
        f"-i{pov_path}",
        f"+W{image_width}",
        f"+H{image_height}",
        "+FN",
        f"-o{image_path}",
    ]
    result = subprocess.run(cmd, capture_output=True)
    result.check_returncode()


def gen_image(ldraw_path, image_path):
    print(f"   {image_path}\r", end="")
    with tempfile.NamedTemporaryFile(suffix=".pov", delete=False) as povfile:
        povfile.close()
        gen_pov(ldraw_path, povfile.name)
        run_pov(povfile.name, image_path)


def gen_images(outdir, part, color, num_images):
    retval = []
    for index in range(num_images):
        piece = gen_piece(part, color)
        out_image_filename = os.path.join(outdir, "image{:05d}.jpg".format(index))
        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".ldr", delete=False
        ) as ldrfile:
            ldrfile.write(str(piece) + "\n")
        gen_image(ldrfile.name, out_image_filename)
        retval.append(out_image_filename)
    return retval


def gen_dataset(outdir: str, images_per_part: int):
    os.makedirs(outdir)
    classes_file = open(os.path.join(outdir, "classes.txt"), "w")
    images_file = open(os.path.join(outdir, "images.txt"), "w")
    labels_file = open(os.path.join(outdir, "image_class_labels.txt"), "w")
    class_index = 1
    image_index = 1

    for part in PARTS:
        for color in COLORS:
            classname = f"{part}-{color.name}"
            print(f"\nGenerating class {classname}...")
            classes_file.write(f"{class_index} {classname}\n")
            imagedir = os.path.join(outdir, "images", classname)
            os.makedirs(imagedir)
            images = gen_images(imagedir, part, color, images_per_part)
            for image in images:
                images_file.write(f"{image_index} {image}\n")
                labels_file.write(f"{image_index} {class_index}\n")
                image_index += 1
            class_index += 1
    classes_file.close()
    images_file.close()
    labels_file.close()
    print("\nDone.")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--outdir", help="Output directory for generated dataset.")
    parser.add_argument(
        "--images_per_part", help="Number of output images per class.", type=int
    )
    args = parser.parse_args()
    gen_dataset(args.outdir, args.images_per_part)


if __name__ == "__main__":
    main()
