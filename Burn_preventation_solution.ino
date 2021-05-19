#include "Arduino.h"
#include <Wire.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include <avr/pgmspace.h>

#define EMMISIVITY 0.95
#define TA_SHIFT 8 

paramsMLX90640 mlx90640;
const byte MLX90640_address1[] PROGMEM = { 0x33 }; //Default 7-bit unshifted address of the MLX90640
const byte MLX90640_address2[] PROGMEM = { 0x32 }; //Default 7-bit unshifted address of the MLX90640

static float tempValues1[32 * 24];
static float tempValues2[32 * 24];

int saveA1 = 0;
int saveb1 = 0;

int saveA2 = 0;
int saveb2 = 0;

int initval=0;
float tempValues1_old[768];
float tempValues2_old[768]; 

void setup() {
    Serial.begin(115200);
    Wire.begin();
    Wire.setClock(400000);
    Wire.beginTransmission((uint8_t)MLX90640_address1[0]);
    Wire.beginTransmission((uint8_t)MLX90640_address2[0]);
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
    status2 = MLX90640_DumpEE(MLX90640_address2[0], eeMLX90640);
    if (status1 != 0 || status2 != 0) Serial.println("Failed to load system parameters");
    status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
    if (status1 != 0 || status2 != 0) Serial.println("Parameter extraction failed");
    MLX90640_SetRefreshRate(MLX90640_address1[0], 0x05);
    MLX90640_SetRefreshRate(MLX90640_address2[0], 0x05);
    Wire.setClock(800000);
}

int del;
void loop(void) {
    readTempValues();
    del = changedelay();
    delay(del);
}

int changedelay() {
    int deltmp = 2000;

    return deltmp;
}


void readTempValues() {
    float midmat1[768];
    float midmat2[768];
    float Atob1=0; float Atob2=0; 
    float btoA1=0; float btoA2=0;
    for (byte x = 0; x < 2; x++)
    {
        uint16_t mlx90640Frame1[834];
        uint16_t mlx90640Frame2[834];
        int status1 = MLX90640_GetFrameData(MLX90640_address1[0], mlx90640Frame1);
        int status2 = MLX90640_GetFrameData(MLX90640_address2[0], mlx90640Frame2);
        if (status1 < 0 || status2 < 0)
        {
            Serial.print("GetFrame Error: ");
            Serial.println(status1 + status2);
        }

        float vdd1 = MLX90640_GetVdd(mlx90640Frame1, &mlx90640);
        float vdd2 = MLX90640_GetVdd(mlx90640Frame2, &mlx90640);
        float Ta1 = MLX90640_GetTa(mlx90640Frame1, &mlx90640);
        float Ta2 = MLX90640_GetTa(mlx90640Frame2, &mlx90640);
        float tr1 = Ta1 - TA_SHIFT;
        float tr2 = Ta2 - TA_SHIFT;

        MLX90640_CalculateTo(mlx90640Frame1, &mlx90640, EMMISIVITY, tr1, tempValues1);
        MLX90640_CalculateTo(mlx90640Frame2, &mlx90640, EMMISIVITY, tr2, tempValues2);
    }


    int temp = 35;

    Serial.println("\r\n===========================WaveShare MLX90640 Thermal Camera1===============================");

    int countA1 = 0;
    int countb1 = 0;



    int maxval = tempValues1[0];

    for (int i = 0; i < 768; i++) {

        if (((i % 32) == 0) && (i != 0)) {
            Serial.println(" ");
        }

        Serial.print((int)tempValues1[i]);
        Serial.print(" ");

        if (maxval < tempValues1[i]) {
            maxval = tempValues1[i];
        }

        if (tempValues1[i] >= temp) {
            tempValues1[i] = 1;
            countA1++;
        }
        else { tempValues1[i] = 0; }
    }

    Serial.println("Max Value is : " && maxval);
  
    for(int i = 0 ; i < 768 ; i++){
    if(tempValues1[i] == 1) {
      if ( i % 32 >= 1  && tempValues1[i-1] == 0) { tempValues1[i-1] = 5; } 
      if ( i % 32 == 0  && tempValues1[i+1] == 0) { tempValues1[i+1] = 5; } 
      
      if ( i % 32 <= 30  && tempValues1[i+1] == 0) { tempValues1[i+1] = 5;}
      if ( i % 32 == 31  && tempValues1[i-1] == 0) { tempValues1[i+1] = 5;}

      if( i >= 32 && i <= 735 ) {
        if (tempValues1[i-32] == 0) { tempValues1[i-32] = 5; }
        if (tempValues1[i+32] == 0) { tempValues1[i+32] = 5; }
      }
      if( i < 32) {if (tempValues1[i+32] == 0) { tempValues1[i+32] = 5; }}
      if( i > 735) {if (tempValues1[i-32] == 0) { tempValues1[i-32] = 5; }}
      }
    }
    



    Serial.println("\r\n===========================WaveShare MLX90640 Thermal Camera2===============================");
    
    int countA2 = 0;
    int countb2 = 0;

    maxval = tempValues2[0];

    for (int i = 0; i < 768; i++) {

        if (((i % 32) == 0) && (i != 0)) {
            Serial.println(" ");
        }

        Serial.print((int)tempValues2[i]);
        Serial.print(" ");

        if (maxval < tempValues2[i]) {
            maxval = tempValues2[i];
        }

        if (tempValues2[i] >= temp) {
            tempValues2[i] = 1;
            countA2++;
        }
        else { tempValues2[i] = 0; }
    }

    Serial.println("Max Value is : " && maxval);

    for(int i = 0 ; i < 768 ; i++){
    if(tempValues2[i] == 1) {
      if ( i % 32 >= 1  && tempValues2[i-1] == 0) { tempValues2[i-1] = 5; } 
      if ( i % 32 == 0  && tempValues2[i+1] == 0) { tempValues2[i+1] = 5; } 
      
      if ( i % 32 <= 30  && tempValues2[i+1] == 0) { tempValues2[i+1] = 5;}
      if ( i % 32 == 31  && tempValues2[i-1] == 0) { tempValues2[i+1] = 5;}

      if( i >= 32 && i <= 735 ) {
        if (tempValues2[i-32] == 0) { tempValues2[i-32] = 5; }
        if (tempValues2[i+32] == 0) { tempValues2[i+32] = 5; }
      }
      if( i < 32) {if (tempValues2[i+32] == 0) { tempValues2[i+32] = 5; }}
      if( i > 735) {if (tempValues2[i-32] == 0) { tempValues2[i-32] = 5; }}
      }
    }
    if (initval==0){
      for (int i=0; i<768; i++){

        tempValues1_old[i]=tempValues1[i];
        tempValues2_old[i]=tempValues2[i];
      }
      initval++;
    }
    else{
      for (int i = 0; i < 768; i++){
        midmat1[i]=tempValues1[i]-tempValues1_old[i];
        if (midmat1[i]==-4) {Atob1++;}
        if (midmat1[i]==4) {btoA1++;}
        midmat2[i]=tempValues2[i]-tempValues2_old[i];
        if (midmat2[i]==-4) {Atob2++;}
        if (midmat2[i]==4) {btoA2++;}
      } 
      
      float epsilon1=(btoA1)/(Atob1+1);
      float epsilon2=(btoA2)/(Atob2+1);
      
      if (epsilon1 >=2 || epsilon2 >=2) {Serial.print("alert!");}
      
      for (int i = 0; i < 768; i++){
        tempValues1_old[i]=tempValues1[i];
        tempValues2_old[i]=tempValues2[i];
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
