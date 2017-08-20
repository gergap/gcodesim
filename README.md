# gcodesim
GCode simulator

This tool is designed to simulate GCode exported from Eagle pcb-gcode plugin
to simulate PCB milling.

Have a look at http://gergap.de/gcode-simulator.html for more information.

# Requirements

This is a pure C program, so all you need is GCC. No dependencies to any
library. For building I use CMake, but `gcc -O2 -o gcodesim *.c` would also do it.

* GCC
* CMake

# Building

Run "cmake && make" or use the provided `./build.sh` script.

# Usage

* Create your board design using Autodesk Eagle (or the old Cadsoft version).
* Export GCode using the Eagle plugin pcb-gcode (http://www.pcbgcode.org)
  - Enable "Nc File Comment Machine Settings" in the GCode Options tab.
* Use gcodesim to simulate the code

# Example: Simulation

This assumes you've created a PCB of size 30x30mm. The tool information is stored
inside the GCode comments. So these setting will be read from gcodesim
automatically.

Lets further assume you've exported the bottom outline and bottom drills and you
want to simulate both.

    ./gcodesim -m -W 30 -H 30 demo.bot.etch.gcode demo.bot.drill.gcode

This will create the file `workpart.pgm` which can be viewed in most graphics
viewers/editors like e.g. KDE Gwenview or Gimp. You can also easily convert it
on console using *Image Magick*: `convert workpart.pgm workpart.png`

# Example: Rewriting GCode

Lets assume the first try didn't work as expected and you want to try it again,
but this time at an offset X=-35mm. You can rewrite the GCode applying the given
offset.

    ./gcodesim -m -W 30 -H 30 -x-35 -O etch.gcode demo.bot.etch.gcode
    ./gcodesim -m -W 30 -H 30 -x-35 -O dril.gcode demo.bot.dril.gcode

Now simulate again the result on a wider board:

    ./gcodesim -m -W 70 -H 30 etch.gcode drill.gcode

# Custom tools

If you have GCode without the tool headers you can use the provided functions in
main.c `create_etch_tool` and `create_drill_tool` to create the tools necessary
in `main()`. Then use the `-t <num>` commandline option to select the tool you
want to use for the given file.

# Commandline arguments

Use `-h` to show the built-in help:

    $ ./gcodesim -h
    GCodesim Copyright (C) 2017 Gerhard Gappmeier
    This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
    This is free software, and you are welcome to redistribute it
    under certain conditions; type `show c' for details.
    
    Usage: ./bld/gcodesim [options] file...
    Options:
      -h: Shows this help
      -v: Verbose output
      -r: Specifies size of one voxel in mm (default=0.1mm)
      -W: Specifies PCB width in mm (default=100mm)
      -H: Specifies PCB height in mm (default=80mm)
      -r: Specifies size of one voxel in mm (default=0.1mm)
      -t: Specifies tool index to use (if no tool selection is in the Gcode)
      -o: Specifies output filname, to rewrite the given GCode files
      -x: Applies given offset in mm at X axis
      -y: Applies given offset in mm at Y axis
      -z: Applies given offset in mm at Z axis
      -m: PCBGcode mirror compensation. Makes the coordinates positive by adding the PCB width.
    Example: ./gcodesim -W 30 -m -x-5 -o drill.gcode ~/eagle/isp_adapter/isp_adapter.bot.drill.gcode


