M107 ;fan off
M104 S215 ;nozzle target 215C
M140 S60 ;bed target 60C
M190 S60 ;wait for bed temp 60C
M109 S215 ;wait for nozzle temp 215C
G28 ;autohome
G29 ;meshbed leveling

G92 E0.0 ;set extruder position 0
G21 ; set units to millimeters
G90 ; use absolute coordinates
M83 ; use relative distances for extrusion

G1 Z4 F1000
G1 X0 Y-2 Z0.2 F3000.0
G1 E6 F2000
G1 X60 E9 F1000.0
G1 X100 E12.5 F1000.0
G1 Z2 E-6 F2100.00000

G1 X10 Y150 Z0.2 F3000
G1 E6 F2000

G1 F1000
G1 X170 Y150 E16
G1 X170 Y130 E2
G1 X10  Y130 E16
G1 X10  Y110 E2
G1 X170 Y110 E16
G1 X170 Y90  E2
G1 X10  Y90  E16
G1 X10  Y70  E2
G1 X170 Y70  E16
G1 X170 Y50  E2
G1 X10  Y50  E16
G1 X10  Y30  E2

G1 F1000
G1 X30  Y30.0  E1
G1 X30  Y29.5  E0.05
G1 X10  Y29.5  E1
G1 X10  Y29.0  E0.05
G1 X30  Y29.0  E1
G1 X30  Y28.5  E0.05
G1 X10  Y28.5  E1
G1 X10  Y28.0  E0.05
G1 X30  Y28.0  E1
G1 X30  Y27.5  E0.05
G1 X10  Y27.5  E1
G1 X10  Y27.0  E0.05
G1 X30  Y27.0  E1
G1 X30  Y26.5  E0.05
G1 X10  Y26.5  E1
G1 X10  Y26.0  E0.05
G1 X30  Y26.0  E1
G1 X30  Y25.5  E0.05
G1 X10  Y25.5  E1
G1 X10  Y25.0  E0.05
G1 X30  Y25.0  E1
G1 X30  Y24.5  E0.05
G1 X10  Y24.5  E1
G1 X10  Y24.0  E0.05
G1 X30  Y24.0  E1
G1 X30  Y23.5  E0.05

G1 Z2 E-6 F2100
G1 X180 Y0 Z10 F3000

G4

M107
M104 S0 ; turn off temperature
M140 S0 ; turn off heatbed
M84     ; disable motors
