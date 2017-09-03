This folder contains GCode generated using Inkscape and the gcodetools
extension. See http://www.cnc-club.ru/forum/viewtopic.php?t=35

The simulation can be generated this way:

    ./gcodesim -W40 -H50 -t1:0.5 ../examples/arc_0001.gcode

The demo.svg includes a star with filled area, which means all the area
should be milled away using a 3mm tool. r=0.5 is a resolution suitable
for this size. Higher resolution take very long.

    ./gcodesim -r 0.5 -W80 -H80 -t1:3 ../examples/demo_0001.gcode

To better see the tool path you can choose a thinner tool.

    ./gcodesim -r 0.5 -W80 -H80 -t1:1 ../examples/demo_0001.gcode

