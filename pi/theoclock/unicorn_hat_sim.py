import sys
import colorsys
import pygame.gfxdraw

try:
    import pygame
except ImportError:
    print("To simulate a unicorn HAT on your computer, please pip install pygame")

class UnicornHatSim(object):
    def __init__(self, width, height):
        # Compat with old library
        self.AUTO = None
        self.PHAT = None
            
        # Set some defaults
        self.pixels = [[(0, 0, 0) for x in range(width)] for y in range(height)]
        self.pixel_size = 15
        self.width = width
        self.height = height
        self.window_width = width * self.pixel_size
        self.window_height = height * self.pixel_size

        # Init pygame and off we go
        pygame.init()
        pygame.display.set_caption("Unicorn HAT simulator")
        self.screen = pygame.display.set_mode([self.window_width, self.window_height])
        self.clear()

    def set_pixel(self, x, y, r, g, b):
        self.pixels[(self.width-1)-x][y] = [int(r), int(g), int(b)]

    def draw(self):
        for event in pygame.event.get(): # User did something
            if event.type == pygame.QUIT:
                print("Exiting...")
                sys.exit()

        for x in range(self.width):
            for y in range(self.height):
                self.draw_led(x, y)

    def show(self):
        pygame.display.flip()
        self.clear()
        self.draw()
        pygame.display.flip()

    def draw_led(self, x, y):
        self.draw_gfxcircle(x,y)

    def draw_gfxcircle(self, x, y):
        p = self.pixel_size
        w_x = int(x * p + self.pixel_size / 2)
        w_y = int(y * p + self.pixel_size / 2)
        r = int(self.pixel_size / 4)
        color = self.pixels[x][y]
        pygame.gfxdraw.aacircle(self.screen, w_x, w_y, r, color)
        pygame.gfxdraw.filled_circle(self.screen, w_x, w_y, r, color)

    def get_shape(self):
        return (self.width, self.height)

    def brightness(self, *args):
        pass

    def rotation(self, r):
        pass

    def clear(self):
        self.screen.fill((0, 0, 0))

    def get_rotation(self):
        return 0

    def set_layout(self, *args):
        pass

    def set_pixel_hsv(self, x, y, h, s=1.0, v=1.0):
        r, g, b = [int(n*255) for n in colorsys.hsv_to_rgb(h, s, v)]
        self.set_pixel(x, y, r, g, b)

    def off(self):
        print("Closing window")
        pygame.quit()

# SD hats works as expected
#unicornhat = UnicornHatSim(8,8)
#unicornphat = UnicornHatSim(8, 4)

unicornhathd = UnicornHatSim(16, 16)

