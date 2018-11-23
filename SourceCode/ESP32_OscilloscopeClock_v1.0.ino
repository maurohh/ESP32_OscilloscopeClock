/******************************************************************************
  
  ESP32 Oscilloscope Clock 
  using internal DACs, with WiFi and ntp sync.
  
  Mauro Pintus , Milano 2018/05/25
  
  https://github.com/maurohh/ESP32_OscilloscopeClock
  https://twitter.com/PintusMauro
  www.youtube.com/channel/UCZ93JYpVb9rEbg5cbcVG_WA/
  www.mauroh.com
  
  How to use it:
  Load this sketch on a ESP32 board
  Connect your oscilloscope channels to GPIO25 and GPIO26 of the ESP32
  Connect the ground of the oscilloscope to the GND of the ESP32 board
  Put your Oscilloscope in XY mode
  Adjust the vertical scale of the used channels to fit the clock

  This sketch is set display the time 10:08:37 everityme you power the board
  To change it, modify Start Hour/Minutes/Seconds

  To synchronize the clock with an NTP server, you have to uncomment 
  the line //#define NTP, removing the //, then you have to edit the 
  WiFi credential in place of Your SSID and Your PASS

  Uncommenting the line //#define EXCEL, removing the //, the sketch
  will run once and will output on the serial monitor all the coordinates
  it has generated. You can use this for example to draw the clock 
  using the graph function in Excel or LibreOffice
  This is useful to test anything you want to display on the oscilloscope
  to verify the actual points been generated.

  Enjoy Your new Oscilloscope Clock

  Credits:
  My project is based on this, thank you!!
  http://www.dutchtronix.com/ScopeClock.htm

******************************************************************************/

#include <driver/dac.h>
#include <soc/rtc.h>
#include <soc/sens_reg.h>
#include "DataTable.h"

//#define DEBUG
//#define EXCEL
//#define NTP

#if defined NTP
  #include <NTPtimeESP.h>
  #include <WiFi.h>
  
  NTPtime NTPch("europe.pool.ntp.org"); // Choose your server pool
  char *ssid      = "Your SSID";        // Set you WiFi SSID
  char *password  = "Your PASS";        // Set you WiFi password
  
  int status = WL_IDLE_STATUS;
  strDateTime dateTime;
#endif //

// Change this to set the initial Time
// Now is 10:08:37 (12h)
int h=10;   //Start Hour 
int m=8;    //Start Minutes
int s=37;   //Start Seconds

//Variables
int           lastx,lasty;
unsigned long currentMillis  = 0;
unsigned long previousMillis = 0;    
int           Timeout        = 10;
const    long interval       = 990; //milliseconds, you should twick this
                                    //to get a better accuracy


//*****************************************************************************
// PlotTable 
//*****************************************************************************

void PlotTable(byte *SubTable, int SubTableSize, int skip, int opt, int offset)
{

  #if defined DEBUG
    Serial.println(SubTableSize);
  #endif 
  int i=offset;
  while (i<SubTableSize){
    if (SubTable[i+2]==skip){
      i=i+3;
      if (opt==1) if (SubTable[i]==skip) i++;
    }
    Line(SubTable[i],SubTable[i+1],SubTable[i+2],SubTable[i+3]); 
    #if defined DEBUG
      Serial.print("Line ");
      Serial.print(i);
      Serial.print(" ");
      Serial.print(SubTable[i]);
      Serial.print(" ");
      Serial.print(SubTable[i+1]);
      Serial.print(" ");
      Serial.print(SubTable[i+2]);
      Serial.print(" ");
      Serial.println(SubTable[i+3]);
    #endif  
    if (opt==2){
      Line(SubTable[i+2],SubTable[i+3],SubTable[i],SubTable[i+1]);
      #if defined DEBUG
        Serial.print("Line ");
        Serial.print(i);
        Serial.print(" ");
        Serial.print(SubTable[i]);
        Serial.print(" ");
        Serial.print(SubTable[i+1]);
        Serial.print(" ");
        Serial.print(SubTable[i+2]);
        Serial.print(" ");
        Serial.println(SubTable[i+3]);
      #endif  
    }
    i=i+2;
    if (SubTable[i+2]==0xFF) break;
  }
  #if defined DEBUG
    Serial.print("End of segment ");
    Serial.println(i);
  #endif 
}

