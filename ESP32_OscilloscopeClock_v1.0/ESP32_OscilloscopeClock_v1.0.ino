/******************************************************************************
  
  ESP32 Oscilloscope Clock 
  using internal DACs with I2S and DMA, with WiFi and ntp sync.
  
  Mauro Pintus , Milano 2018/05/25
  DL4OU , 2021/07/15

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

  Includes Code from bitluni / Wave2020
  
  My project is based on this one:
  http://www.dutchtronix.com/ScopeClock.htm
  
  Thank you!!

******************************************************************************/

#include <driver/dac.h>
#include <soc/rtc.h>
#include <soc/sens_reg.h>
#include "driver/i2s.h"
#include "DataTable.h"


//#define EXCEL
//#define NTP


//--> Attribution 4.0 International (CC BY 4.0)
//bitluni 2020
static const i2s_port_t i2s_num = (i2s_port_t)I2S_NUM_0; // i2s port number

//static i2s_config_t i2s_config;
static const i2s_config_t i2s_config = {
     .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
     .sample_rate = 1000000,  //not really used
     .bits_per_sample = (i2s_bits_per_sample_t)I2S_BITS_PER_SAMPLE_16BIT, 
     .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
     .communication_format = I2S_COMM_FORMAT_I2S_MSB,
     .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
     .dma_buf_count = 2,
     .dma_buf_len = 1024  //big buffers to avoid noises
};

int bpos = 0;
short buff[1024];

//fills the i2s buffer with another coordinate and pushes the buffer once it's full
void pixel(int x, int y)
{
  buff[bpos++ ^ 1] = x << 8;
  buff[bpos++ ^ 1] = y << 8;
  if(bpos == 1024)
  {
    bpos = 0;
    i2s_write_bytes(i2s_num, (char*)buff, sizeof(buff), portMAX_DELAY);
  }
}
//end bitluni 2020 <--

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
int           Timeout        = 20;
const    long interval       = 990; //milliseconds, you should twick this
                                    //to get a better accuracy


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
  lastx=x;
  lasty=y;
  pixel (x, y);
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
  Serial.println("\nESP32 Oscilloscope Clock v1.0");
  Serial.println("Mauro Pintus 2018\nwww.mauroh.com");
  //rtc_clk_cpu_freq_set(RTC_CPU_FREQ_240M);
  Serial.println("CPU Clockspeed: ");
  Serial.println(rtc_clk_cpu_freq_value(rtc_clk_cpu_freq_get()));

  i2s_driver_install(i2s_num, &i2s_config, 0, NULL);    //start i2s driver
  i2s_set_pin(i2s_num, NULL);                           //use internal DAC
  //dummy frequency
  i2s_set_sample_rates(i2s_num, 1000000);               //dummy sample rate, since the function fails at high values

  //this is the hack that enables the highest sampling rates, You can even change BCK_DIV_NUM to 1 but the DACs lag behind with the voltage
  SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_A_V, 1, I2S_CLKM_DIV_A_S);
  SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_B_V, 1, I2S_CLKM_DIV_B_S);
  SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_NUM_V, 2, I2S_CLKM_DIV_NUM_S); 
  SET_PERI_REG_BITS(I2S_SAMPLE_RATE_CONF_REG(0), I2S_TX_BCK_DIV_NUM_V, 15, I2S_TX_BCK_DIV_NUM_S);

  if (h > 12) h=h-12;

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
      Timeout=20;
  
      while (!dateTime.valid) {
        dateTime = NTPch.getNTPtime(1.0, 1);
        Serial.print(".");
        delay(1000);
        Timeout--;
        if (Timeout==0){
          Serial.println("\nNTP Server Timeout");
          break;
        }
      }
      
      if (Timeout!=0){

        Serial.println("\nUsing NTP Time");
        NTPch.printDateTime(dateTime);
    
        byte actualHour      = dateTime.hour;
        byte actualMinute    = dateTime.minute;
        byte actualsecond    = dateTime.second;
        int  actualyear      = dateTime.year;
        byte actualMonth     = dateTime.month;
        byte actualday       = dateTime.day;
        byte actualdayofWeek = dateTime.dayofWeek;

        if (actualHour > 12) actualHour=actualHour-12;
        
        h=actualHour;
        m=actualMinute;
        s=actualsecond;
      }
      else{
        Serial.println("\nUsing Fix Time");
      }
    }  
  #endif    

  #if !defined NTP
    Serial.println("Using Fix Time");
  #endif

  if (h<10) Serial.print("0");
  Serial.print(h);
  Serial.print(":");
  if (m<10) Serial.print("0");
  Serial.print(m);
  Serial.print(":");
  if (s<10) Serial.print("0");
  Serial.println(s);
  h=(h*5)+m/12;
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
  //Serial.println("Out Ring");                         //2 to back trace
  //for (i=0; i < 1000; i++) PlotTable(DialData,sizeof(DialData),0x00,2,0);
 
  //Serial.println("Diagonals");                        //2 to back trace
  //for (i=0; i < 2000; i++) PlotTable(DialData,sizeof(DialData),0x00,0,0);

  PlotTable(DialData,sizeof(DialData),0x00,1,0);      //2 to back trace
  PlotTable(DialDigits12,sizeof(DialDigits12),0x00,1,0);//2 to back trace 
  PlotTable(HrPtrData, sizeof(HrPtrData), 0xFF,0,9*h);  // 9*h
  PlotTable(MinPtrData,sizeof(MinPtrData),0xFF,0,9*m);  // 9*m
  PlotTable(SecPtrData,sizeof(SecPtrData),0xFF,0,5*s);  // 5*s

  #if defined EXCEL
    while(1);
  #endif 

}

// End loop 
//*****************************************************************************
