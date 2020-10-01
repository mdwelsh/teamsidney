camera { location <0, 5, -10> look_at 0 angle 35 }
light_source
{ <100, 200, -150>/50, 1
  fade_distance 6 fade_power 2
  area_light x*3, y*3, 12, 12 circular orient adaptive 0
}

#declare PlankNormal =
  normal
  { gradient x 2 slope_map { [0 <0,1>][.05 <1,0>][.95 <1,0>][1 <0,-1>] }
    scale 2
  };

plane
{ y, -.25
  //pigment { rgb <.7,.95,1> }
  pigment
  { wood color_map
    { [.4 rgb <.9, .7, .4>]
      [.6 rgb <1, .8, .6>]
    }
    turbulence .5
    scale <1, 1, 10>*.5
    rotate y*20
  }
  normal
  { average normal_map
    { [1 PlankNormal]
      [1 wood .5 slope_map { [0 <0,0>][.5 <.5,1>][1 <1,0>] }
         turbulence .5 scale <1, 1, 10>*.5]
    }
    rotate y*20
  }
  finish { specular .5 reflection .2 }
}

#declare Metal =
  pigment
  { bozo color_map
    { [0 rgb <.8, .75, .5>]
      [.6 rgb <.8, .75, .5>]
      [.7 rgb <.7, .4, .2>]
      [1 rgb <.5, .35, .15>]
    }
    turbulence .4
    scale .2
  }

#declare Cyl1 =
  cylinder
  { -x, x, .25
    pigment { Metal }
    finish { specular .5 reflection { .3, .6 } }
    normal
    { gradient x .1
      slope_map
      { [0 <1, 0>][.5 <0, -1>][.5 <0, 1>][1 <1, 0>]
      }
      scale <.1, 0, 0>
    }
  };

#declare Cyl2 =
  cylinder
  { -y*.25, 0, .3
    pigment { rgb <.7, .75, .8> }
    finish { specular .5 reflection { .3, .6 } }
    normal
    { radial .2
      slope_map
      { [0 <1, 0>][.5 <0, -1>][.5 <0, 1>][1 <1, 0>]
      }
      frequency 20
    }
  };

object { Cyl1 rotate y*-25 }
object { Cyl1 rotate x*20+y*-60 translate <-2, 0, 1> }
object { Cyl1 rotate x*50+y*50 translate <1.5, 0, 2.5> }
object { Cyl1 rotate x*80+y*-130 translate <-.5, 0, -2.5> }
object { Cyl1 rotate x*130+y*160 translate <.5, 0, 4.5> }
object { Cyl1 rotate x*200+y*20 translate <3, 0, -1> }

object { Cyl2 translate <1.2, 0, -2.3> }
object { Cyl2 rotate y*35 translate <-2, 0, -1.5> }
object { Cyl2 rotate y*105 translate <.5, 0, 2> }
object { Cyl2 rotate y*5 translate <-2.7, 0, 2.2> }
object { Cyl2 translate x*.3 rotate z*50 translate <-1.5, 0, -.4> }
