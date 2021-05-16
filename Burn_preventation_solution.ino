#include "Arduino.h"
#include <Wire.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include <avr/pgmspace.h>

#define EMMISIVITY 0.95
#define TA_SHIFT 8 

paramsMLX90640 mlx90640;
const byte MLX90640_address1[] PROGMEM = {0x33}; //Default 7-bit unshifted address of the MLX90640
const byte MLX90640_address2[] PROGMEM = {0x32}; //Default 7-bit unshifted address of the MLX90640

static float tempValues1[32 * 24];
static float tempValues2[32 * 24];

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000); 
  Wire.beginTransmission((uint8_t)MLX90640_address1[0]);
  Wire.beginTransmission((uint8_t)MLX90640_address2[0]);
  if (  Wire.endTransmission() != 0) {
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

int changedelay(){
  int deltmp  = 2000;

  return deltmp;
}


void readTempValues() {
  for (byte x = 0 ; x < 2 ; x++) 
  {
    uint16_t mlx90640Frame1[834];
    uint16_t mlx90640Frame2[834];
    int status1 = MLX90640_GetFrameData(MLX90640_address1[0], mlx90640Frame1);
    int status2 = MLX90640_GetFrameData(MLX90640_address2[0], mlx90640Frame2);
    if (status1 < 0 || status2<0 )
    { 
      Serial.print("GetFrame Error: ");
      Serial.println(status1+status2);
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

  int A1 = 0;
  int B1 = 0;
  int A1save = 0;
  int B1save = 0;
  int temp = 35;
  
  Serial.println("\r\n===========================WaveShare MLX90640 Thermal Camera1===============================");

  int maxval=tempValues1[0];
  for (int i = 0; i < 768; i++) {
    
    if (((i % 32) == 0) && (i != 0)) {
      Serial.println(" ");
    }
    Serial.print((int)tempValues1[i]);
    Serial.print(" ");
    if (maxval<tempValues1[i]){

      maxval=tempValues1[i];
  }
  } 
  Serial.println("\r\n===========================WaveShare MLX90640 Thermal Camera1===============================");

  //Serial.println("Max Value is : ");
  //Serial.print(maxval);


  int A2 = 0;
  int B2 = 0;
  int A2save = 0;
  int B2save = 0;
  
  Serial.println("\r\n===========================WaveShare MLX90640 Thermal Camera2===============================");
  maxval=tempValues2[0];
  for (int i = 0; i < 768; i++) {
    
    if (((i % 32) == 0) && (i != 0)) {
      Serial.println(" ");
    }
    Serial.print((int)tempValues2[i]);
    Serial.print(" ");
    if (maxval<tempValues2[i]){

      maxval=tempValues2[i];
  }
  }
  Serial.println(" ");
  
  for(int i = 0 ; i < 768 ; i++){
    if(tempValues1[i] >= temp) {
      tempValues1[i] = 1;
    }else{
      tempValues1[i] = 0;
    }
  }

  for (int i = 0; i < 768; i++) {    
    
    if (((i % 32) == 0) && (i != 0)) {
      Serial.println(" ");    //줄넘김

    }
    Serial.print((int)tempValues1[i]);
    Serial.print(" ");
  }
  Serial.println(" ");
  Serial.println(" ");
  
  for(int i = 0 ; i < 768 ; i++){
    if(tempValues1[i] == 1) {
      if ( i % 32 >= 1  && tempValues1[i-1] == 0) { tempValues1[i-1] = 2; }
      if ( i % 32 <= 30  && tempValues1[i+1] == 0) { tempValues1[i+1] = 2; }

      if( i >= 32 && i <= 735 ) {
        if (tempValues1[i-32] == 0) { tempValues1[i-32] = 2; }
        if (tempValues1[i+32] == 0) { tempValues1[i+32] = 2; }
      }
    }
  }
  
  for (int i = 0; i < 768; i++) {    
    
    if (((i % 32) == 0) && (i != 0)) {
      Serial.println(" ");    //줄넘김

    }
    Serial.print((int)tempValues1[i]);
    Serial.print(" ");
  }
 
  Serial.println(' ');
  Serial.println(A1);
  Serial.println(B1);
  
   
  Serial.println("\r\n===========================WaveShare MLX90640 Thermal Camera2===============================");
}
 

void Device_Scan() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for (address = 1; address < 127; address++ )
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
