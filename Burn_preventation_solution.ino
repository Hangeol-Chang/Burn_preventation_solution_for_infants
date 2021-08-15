#include "Arduino.h"
#include <Wire.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include <avr/pgmspace.h>
//=======================================wifi
#include <SPI.h> 
#include <WiFiNINA.h>
//=======================================wifi
#define EMMISIVITY 0.95
#define TA_SHIFT 8 

paramsMLX90640 mlx90640;
const byte MLX90640_address1[] PROGMEM = { 0x32 }; //Default 7-bit unshifted address of the MLX90640

//=======================================wifi _ 잠그면안됨
int stat = 0;
bool ing = false;

const char* ssid = "AndroidHotspot1867";       // 공백없이 정확히 넣어야 해요.
const char* password = "01051781867";          // 공백없이 정확히 넣어야 해요.

int status_wifi = WL_IDLE_STATUS; 
WiFiServer server(80); 
WiFiClient client;
//=======================================wifi

void setup() {
    Serial.begin(115200);
    
    //=================================================wifi
    while (status_wifi != WL_CONNECTED) { 
      status_wifi = WiFi.begin(ssid, password); 
      delay(1000);       
      Serial.println("try to connect wifi");
      server.begin(); 
    }
    Serial.println("wifi connected");

    while (!client) { client = server.available(); }
    Serial.println("new client"); 
    while(!client.available()){ delay(1); }                   // 클라이언트로부터 데이터 수신을 기다림
    //=================================================wifi
    
    
    Wire.begin();
    Wire.setClock(400000);
    Wire.beginTransmission((uint8_t)MLX90640_address1[0]);
    if (Wire.endTransmission() != 0) {
        Serial.println("MLX90640 not detected at default I2C address. Starting scan the device addr...");
        //  Device_Scan();
    }
    else { Serial.println("MLX90640 online!"); }
    
    int status;
    int status1;
    uint16_t eeMLX90640[832];
    status1 = MLX90640_DumpEE(MLX90640_address1[0], eeMLX90640);
    status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
    if (status1 != 0) Serial.println("Failed to load system parameters");
    if (status != 0) Serial.println("Parameter extraction failed");
    MLX90640_SetRefreshRate(MLX90640_address1[0], 0x05);
    Wire.setClock(800000);
}

void loop(void) {
  switch (stat){
    case 0:         //위험판단 실행
      Serial.println("====================================loop start====================================");
      readTempValues();
      Serial.println("////////////////////////////////////=loop end=//////////////////////////////////// \n \n \n");
      delay(500);
      break;
      
    case 1:         //위험신호(ON), 안전신호(OFF)
    
      //================================================================wifi
      String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n"; 
      s += (ing)? "OFF": "ON";      // vall의 값이 참이면('0'아닌 값은 모두 참) 'ON'저장, 거짓이면'OFF'저장
      s += "</html>\n"; 
      for (int i = 0; i < 1000; i++) { client.print(s); }
      delay(1);
      client.stop();                                        //연결 끊기
      Serial.println("disconnect");
      
      while (!client) { client = server.available(); }      //재연결
      while (!client.available()){ delay(1); }
      Serial.println("new client");
      //================================================================wifi
      

      
      delay(1);
      if (ing) ing = false;
      else     ing = true ;

      stat = 0;
      break;
  }
}

//===================================================위험판단알고리즘======================================================================

static int tempValues1[26 * 34];             static int tempValues2[26 * 34];
float epsilon1_1;                            float epsilon1_2;                      // 입실론
int coor1[3];                                int coor2[3];                          // 중심좌표, 사이즈정보 저장
int movespd[2];           /*중심속도*/        bool isgraze;                          //가속도(지나치는지 판단)
int swi = 1;              /*스위치*/         float sizecomp;
float correction_factor;            // 0 = 비왜곡영역, 1 = 저왜곡영역, 2 = 고왜곡영역

int temp = 35;            //화상위험 기준온도
int stdspd = 1;           //지나쳐감 기준속도

