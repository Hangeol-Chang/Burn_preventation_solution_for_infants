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

static float tempValues1[26 * 34];

float epsilon1_1 = 0;
float epsilon1_2 = 0;

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

    //IPAddress ip = WiFi.localIP();
    while (!client) { client = server.available(); }
    Serial.println("new client"); 
    
    // 클라이언트로부터 데이터 수신을 기다림
    while(!client.available()){ delay(1); }
    //=================================================wifi
       
    Wire.begin();
    Wire.setClock(400000);
    Wire.beginTransmission((uint8_t)MLX90640_address1[0]);
    if (Wire.endTransmission() != 0) {
        Serial.println("MLX90640 not detected at default I2C address. Starting scan the device addr...");
        //    Device_Scan();
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
      readTempValues();
      break;
      
    case 1:         //위험신호 전달(200)
      for (int i = 0; i < 1000; i++) {
        client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n</html>\n");
      }
      client.stop();                                        //연결 끊고 재연결
      Serial.println("disconnect");
      
      while (!client) { client = server.available(); }
      while (!client.available()){ delay(1); }
      Serial.println("new client");
      
      delay(1);
      ing = true;
      stat = 0;
      break;
      
    case 2:         //안전신호 전달(0)
      for (int i = 0; i < 1000; i++) {
        client.print("HTTP/1.1 100 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n</html>\n");
      }
      client.stop();                                        //연결 끊고 재연결
      Serial.println("disconnect");
      
      while (!client) { client = server.available(); }
      while (!client.available()){ delay(1); }
      Serial.println("new client");
      
      delay(1);
      ing = false;
      stat = 0;
      break;
      
    default:
      break;
  }
}

//===================================================위험판단알고리즘======================================================================
int save1[884];

void readTempValues() {
    uint16_t mlx90640Frame1[834];
    int status1 = MLX90640_GetFrameData(MLX90640_address1[0], mlx90640Frame1);
    if (status1 < 0) Serial.println("GetFrame Error: " && status1);

    //float vdd1 = MLX90640_GetVdd(mlx90640Frame1, &mlx90640);        //뒤에서 안쓰이는듯? 일단 지워봄
    float Ta1 = MLX90640_GetTa(mlx90640Frame1, &mlx90640);
    float tr1 = Ta1 - TA_SHIFT;

    int temp = 35;
    MLX90640_CalculateTo(mlx90640Frame1, &mlx90640, EMMISIVITY, tr1, tempValues1, temp);

    for (int i = 1; i < 25; i++) {                                  //인접지역(b)변환, 3으로 변환하도록 함
      for(int j = 1; j < 33; j++) {
        if(tempValues1[ (i * 34) + j ] == 1){
          if(tempValues1[ (i * 34) + j - 1 ] == 0) tempValues1[ (i * 34) + j - 1 ] = 3;
          if(tempValues1[ (i * 34) + j + 2 ] == 0) tempValues1[ (i * 34) + j + 2 ] = 3;
          if(tempValues1[ ((i - 1) * 34) + j ] == 0) tempValues1[ ((i - 1) * 34) + j ] = 3;
          if(tempValues1[ ((i + 1) * 34) + j ] == 0) tempValues1[ ((i + 1) * 34) + j ] = 3;
        }
      }
    }
    //Serial.println("Array - Save");

    float Atob = 0;
    float btoA = 0;

    int tendency;
    for (int i = 0; i < 884; i++) {
      if(tempValues1[i] == 0) continue;
      else{
        if(tempValues1[i] - save1[i] == 2)  { Atob ++; }           //후퇴
        if(tempValues1[i] - save1[i] == -2) { btoA ++; }           //접근  
      }
    }
    epsilon1_2 = epsilon1_1;
    epsilon1_1 = btoA / (Atob + 1);

    //============================================입실론 디버깅
    Serial.print("Epsilon1 is : ");
    Serial.println(epsilon1_1);
    Serial.print("Epsilon2 is : ");
    Serial.println(epsilon1_2);
    Serial.println("");
    //============================================입실론 디버깅
    
    switch (ing) {
      case false :
        if (epsilon1_1 >= 1 && epsilon1_2 >= 1) {
          Serial.println("Warning!");
          stat = 1;
        }
        break;
        
      case true :
        if(epsilon1_1 < 1 && epsilon1_2 < 1) {
        Serial.println("Dager disappear");
        stat = 2;
        }
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
