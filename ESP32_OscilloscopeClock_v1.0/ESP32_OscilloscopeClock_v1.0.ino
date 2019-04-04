/******************************************************************************
  
  ESP32 Oscilloscope Clock 
  using internal DACs, with WiFi and ntp sync.
  
  Mauro Pintus , Milano 2018/05/25

  How to use it:
  Load this sketch on a ESP32 board using the Arduino IDE 1.8.7
  See Andreas Spiess video linked below if you dont know how to...
  Connect your oscilloscope channels to GPIO25 and GPIO26 of the ESP32
  Connect the ground of the oscilloscope to the GND of the ESP32 board
  Put your Oscilloscope in XY mode
  Adjust the vertical scale of the used channels to fit the clock

  Enjoy Your new Oscilloscope Clock!!! :)

  Additional notes:
  By default this sketch will start from a fix time 10:08:37 everityme 
  you reset the board.
  To change it, modify the variables h,m,s below.

  To synchronize the clock with an NTP server, you have to install 
  the library NTPtimeESP from Andreas Spiess.
  Then ncomment the line //#define NTP, removing the //.
  Edit the WiFi credential in place of Your SSID and Your PASS.
  Check in the serial monitor if it can reach the NTP server.
  You mignt need to chouse a different pool server for your country.

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

******************************************************************************/

#include <driver/dac.h>
#include <soc/rtc.h>
#include <soc/sens_reg.h>
#include "DataTable.h"
#include <simpleDSTadjust.h>
#include <WiFi.h>


//#define EXCEL
//#define NTP
#define UTC_OFFSET -8
struct dstRule StartRule = {"PDT", Second, Sun, Mar, 2, 3600}; // Pacific Daylight time = UTC/GMT -7 hours
struct dstRule EndRule = {"PST", First, Sun, Nov, 1, 0};       // Pacific Standard time = UTC/GMT -8 hour
simpleDSTadjust dstAdjusted(StartRule, EndRule);

// change for different NTP (time servers)
#define NTP_SERVERS "us.pool.ntp.org", "time.nist.gov", "pool.ntp.org"

// August 1st, 2018
#define NTP_MIN_VALID_EPOCH 1533081600

//Variables
int           lastx,lasty;
unsigned long currentMillis  = 0;
unsigned long previousMillis = 0;    
int           Timeout        = 20*1000;
const    long interval       = 10*60*1000; //milliseconds, you should twick this
                                          //to get a better accuracy

char *ssid      = "SSID";        // Set you WiFi SSID
char *password  = "PASSWORD";        // Set you WiFi password

const int TRIGGER = 15;

//*****************************************************************************
// PlotTable 
//*****************************************************************************

void PlotTable(byte *SubTable, int SubTableSize, int skip, int opt, int offset)
{
  int i=offset;
  while (i<SubTableSize){
    if (SubTable[i+2]==skip){
      i=i+3;
      if (opt==1) if (SubTable[i]==skip) i++;
    }
    Line(SubTable[i],SubTable[i+1],SubTable[i+2],SubTable[i+3]);  
    if (opt==2){
      Line(SubTable[i+2],SubTable[i+3],SubTable[i],SubTable[i+1]); 
    }
    i=i+2;
    if (SubTable[i+2]==0xFF) break;
  }
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
    int n = 2;
    int acc;
    bool first_point = true;
    // for speed, there are 8 DDA's, one for each octant
    if (y1 < y2) { // quadrant 1 or 2
        byte dy = y2 - y1;
        if (x1 < x2) { // quadrant 1
            byte dx = x2 - x1;
            if (dx > dy) { // < 45
                acc = (dx >> 1);
                for (; x1 <= x2; x1++) {
                    Dot(x1, y1);
                    if (first_point)
                    {
                      delayMicroseconds(n);
                      first_point = false;
                      digitalWrite(TRIGGER, LOW);
                    }
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
                    if (first_point)
                    {
                      delayMicroseconds(n);
                      first_point = false;
                      digitalWrite(TRIGGER, LOW);
                    }
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
                    if (first_point)
                    {
                      delayMicroseconds(n);
                      first_point = false;
                      digitalWrite(TRIGGER, LOW);
                    }
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
                    if (first_point)
                    {
                      delayMicroseconds(n);
                      first_point = false;
                      digitalWrite(TRIGGER, LOW);
                    }
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
                    if (first_point)
                    {
                      delayMicroseconds(n);
                      first_point = false;
                      digitalWrite(TRIGGER, LOW);
                    }
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
                    if (first_point)
                    {
                      delayMicroseconds(n);
                      first_point = false;
                      digitalWrite(TRIGGER, LOW);
                    }
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
                    if (first_point)
                    {
                      delayMicroseconds(n);
                      first_point = false;
                      digitalWrite(TRIGGER, LOW);
                    }
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
                    if (first_point)
                    {
                      delayMicroseconds(n);
                      first_point = false;
                      digitalWrite(TRIGGER, LOW);
                    }
                    acc -= dx;
                    if (acc < 0) {
                        x1--;
                        acc += dy;
                    }
                }
            }
        }
    
    }

    
    digitalWrite(TRIGGER, HIGH);
}

