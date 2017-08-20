
(Custom header for Mendel90)
G1 Z10 F200                  (move to save Z height)
G28 X Y                      (home XY)
G00 X50.0000 Y10.0000 F5000  (move to left bottom edge of the PCB)
G92 X0 Y0                    (set this position as new origin)
G29                          (auto bed leveling)
G4 P10000                    (pause for 10s)
(Custom header end)

