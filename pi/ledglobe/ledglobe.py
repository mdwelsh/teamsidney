#!/usr/local/bin/python3

from kivy.app import App
from kivy.uix.carousel import Carousel
from kivy.uix.image import AsyncImage
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.label import Label


class CarouselApp(App):
    def build(self):
        carousel = Carousel(direction='right')
        carousel.loop = True

        layout1 = BoxLayout(orientation='vertical')
        label1 = Label(text='Earth')
        layout1.add_widget(label1)
        image1 = AsyncImage(source='https://media.npr.org/assets/img/2015/02/03/globe_west_2048_sq-3c11e252772de81daba7366935eb7bd4512036b8-s800-c85.jpg', allow_stretch=True)
        layout1.add_widget(image1)

        layout2 = BoxLayout(orientation='vertical')
        label2 = Label(text='Moon')
        layout2.add_widget(label2)
        image2 = AsyncImage(source='https://upload.wikimedia.org/wikipedia/commons/thumb/e/e1/FullMoon2010.jpg/1200px-FullMoon2010.jpg', allow_stretch=True)
        layout2.add_widget(image2)

        carousel.add_widget(layout1)
        carousel.add_widget(layout2)
        return carousel


CarouselApp().run()