// End PlotTable 
//*****************************************************************************



//*****************************************************************************
// Dot 
//*****************************************************************************

inline void Dot(int x, int y)
{
    if (lastx!=x){
      lastx=x;
      dac_output_voltage(DAC_CHANNEL_1, x);
    }
    #if defined EXCEL
      Serial.print("0x");
      if (x<=0xF) Serial.print("0");
      Serial.print(x,HEX);
      Serial.print(",");
    #endif
    #if defined EXCEL
      Serial.print("0x");
      if (lasty<=0xF) Serial.print("0");
      Serial.print(lasty,HEX);
      Serial.println(",");
    #endif
    if (lasty!=y){
      lasty=y;
      dac_output_voltage(DAC_CHANNEL_2, y);
    }
    #if defined EXCEL
      Serial.print("0x");
      if (x<=0xF) Serial.print("0");
      Serial.print(x,HEX);
      Serial.print(",");
    #endif
    #if defined EXCEL
      Serial.print("0x");
      if (y<=0xF) Serial.print("0");
      Serial.print(y,HEX);
      Serial.println(",");
    #endif
}

// End Dot 
//*****************************************************************************



//*****************************************************************************
// Line 
//*****************************************************************************
// Bresenham's Algorithm implementation optimized
// also known as a DDA - digital differential analyzer

void Line(byte x1, byte y1, byte x2, byte y2)
{
    int acc;
    // for speed, there are 8 DDA's, one for each octant
    if (y1 < y2) { // quadrant 1 or 2
        byte dy = y2 - y1;
        if (x1 < x2) { // quadrant 1
            byte dx = x2 - x1;
            if (dx > dy) { // < 45
                acc = (dx >> 1);
                for (; x1 <= x2; x1++) {
                    Dot(x1, y1);
                    acc -= dy;
                    if (acc < 0) {
                        y1++;
                        acc += dx;
                    }
                }
            }
            else {   // > 45
                acc = dy >> 1;
                for (; y1 <= y2; y1++) {
                    Dot(x1, y1);
                    acc -= dx;
                    if (acc < 0) {
                        x1++;
                        acc += dy;
                    }
                }
            }
        }
        else {  // quadrant 2
            byte dx = x1 - x2;
            if (dx > dy) { // < 45
                acc = dx >> 1;
                for (; x1 >= x2; x1--) {
                    Dot(x1, y1);
                    acc -= dy;
                    if (acc < 0) {
                        y1++;
                        acc += dx;
                    }
                }
            }
            else {  // > 45
                acc = dy >> 1;
                for (; y1 <= y2; y1++) {
                    Dot(x1, y1);
                    acc -= dx;
                    if (acc < 0) {
                        x1--;
                        acc += dy;
                    }
                }
            }        
        }
    }
    else { // quadrant 3 or 4
        byte dy = y1 - y2;
        if (x1 < x2) { // quadrant 4
            byte dx = x2 - x1;
            if (dx > dy) {  // < 45
                acc = dx >> 1;
                for (; x1 <= x2; x1++) {
                    Dot(x1, y1);
                    acc -= dy;
                    if (acc < 0) {
                        y1--;
                        acc += dx;
                    }
                }
            
            }
            else {  // > 45
                acc = dy >> 1;
                for (; y1 >= y2; y1--) { 
                    Dot(x1, y1);
                    acc -= dx;
                    if (acc < 0) {
                        x1++;
                        acc += dy;
                    }
                }

            }
        }
        else {  // quadrant 3
            byte dx = x1 - x2;
            if (dx > dy) { // < 45
                acc = dx >> 1;
                for (; x1 >= x2; x1--) {
                    Dot(x1, y1);
                    acc -= dy;
                    if (acc < 0) {
                        y1--;
                        acc += dx;
                    }
                }

            }
            else {  // > 45
                acc = dy >> 1;
                for (; y1 >= y2; y1--) {
                    Dot(x1, y1);
                    acc -= dx;
                    if (acc < 0) {
                        x1--;
                        acc += dy;
                    }
                }
            }
        }
    
    }
}

// End Line 
//*****************************************************************************



