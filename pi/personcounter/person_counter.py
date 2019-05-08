#!/usr/bin/env python3

import argparse
import datetime
from pprint import pprint
import time
import random
import sys
import unicornhathd

try:
    from PIL import Image
except ImportError:
    exit('This script requires the pillow module\nInstall with: sudo pip install pillow')

try:
    import picamera
except ImportError:
    sys.exit("Requires picamera module. "
             "Please install it with pip:\n\n"
             "   pip3 install picamera\n"
             "(drop the --user if you are using a virtualenv)")

try:
    import xnornet
except ImportError:
    sys.exit("The xnornet wheel is not installed.  "
             "Please install it with pip:\n\n"
             "    python3 -m pip install --user xnornet-<...>.whl\n")

# Generate fake data for testing.
FAKE_DATA = True

# Input resolution
INPUT_RES = 0
# Constant frame size
SINGLE_FRAME_SIZE_RGB = 0
SINGLE_FRAME_SIZE_YUV = 0
YUV420P_Y_PLANE_SIZE = 0
YUV420P_U_PLANE_SIZE = 0
YUV420P_V_PLANE_SIZE = 0

# Unicorn Hat HD size.
WIDTH, HEIGHT = unicornhathd.get_shape()


def showImage(image):
    """Draw the given Pillow image on the Unicorn Hat HD."""
    width, height = image.size
    print("Image size {}x{}".format(width, height))
    for x in range(min(WIDTH, width)):
        for y in range(min(HEIGHT, height)):
            r, g, b = image.getpixel((x, y))
            print("Setting {},{} to {}/{}/{}".format(x, y, r, g, b))
            unicornhathd.set_pixel(x, y, r, g, b)
    unicornhathd.show()


def interpolate(color1, color2, mix):
    """Interpolate between color1 and color2."""
    r = (color1[0] * (1.0 - mix)) + (color2[0] * mix)
    g = (color1[1] * (1.0 - mix)) + (color2[1] * mix)
    b = (color1[2] * (1.0 - mix)) + (color2[2] * mix)
    return (r, g, b)


class PixelFont:
    def __init__(self, filename):
        self.glyphs = []
        im = Image.open(filename)
        image = im.convert('RGB')
        width, height = image.size
        delimiter = image.getpixel((0, 0))
        last_x = 0
        for x in range(width):
            p = image.getpixel((x, 0))
            if (x > 0 and p == delimiter) or (x == width-1):
                # Make a glyph from the last chunk.
                c = image.crop((last_x, 1, x-1, height))
                self.glyphs.append(c)
                last_x = x

    def glyph(self, c):
        # These fonts generally start with the '!' character.
        index = ord(c) - ord('!')
        if index < 0 or index > len(self.glyphs):
            return self.glyph('?')
        return self.glyphs[index]

    def drawString(self, string, x, y):
        iw = 0
        ih = 0
        for c in string:
            glyph = self.glyph(c)
            w, h = glyph.size
            iw += w
            if h > ih:
                ih = h
        im = Image.new('RGB', (iw, ih))
        for c in string:
            glyph = self.glyph(c)
            w, h = glyph.size
            print("Glyph: {}".format(list(glyph.getdata())))
            im.paste(glyph, box=(x, y))
            x += w
        print("Final image: {}".format(list(im.getdata())))
        return im



class Plotter:
  def __init__(self, width, height, history_size=100):
    self.width = width
    self.height = height
    self.history_size = history_size
    self.values = []

  def update(self, value):
    # We maintain a list of history_size values.
    self.values.append((datetime.datetime.now(), value))
    if len(self.values) > self.history_size:
      self.values.pop(0)

  def drawBargraph(self):
    # Plot the most recent 'width' values.
    last_values = self.values[-self.width:]
    for index, val in enumerate(last_values):
      dt, count = val
      if count > self.height-1:
        count = self.height-1
      for y in range(self.height):
        if y < count:
          color = interpolate((255, 0, 0), (255, 255, 0), (y * 1.0) / self.height)
        else:
          color = (0, 0, 0)
        r, g, b = color
        unicornhathd.set_pixel(index, y, r, g, b)
    unicornhathd.show()

  def draw(self):
      self.drawBargraph()


def _initialize_camera_vars(camera_res):
    global INPUT_RES
    global SINGLE_FRAME_SIZE_RGB
    global SINGLE_FRAME_SIZE_YUV
    global YUV420P_Y_PLANE_SIZE
    global YUV420P_U_PLANE_SIZE
    global YUV420P_V_PLANE_SIZE

    INPUT_RES = camera_res
    SINGLE_FRAME_SIZE_RGB = INPUT_RES[0] * INPUT_RES[1] * 3
    SINGLE_FRAME_SIZE_YUV = INPUT_RES[0] * INPUT_RES[1] * 3 // 2
    YUV420P_Y_PLANE_SIZE = INPUT_RES[0] * INPUT_RES[1]
    YUV420P_U_PLANE_SIZE = YUV420P_Y_PLANE_SIZE // 4
    YUV420P_V_PLANE_SIZE = YUV420P_U_PLANE_SIZE


