# ESP32 Oscilloscope Clock by Mauro Pintus

ESP32 Oscilloscope Clock using internal DACs, with WiFi ntp sync

Mauro Pintus , Milano 2018/05/25

![](https://github.com/maurohh/ESP32_OscilloscopeClock/blob/master/ESP32_OscilloscopeClock_01.jpg)

  How to use it:
  Load this sketch on a ESP32 board using the Arduino IDE 1.8.7
  See Andreas Spiess video linked below if you dont know how to...
  Connect your oscilloscope channels to GPIO25 and GPIO26 of the ESP32
  Connect the ground of the oscilloscope to the GND of the ESP32 board
  Put your Oscilloscope in XY mode
  Adjust the vertical scale of the used channels to fit the clock

  Enjoy Your new Oscilloscope Clock!!! :)

  Additional notes:
  NTP Sync
  By default this sketch will start from a fix time 10:08:37 everityme 
  you reset the board.
  
  To change it, modify the variables h,m,s below.
  To synchronize the clock with an NTP server, you have to install 
  the library NTPtimeESP from Andreas Spiess.
  Then ncomment the line //#define NTP, removing the //.
  Edit the WiFi credential in place of Your SSID and Your PASS.
  Check in the serial monitor if it can reach the NTP server.
  You mignt need to chouse a different pool server for your country.
  
  The NTPtimeESP library was meant for the ESP8266 and you need to adit the file NTPtimeESP.h to use it with the ESP32.
  Open up the "NTPtimeESP.h" inside the library and replace the "#include <ESP8266WiFi.h>" with "#include <WiFi.h>".
  
  TimeZone Change - You needed to modify the "NTPch.getNTPtime(1.0, 1);" request to fit your TimeZone. 
  Simply change the first argument with your required GMT offset. So for Italy (GMT +1) I've need tit to be 1.0.
  For Pacific Coastal Zone (GMT -8), you need to change "NTPch.getNTPtime(1.0, 1);"  to "NTPch.getNTPtime(-8.0, 1);".
  
  EXCEL XY Coordinates Test
  If you want there is also a special mode that can be enabled uncommenting 
  the line //#define EXCEL, removing the //. In this mode, the sketch
  will run once and will output on the serial monitor all the coordinates
  it has generated. You can use this coordinates to draw the clock 
  using the graph function in Excel or LibreOffice
  This is useful to test anything you want to display on the oscilloscope
  to verify the actual points that will be generated.

  GitHub Repository
  https://github.com/maurohh/ESP32_OscilloscopeClock

  Twitter Page
  https://twitter.com/PintusMauro

  Youtube Channel
  www.youtube.com/channel/UCZ93JYpVb9rEbg5cbcVG_WA/

  Old Web Site
  www.mauroh.com

  Credits:
  Andreas Spiess
  https://www.youtube.com/watch?v=DgaKlh081tU

  Andreas Spiess NTP Library
  https://github.com/SensorsIot/NTPtimeESP
  
  My project is based on this one:
  http://www.dutchtronix.com/ScopeClock.htm
  
  Thank you!!

![](https://github.com/maurohh/ESP32_OscilloscopeClock/blob/master/ESP32_OscilloscopeClock_Excel.jpg)