//*****************************************************************************
// setup 
//*****************************************************************************

void setup() 
{
  Serial.begin(115200);
  rtc_clk_cpu_freq_set(RTC_CPU_FREQ_240M);
  Serial.println("CPU Clockspeed: ");
  Serial.println(rtc_clk_cpu_freq_value(rtc_clk_cpu_freq_get()));
  
  dac_output_enable(DAC_CHANNEL_1);
  dac_output_enable(DAC_CHANNEL_2);

  //int i;
  //for(i==0;i<20;i++) PlotTable(TestData,sizeof(TestData),0x00,0,00);//Full

  #if defined NTP
  Serial.println("Connecting to Wi-Fi");
  
  WiFi.begin (ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
    Timeout--;
    if (Timeout==0){
      Serial.println("\nWiFi Timeout");
      break;
    }
  }
  
  if (Timeout!=0){
    Serial.println("\nWiFi connected");
    Serial.println("NTP request sent to Server.");
    dateTime = NTPch.getNTPtime(1.0, 1);
    
    // check dateTime.valid before using the returned time
    // Use "setSendInterval" or "setRecvTimeout" if required

    Timeout=20;

    while (!dateTime.valid) {
      Serial.println(dateTime.valid);
      delay(1000);
      Timeout--;
      if (Timeout==0){
        Serial.println("\nNTP Server Timeout");
        break;
      }
    }
    
    if (Timeout!=0){
      
      NTPch.printDateTime(dateTime);
  
      byte actualHour      = dateTime.hour;
      byte actualMinute    = dateTime.minute;
      byte actualsecond    = dateTime.second;
      int  actualyear      = dateTime.year;
      byte actualMonth     = dateTime.month;
      byte actualday       = dateTime.day;
      byte actualdayofWeek = dateTime.dayofWeek;
      
  
      h=(actualHour*5)+actualMinute/12;
      m=actualMinute;
      s=actualsecond;
      //h=(h*5)+m/12;
      Serial.println(h);
      Serial.println(m);
      Serial.println(s);
      h=(h*5)+m/12;
    }
    else{
      Serial.println("Using Fix Time");
      Serial.println(h);
      Serial.println(m);
      Serial.println(s);
      h=(h*5)+m/12;
    }
  }  
  #endif 
  #if defined NTP
    else{
  #endif 
  #if !defined NTP
    {    
  #endif 
    Serial.println("Using Fix Time");
    Serial.println(h);
    Serial.println(m);
    Serial.println(s);
    h=(h*5)+m/12;
  }
}

// End setup 
//*****************************************************************************



//*****************************************************************************
// loop 
//*****************************************************************************

void loop() {

  currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    s++;
  }
  if (s==60) {
    s=0;
    m++;
    if ((m==12)||(m==24)||(m==36)||(m==48)) {
      h++;
    }
  }
  if (m==60) {
    m=0;
    h++;
  }
  if (h==60) {
    h=0;
  }

  //Optionals
  //PlotTable(DialDots,sizeof(DialDots),0x00,1,0);
  //PlotTable(TestData,sizeof(TestData),0x00,0,00); //Full
  //PlotTable(TestData,sizeof(TestData),0x00,0,11); //Without square

  int i;
  
  //Serial.println("Out Ring");                           //2 to back trace
  //for (i=0; i < 1000; i++) PlotTable(DialData,sizeof(DialData),0x00,2,0);
 
  //Serial.println("Diagonals");                          //2 to back trace
  //for (i=0; i < 2000; i++) PlotTable(DialData,sizeof(DialData),0x00,0,0);

  PlotTable(DialData,sizeof(DialData),0x00,2,0);//2 to back trace
  PlotTable(DialDigits12,sizeof(DialDigits12),0x00,1,0);//26 Test 2 retrace 
  
  PlotTable(HrPtrData,sizeof(HrPtrData),0xFF,0,9*h);   // 9*h
  PlotTable(MinPtrData,sizeof(MinPtrData),0xFF,0,9*m); // 9*m
  PlotTable(SecPtrData,sizeof(SecPtrData),0xFF,0,5*s); // 5*s

  #if defined EXCEL
    while(1);
  #endif 

}

// End loop 
//*****************************************************************************
