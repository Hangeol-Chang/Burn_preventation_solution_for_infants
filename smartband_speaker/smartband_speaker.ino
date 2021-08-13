#include "DFRobotDFPlayerMini.h"
#include "wiring_private.h"

//=======================================wifi
#include <SPI.h> 
#include <WiFiNINA.h>
//=======================================wifi

// Create the DFPlayer Mini object.
DFRobotDFPlayerMini myDFPlayer;

int status_wifi = WL_IDLE_STATUS; 
WiFiServer server(80); 
WiFiClient client;

const char* ssid = "AndroidHotspot1867";       // 공백없이 정확히 넣어야 해요.
const char* password = "01051781867";          // 공백없이 정확히 넣어야 해요.

int musicnum;

void setup()
{
  Serial1.begin(9600);  
  while(!myDFPlayer.begin(Serial1)){ Serial.println("Not Connected!"); }  
  Serial.println("DFPlayer Connected!!!");  

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

  //=================================================speaker setting
  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
  //----Set volume----
  myDFPlayer.volume(30);  //Set volume value (0~30).
  // Set EQ
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  // Set the SD Card as default source.
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  //=================================================speaker setting
}


void loop()
{ 
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  
  //=================================================앱 버튼으로 노래 고르기 
  if (req.indexOf("music1")!=-1){
    musicnum=1;
  }
  if (req.indexOf("music2")!=-1){
    musicnum=2;
  }
  //=================================================앱 버튼으로 노래 고르기 
  

  //=================================================경고 신호 수신 시 안전 신호 발생할때까지 고른 노래를 0~1초까지 반복 재생  
  while (req.indexOf("/warning/")!=-1) {
    myDFPlayer.play(musicnum);
    delay(1000);           
  }
  //=================================================경고 신호 수신 시 안전 신호 발생할때까지 고른 노래를 0~1초까지 반복 재생  

   
  myDFPlayer.stop(); //이 외에는 노래 중지 
  

}