// End Line 
//*****************************************************************************

// time_t getNtpTime()
void getNtpTime() {
  time_t now;
  
  int i = 0;
  configTime(UTC_OFFSET * 3600, 0, NTP_SERVERS);
  while((now = time(nullptr)) < NTP_MIN_VALID_EPOCH) {
    delay(500);
    i++;
    if (i > 60)
      break;
  }
}

//*****************************************************************************
// setup 
//*****************************************************************************

void setup() 
{
  Serial.begin(115200);
  Serial.println("\nESP32 Oscilloscope Clock v1.0");
  Serial.println("Mauro Pintus 2018\nwww.mauroh.com");
  //rtc_clk_cpu_freq_set(RTC_CPU_FREQ_240M);
  Serial.println("CPU Clockspeed: ");
  Serial.println(rtc_clk_cpu_freq_value(rtc_clk_cpu_freq_get()));

  Serial.println("Connecting to Wi-Fi");

  pinMode(TRIGGER, OUTPUT);
  digitalWrite(TRIGGER, HIGH);
    
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
  
  dac_output_enable(DAC_CHANNEL_1);
  dac_output_enable(DAC_CHANNEL_2);

  getNtpTime();

  char *dstAbbrev;
  time_t now = dstAdjusted.time(&dstAbbrev);
  struct tm * timeinfo = localtime (&now);
  
  Serial.println();
  Serial.printf("Current time: %d:%d:%d\n", timeinfo->tm_hour%12, timeinfo->tm_min,timeinfo->tm_sec);
  
//  // calculate for time calculation how much the dst class adds.
//  time_t dstOffset = UTC_OFFSET * 3600 + dstAdjusted.time(nullptr) - now;
//  Serial.printf("Time difference for DST: %d\n", dstOffset);

  previousMillis = currentMillis;
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
    getNtpTime();
  }

  char *dstAbbrev;
  time_t now = dstAdjusted.time(&dstAbbrev);
  struct tm * timeinfo = localtime (&now);

  int h = timeinfo->tm_hour%12;
  int m = timeinfo->tm_min;
  int s = timeinfo->tm_sec;
  
  //Optionals
  //PlotTable(DialDots,sizeof(DialDots),0x00,1,0);
  //PlotTable(TestData,sizeof(TestData),0x00,0,00); //Full
  //PlotTable(TestData,sizeof(TestData),0x00,0,11); //Without square

  int i;
  //Serial.println("Out Ring");                         //2 to back trace
  //for (i=0; i < 1000; i++) PlotTable(DialData,sizeof(DialData),0x00,2,0);
 
  //Serial.println("Diagonals");                        //2 to back trace
  //for (i=0; i < 2000; i++) PlotTable(DialData,sizeof(DialData),0x00,0,0);

  PlotTable(DialData,sizeof(DialData),0x00,1,0);      //2 to back trace
  PlotTable(DialDigits12,sizeof(DialDigits12),0x00,1,0);//2 to back trace 
  PlotTable(HrPtrData, sizeof(HrPtrData), 0xFF,0,(9*(5*h+m/12)));  // 9*h
  PlotTable(MinPtrData,sizeof(MinPtrData),0xFF,0,9*m);  // 9*m
  PlotTable(SecPtrData,sizeof(SecPtrData),0xFF,0,5*s);  // 5*s

  #if defined EXCEL
    while(1);
  #endif 

}

// End loop 
//*****************************************************************************
