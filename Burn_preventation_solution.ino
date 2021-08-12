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

//=======================================wifi
int stat = 0;
bool ing = false;

const char* ssid = "AndroidHotspot1867";       // 공백없이 정확히 넣어야 해요.
const char* password = "01051781867";   // 공백없이 정확히 넣어야 해요.

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
    Serial.println("pass");
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
      Serial.println("위험판단");
      readTempValues();
      break;
      
    case 1:         //위험신호(ON), 안전신호(OFF)
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
      
      delay(1);
      if (ing) ing = false;
      else     ing = true ;

      stat = 0;
      break;
  }
}

//===================================================위험판단알고리즘======================================================================
int save1[884];
float correction_factor = 0;                 //0 = 정사각영역, 1 = 직사각영역, 2 = 비사각영역
static float tempValues1[26 * 34];
float epsilon1_1;
float epsilon1_2;
int coor1[2];
int coor2[2];
int movespd[2];
bool isgraze;

int temp = 35;
int stdspd = 0;

void readTempValues() {
    uint16_t mlx90640Frame1[834];
    int status1 = MLX90640_GetFrameData(MLX90640_address1[0], mlx90640Frame1);
    if (status1 < 0) Serial.println("GetFrame Error: " && status1);

    float Ta1 = MLX90640_GetTa(mlx90640Frame1, &mlx90640);
    float tr1 = Ta1 - TA_SHIFT;

    MLX90640_CalculateTo(mlx90640Frame1, &mlx90640, EMMISIVITY, tr1, tempValues1, temp, correction_factor, coor2);
    Serial.println("pass collecting data");
    
    //==========================================================================스피드보정
    if(coor1[0] != 0 || coor1[1] != 0) { movespd[1] = sqrt( (coor2[0] - coor1[0])^2 + (coor2[1] - coor1[1])^2 ); }
    if ( abs( movespd[1] - movespd[0] ) > 0 ) isgraze = true ;
    else                                      isgraze = false;

    Serial.print( "가속도 : "); Serial.println( movespd[1] - movespd[0] ); Serial.println("");

    coor1[0] = coor2[0]; coor1[1] = coor2[1];             //다음을 위한 정리
    movespd[0] = movespd[1];
    //==========================================================================스피드보정
    
    int Atob = 0;
    int btoA = 0;

    for (int i = 0; i < 884; i++) {
      if(tempValues1[i] == 0) continue;
      else{
        if(tempValues1[i] - save1[i] ==  2) { Atob ++; }           //후퇴
        if(tempValues1[i] - save1[i] == -2) { btoA ++; }           //접근  
      }
    }
    epsilon1_2 = epsilon1_1;
    epsilon1_1 = btoA / (Atob + 1 + correction_factor);

    //============================================입실론 디버깅
    Serial.print("Epsilon1 is : "); Serial.println(epsilon1_1);
    Serial.print("Epsilon2 is : "); Serial.println(epsilon1_2);
    Serial.println("");

    Serial.println(correction_factor);
    //============================================입실론 디버깅
    
    switch (ing) {
      case false :
        if (epsilon1_1 >= 4) { Serial.println("Warning!"); stat = 1; }
        if (epsilon1_1 >= 1.5 && epsilon1_2 >= 1.5 && isgraze == false) {
                               Serial.println("Warning!"); stat = 1; }
        break;
      case true :
        if(epsilon1_1 < 1 && epsilon1_2 < 1) {
                               Serial.println("Dager disappear");
                               stat = 1; }
        break;       
    }
    for(int i = 0; i < 884; i++){ save1[i] = tempValues1[i]; }    //이전 루프 012배열을 save배열에 저장(메모리 때문에 일차원배열에 저장)
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
            if (address < 16)
                Serial.print("0");
            Serial.print(address, HEX);
            Serial.println("  !");
            nDevices++;
        }
        else if (error == 4)
        {
            Serial.print("Unknow error at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
        }
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found");
    else
        Serial.println("done");
}
