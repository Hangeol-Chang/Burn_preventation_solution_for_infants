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
const byte MLX90640_address1[] PROGMEM = { 0x33 }; //Default 7-bit unshifted address of the MLX90640

static float tempValues1[24 * 32];

float epsilon1_1 = 0;
float epsilon1_2 = 0;

//=======================================wifi
int stat = 0;
bool ing = false;

const char* ssid = "AndroidHotspot1867";       // 공백없이 정확히 넣어야 해요.
const char* password = "01051781867";   // 공백없이 정확히 넣어야 해요.

int status = WL_IDLE_STATUS; 
WiFiServer server(80); 
WiFiClient client;
//=======================================wifi

void setup() {
    Serial.begin(115200);
    //=================================================wifi
    while (status != WL_CONNECTED) { 
      status = WiFi.begin(ssid, password); 
      delay(1000); 
      
      Serial.println("wifi connected");
      server.begin(); 
    }

    IPAddress ip = WiFi.localIP();
    while (!client) { 
      client = server.available(); 
    }
  
    // 클라이언트로부터 데이터 수신을 기다림
    Serial.println("new client"); 
    //while(!client.available()){ delay(1); }
    //=================================================wifi
       
    Wire.begin();
    Wire.setClock(400000);
    Wire.beginTransmission((uint8_t)MLX90640_address1[0]);
    if (Wire.endTransmission() != 0) {
        Serial.println("MLX90640 not detected at default I2C address. Starting scan the device addr...");
        //    Device_Scan();
        //    while(1);
    }
    else {
        Serial.println("MLX90640 online!");
    }
    int status;
    int status1;
    int status2;
    uint16_t eeMLX90640[832];
    status1 = MLX90640_DumpEE(MLX90640_address1[0], eeMLX90640);
    if (status1 != 0) Serial.println("Failed to load system parameters");
    status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
    if (status1 != 0) Serial.println("Parameter extraction failed");
    MLX90640_SetRefreshRate(MLX90640_address1[0], 0x05);
    Wire.setClock(800000);
}

void loop(void) {
  switch (stat){
    case 0:         //위험판단 실행
      readTempValues();
      break;
      
    case 1:         //위험신호 전달(200)
      client.flush();
      client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  // the connection will be closed after completion of the response
            "Refresh: 20\r\n"        // refresh the page automatically every 20 sec
            "\r\n");
      Serial.println("되냐");
      delay(2); 

      ing = true;
      stat = 0;
      break;
      
    case 2:         //안전신호 전달(0)
      client.flush();
      client.print(
            "HTTP/1.1 0 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  // the connection will be closed after completion of the response
            "Refresh: 20\r\n"        // refresh the page automatically every 20 sec
            "\r\n");
      Serial.println("되냐");
      delay(2); 

      ing = false;
      stat = 0;
      
      break;
      
    default:
      break;
  }

}
//int8_t

void readTempValues() {
    int save1[24][32];

    for(int i = 0; i < 24; i++){    //이전 루프 012배열을 save배열에 저장(메모리 때문에 일차원배열에 저장)
      for(int j = 0; j < 32; j++){
       save1[i][j] = tempValues1[i * 24 + j];
      }
    }
  
    for (byte x = 0; x < 2; x++)
    {
        uint16_t mlx90640Frame1[834];
        int status1 = MLX90640_GetFrameData(MLX90640_address1[0], mlx90640Frame1);
        if (status1 < 0)
        {
            Serial.print("GetFrame Error: ");
            Serial.println(status1);
        }

        float vdd1 = MLX90640_GetVdd(mlx90640Frame1, &mlx90640);
        float Ta1 = MLX90640_GetTa(mlx90640Frame1, &mlx90640);
        float tr1 = Ta1 - TA_SHIFT;

        MLX90640_CalculateTo(mlx90640Frame1, &mlx90640, EMMISIVITY, tr1, tempValues1);
    }

    int temp = 35;
    int Atob = 0;
    int btoA = 0;

    int cal[24][32];

    epsilon1_2 = epsilon1_1;

    //Serial.println("\r\n===========================WaveShare MLX90640 Thermal Camera1===============================");
    //Serial.println("Temperature array");

    for (int i = 0; i < 768; i++) {
        //==============================================온도배열 출력
        //if (((i % 32) == 0) && (i != 0)) { Serial.println(" "); }
        //Serial.print( (int)tempValues1[i], " " );
        //==============================================온도배열 출력
        
        if (tempValues1[i] >= temp) { tempValues1[i] = 0; }
        else                        { tempValues1[i] = 1; }
    }
    //Serial.println(" ");
    
    int arr[26][34];

    for (int i = 0; i < 26; i++) {       //0과 1 배열을 테두리 프레임 추가된 2차원 배열로 변환
        for (int j = 0; j < 34; j++) {
            if (i * j == 0 || i == 25 || j == 33) {
                arr[i][j] = 1;
            }
            else {
                arr[i][j] = tempValues1[32 * (i - 1) + (j - 1)];
            }
        }
    }
    //Serial.println("0,1,2 Array");
    
    for (int i = 1; i < 25; i++) {       //0과 인접한 1을 2로 변환
        for (int j = 1; j < 33; j++) {
            if (arr[i][j] == 1 && arr[i - 1][j] * arr[i + 1][j] * arr[i][j - 1] * arr[i][j + 1] == 0) {
                arr[i][j] = 2;
            }
            //Serial.print(arr[i][j]);
            //Serial.print(" ");
        }
        //Serial.println(" ");
    }
    //Serial.println(" ");
    //Serial.println("Array - Save");

    for (int i = 0; i < 24; i++) {    //배열 위치대로 계산(나중배열 - 이전배열)
      for (int j = 0; j < 32; j++) {
        cal[i][j] = arr[i+1][j+1] - save1[i][j];
          if(cal[i][j]>=0){
            //Serial.print(" ");
            //Serial.print(cal[i][j]);
            //Serial.print(" ");
          }
          else{
            //Serial.print(cal[i][j]);
            //Serial.print(" ");
          }          
          if (cal[i][j] == -2) {   //b->A와 A->b 개수 카운트
            btoA++;
          }
          else if (cal[i][j] == 2) {
            Atob++;
          }
      }
    //Serial.println(" ");
    }
    
    epsilon1_1 = btoA / (Atob + 1)*1.0;

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
        
    for (int i = 0; i < 24; i++) {     //현재 0과1 배열을 일차원 배열에 저장(메모리 때문에)
      for (int j = 0; j < 32; j++) {
          tempValues1[i * 24 + j] = arr[i + 1][j + 1];
      }
    }
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