void readTempValues() {
    uint16_t mlx90640Frame1[834];
    int status1 = MLX90640_GetFrameData(MLX90640_address1[0], mlx90640Frame1);
    if (status1 < 0) Serial.println("GetFrame Error: " && status1);

    float Ta1 = MLX90640_GetTa(mlx90640Frame1, &mlx90640);
    float tr1 = Ta1 - TA_SHIFT;

    if(swi == 1){
      correction_factor = MLX90640_CalculateTo(mlx90640Frame1, &mlx90640, EMMISIVITY, tr1, tempValues1, temp, coor1);
      swi = -1;
      /*
      Serial.println("\r\n===========================WaveShare MLX90640 Thermal Camera===============================");
      for (int i = 0; i < 884; i++) { if (((i % 34) == 0) && (i != 0)) { Serial.println(" "); }
        Serial.print((int)tempValues1[i]); Serial.print(" "); }
      Serial.println("\r\n===========================WaveShare MLX90640 Thermal Camera===============================");
      */
    }
    else {
      correction_factor = MLX90640_CalculateTo(mlx90640Frame1, &mlx90640, EMMISIVITY, tr1, tempValues2, temp, coor2);
      swi = 1;
      /*
      Serial.println("\r\n===========================WaveShare MLX90640 Thermal Camera===============================");
      for (int i = 0; i < 884; i++) { if (((i % 34) == 0) && (i != 0)) { Serial.println(" "); }
        Serial.print((int)tempValues2[i]); Serial.print(" "); }
      Serial.println("\r\n===========================WaveShare MLX90640 Thermal Camera===============================");
      */
    }
    //==========================================================================스피드보정==============================================================
    if(coor1[0] != 0 && coor2[0] != 0) { 
      movespd[1] = sqrt( pow(coor2[0] - coor1[0],2) + pow(coor2[1] - coor1[1],2)); 
      if ( abs( movespd[1] - movespd[0] ) > stdspd ) isgraze = true ;
      else                                           isgraze = false;
    }else { 
      movespd[1] = 0;
      isgraze = false;
    }
    
    Serial.println("------------------------------------스피드팩터 디버깅------------------------------------");
    Serial.print("   coor1 : "); Serial.print(coor1[0]);   Serial.print(", ");  Serial.println(coor1[1]);
    Serial.print("   coor2 : "); Serial.print(coor2[0]);   Serial.print(", ");  Serial.println(coor2[1]);
    Serial.print(" movespd : "); Serial.print(movespd[0]); Serial.print(" / "); Serial.println(movespd[1]);
    
    movespd[0] = movespd[1];
    //==========================================================================스피드보정==============================================================
    int Atob = 0;
    int btoA = 0;

    for (int i = 0; i < 884; i++) {
      if(tempValues1[i] == 0) continue;
      else{
        if(swi * (tempValues2[i] - tempValues1[i]) ==  2) { Atob ++; }           //후퇴
        if(swi * (tempValues2[i] - tempValues1[i]) == -2) { btoA ++; }           //접근  
      }
    }
    epsilon1_2 = epsilon1_1;
    epsilon1_1 = btoA / (Atob + 1 + correction_factor);

    if(swi == 1) { if(coor1[2] == 0) { sizecomp = 0; } else { sizecomp = (float)coor2[2] / (float)coor1[2]; }}
    else         { if(coor2[2] == 0) { sizecomp = 0; } else { sizecomp = (float)coor1[2] / (float)coor2[2]; }}
    
    Serial.println("====================================Debugging page====================================");
    Serial.print("Epsilon component : Atob : "); Serial.print(Atob); Serial.print(" / btoA : "); Serial.println(btoA); 
    Serial.print("       Correction factor : "); Serial.println(correction_factor);
    Serial.print("                Epsilon1 : "); Serial.println(epsilon1_1);
    Serial.print("                Epsilon2 : "); Serial.println(epsilon1_2);
    Serial.print("                sizecomp : "); Serial.println(sizecomp);
    Serial.println("");
    
    switch (ing) {
      case false :
        if      (epsilon1_1 >= 3.5) { Serial.println("Warning!");        stat = 1; }
        else if (epsilon1_1 >= 1.5 && epsilon1_2 >= 1.5 && isgraze == false) {
                                      Serial.println("Warning!");        stat = 1; }
        else if (10 >= sizecomp >= 2)   { Serial.println("Warning!");        stat = 1; }
        break;    
      case true :
        if(epsilon1_1 < 1 && epsilon1_2 < 1) {
                                      Serial.println("Dager disappear"); stat = 1; }
        break;       
    }
}

void Device_Scan() {
    byte error, address;
    int nDevices;
    Serial.println("Scanning...");
    nDevices = 0; 
    for (address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0)
        {
            Serial.print("I2C device found at address 0x");
            if (address < 16) { Serial.print("0"); }
            Serial.print(address, HEX);
            Serial.println("  !");
            nDevices++;
        }
        else if (error == 4)
        {
            Serial.print("Unknow error at address 0x");
            if (address < 16) { Serial.print("0"); }
            Serial.println(address, HEX);
        }
    }
    if (nDevices == 0) { Serial.println("No I2C devices found"); }
    else { Serial.println("done"); }
}
