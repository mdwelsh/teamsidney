

./render.py > test.ldr

./ldraw/tools/ldr2png.py ~/src/github/teamsidney/brickfinder/render/test.ldr --stroke-colour '#000000' foo.png 10240x7680 2000,2000,-2000

povray -ifoo.pov +W12800 +H10240 +fp -o-  > o.png