def _make_argument_parser():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("--camera_frame_rate", action='store', type=int,
                        default=8, help="Adjust the framerate of the camera.")
    parser.add_argument("--camera_brightness", action='store', type=int,
                        default=60, help="Adjust the brightness of the camera.")
    parser.add_argument(
        "--camera_recording_format", action='store', type=str, default='yuv',
        choices={'yuv', 'rgb'},
        help="Changing the camera recording format, \'yuv\' format is "
        "implicitly defaulted to YUV420P.")
    parser.add_argument("--camera_input_resolution", action='store', nargs=2,
                        type=int, default=(512, 512),
                        help="Input Resolution of the camera.")
    return parser


def _get_camera_frame(args, camera, stream):
    # Get the frame from the CircularIO buffer.
    cam_output = stream.getvalue()

    if args.camera_recording_format == 'yuv':
        # The camera has not written anything to the CircularIO yet
        # Thus no frame is been captured
        if len(cam_output) != SINGLE_FRAME_SIZE_YUV:
            return None
        # Split YUV plane
        y_plane = cam_output[0:YUV420P_Y_PLANE_SIZE]
        u_plane = cam_output[YUV420P_Y_PLANE_SIZE:YUV420P_Y_PLANE_SIZE +
                             YUV420P_U_PLANE_SIZE]
        v_plane = cam_output[YUV420P_Y_PLANE_SIZE +
                             YUV420P_U_PLANE_SIZE:SINGLE_FRAME_SIZE_YUV]
        # Passing corresponding YUV plane
        model_input = xnornet.Input.yuv420p_image(INPUT_RES, y_plane,
                                                  u_plane, v_plane)
    elif args.camera_recording_format == 'rgb':
        # The camera has not written anything to the CircularIO yet
        # Thus no frame is been captured
        if len(cam_output) != SINGLE_FRAME_SIZE_RGB:
            return None
        model_input = xnornet.Input.rgb_image(INPUT_RES, cam_output)
    else:
        raise ValueError("Unsupported recording format")

    return model_input


def _inference_loop(args, camera, stream, model):
    plotter = Plotter(WIDTH, HEIGHT)
    while True:
        model_input = _get_camera_frame(args, camera, stream)
        if model_input is not None:
            results = model.evaluate(model_input)
            if FAKE_DATA:
                num_people = random.randint(0, HEIGHT)
            else:
                num_people = len(results)
            print("Detected {} people".format(num_people))
            plotter.update(num_people)
            plotter.draw()
            time.sleep(1.0)


def main(args=None):
    parser = _make_argument_parser()
    args = parser.parse_args(args)

    # Initialize Unicorn Hat HD
    unicornhathd.rotation(0)
    unicornhathd.brightness(1.0)

    font = PixelFont("Solar.png")
    showImage(font.drawString("ABC", 0, 0))
    unicornhathd.show()

    sys.exit(0)

    try:
        camera = picamera.PiCamera()
        camera.resolution = tuple(args.camera_input_resolution)
        _initialize_camera_vars(camera.resolution)

        # Initialize the buffer for picamera to hold the frame
        # https://picamera.readthedocs.io/en/release-1.13/api_streams.html?highlight=PiCameraCircularIO
        if args.camera_recording_format == 'yuv':
            stream = picamera.PiCameraCircularIO(camera,
                                                 size=SINGLE_FRAME_SIZE_YUV)
        elif args.camera_recording_format == 'rgb':
            stream = picamera.PiCameraCircularIO(camera,
                                                 size=SINGLE_FRAME_SIZE_RGB)
        else:
            raise ValueError("Unsupported recording format")

        camera.framerate = args.camera_frame_rate
        camera.brightness = args.camera_brightness
        # Record to the internal CircularIO
        # PiCamera's YUV is YUV420P
        # https://picamera.readthedocs.io/en/release-1.13/recipes2.html#unencoded-image-capture-yuv-format
        camera.start_recording(stream, format=args.camera_recording_format)

        # Load Xnor model from disk.
        model = xnornet.Model.load_built_in()

        _inference_loop(args, camera, stream, model)

    except Exception as e:
        raise e
    finally:
        # For good practice, kill it by ctrl+c anyway.
        camera.stop_recording()
        camera.close()


if __name__ == "__main__":
    main()