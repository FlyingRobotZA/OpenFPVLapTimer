# OpenFPVLapTimer
An open FPV Lap Timing system using IR emitters and receivers. On hardware such NodeMCU, Wemos and RasberryPi

# TX Emitter
Normal powerup will just emit ID number and pushing the button does nothing. This is to prevent accidental channel changes during racing.

Hold button in during powerup to enter config mode.

LED will flash once (0.5s) and then go off.

After releasing button, LED will flash current ID.

ID is indicated by long flashes (1s) for 10's digit followed by short flashes (0.3s) for 1's digit. So: long long short short short short = 24

Short button press (0.05s - 2s) will go up 1 ID

Long button press (hold for 2s - 5s, LED turns on after 2s) will go up 10 ID's

Very long press (5s+, LED turns on at 2s and off again at 5s) will reset ID back to 1.

After a button press LED will flash ID
You can push button while LED is flashing and it will change ID and restart ID flash indication.

Cycle power to go back to normal mode and save your emitter ID.

Wrapping is a little weird to keep 1's digit consistent.</br>
63+1 = 1 </br>
54+10 = 4 </br>
59+10 = 9 </br>
60+10 = 10 </br>
61+10 = 1 </br>
63+10 = 3 </br>
