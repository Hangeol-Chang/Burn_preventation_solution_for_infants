#include "DFRobotDFPlayerMini.h"
#include "wiring_private.h"
#include <SPI.h> 
#include <WiFiNINA.h>

// Create the DFPlayer Mini object.
DFRobotDFPlayerMini myDFPlayer;

int status_wifi = WL_IDLE_STATUS; 
WiFiServer server(80); 
WiFiClient client;

const char* ssid = "AndroidHotspot1867";       // 공백없이 정확히 넣어야 해요.
const char* password = "01051781867";          // 공백없이 정확히 넣어야 해요.

int musicnum;
bool playing;

void setup()
{
  Serial1.begin(9600);  
  while(!myDFPlayer.begin(Serial1)){ Serial.println("Not Connected!"); }  
  Serial.println("DFPlayer Connected!!!");  

  //=================================================speaker setting
  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
  //----Set volume----
  myDFPlayer.volume(30);  //Set volume value (0~30).
  // Set EQ
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  // Set the SD Card as default source.
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  //=================================================speaker setting

  //=================================================wifi
  while (status_wifi != WL_CONNECTED) { 
      status_wifi = WiFi.begin(ssid, password); 
      delay(1000);       
      Serial.println("try to connect wifi");
      server.begin(); 
    }
    Serial.println("wifi connected");
    //=================================================wifi
}

void loop()
{ 
  while (!client) { client = server.available(); } //데이터 수신 대기
  Serial.println("new client"); 
  while(!client.available()){ delay(1); }  

  String req = client.readStringUntil('\r');
  Serial.println(req);
  

  switch (playing){
    case false:
      if (req.indexOf("/warning/")!= -1)   { myDFPlayer.play(musicnum); playing = true; break; }    //노래 재생
      else if (req.indexOf("/music1/")!= -1){ musicnum=1; }       // 노래 고르기
      else if (req.indexOf("/music2/")!= -1){ musicnum=2; }
    
      break;
    case true:
      if (req.indexOf("/disappear/")!= -1) { myDFPlayer.stop(); playing = false;} // 노래 멈춤
      break;
  }
  delay(1000);                  //노래를 끄려면 적어도 1초 뒤에 가능
  client.stop();                //연결 끊기
}
