//#declare rock_color=<0.55,0.45,0.3>; // Mandatory
//#declare rock_color=<0,0.35,0>; // Mandatory
//#include "maprock1.inc"
#include "colors.inc"
#include "metals.inc"
#include "pos.inc"

//#declare pos = <10,0,10>;
//#declare filename = "workpart0000.pgm";

// PCB size
#declare w = 23;
#declare h = 23;
#declare th = 1.6;

camera {
    //location <0,10,-4>
    //look_at <0,0,-1>
    location <0,15,-25>
    look_at <0,0,0>
}

// gewöhnliche Lichtquelle von links
light_source { <-140,200,300> rgb 1.5}
// schatztenloses Füll-Licht von rechts
light_source { <140,200,-300> rgb 0.9 shadowless}

#declare Chain_Copper = texture {
    pigment {
        gradient <0,1,0>
        color_map {
            [0  color rgb<1,1,1>]
            [0.25  color rgb<1,1,1>]
            [0.5  color rgb<1,1,1>]
            [1  color P_Copper1]
        }
    }
    finish {
        ambient .1
        diffuse .4
        reflection .25
        specular 1
        metallic
    }
}

/* PCB surounding box for heightfield */
difference {
    box {
        <0,0,0><1,1,1>
        pigment { color White }
        scale <w,th,h>
        translate<-w/2,-th,-h/2>
    }
    box {
        <0,0,0><1,1,1>
        pigment { color White }
        scale <w-0.1,th,h-0.1>
        translate<-w/2+0.05,-th+0.15,-h/2+0.05>
    }
}

// Heightfield mit cells-pigment als
// interner bitmap auf <0,0,0> bis <<1,1,0>
height_field {
    png filename
    //pgm "../workpart.pgm"
    //water_level 0.5
    smooth
    //texture {pigment {gradient y color_map{rock_map} scale 1} }
    texture { Chain_Copper }
    scale <w,th,h>
    translate<-w/2,-1.55,-h/2>
}

#declare F1=function {
    pattern {
        density_file df3 toolfile
        interpolate 0
    }
}

/* tool */
#declare tool_x=tooldiameter;
#declare tool_y=tooldiameter;
#declare tool_z=5;

isosurface {
    function {
        0.005 - F1(x,z,y)
    }
    contained_by { box { <0,0,0><1,1,1> } }
    pigment { rgb 0.8 }
    scale<tool_x,tool_z,tool_y>
    // move tool to pcb origin
    translate <-w/2,0,-h/2>
    // center tool
    translate <-tool_x/2, 0, -tool_y/2>
    // move relative to current Gcode position
    translate pos
}

/*
box {
    <0,0,0><1,1,1>
    hollow
    pigment {rgbt <1,0,0,0.8>}
    scale<tool_x,tool_z,tool_y>
    // move tool to pcb origin
    translate <-w/2,0,-h/2>
    // center tool
    translate <-tool_x/2, 0, -tool_y/2>
    // move relative to current Gcode position
    translate pos
}
*/

/*
box {
    <0,0,0><1,1,1>
    hollow
    pigment {rgbt 1}
    interior {
        media {
            emission 0.0
            scattering {1, 0.9}
            density{
                density_file df3 "../tool.d3f"
                interpolate 0
                color_map {
                    [0.0 rgb <0.0, 0.0, 0.0>]
                    [1.0 rgb <1, 1, 1>]
                }
            }
        }
    }
    scale<tool_x,tool_y,tool_z>
    rotate <-90,0,0>
    // move tool to pcb origin
    translate <-w/2,0,-h/2>
    // center tool
    translate <-tool_x/2, 0, tool_y/2>
    // move relative to current Gcode position
    translate pos
}
*/

// wood table
plane{y,-1.6
    texture{
        pigment{image_map{jpeg "wood-texture0020-e1447163908131-1024x765.jpg"}} // use any jpg of your choice
        normal{bump_map{jpeg "wood-texture0020-e1447163908131-1024x765.jpg"}}
        finish{reflection{0,.1}}
        scale 100
    }
    clipped_by {
        sphere { 0, 100 }
    }
}

