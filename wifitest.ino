//=======================================wifi
#include <SPI.h> 
#include <WiFiNINA.h>
int stat = 0;
bool ing = false;

const char* ssid = "AndroidHotspot1867";       // 공백없이 정확히 넣어야 해요.
const char* password = "01051781867";   // 공백없이 정확히 넣어야 해요.
String rec;

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
      
      Serial.println("wifi connected");
      server.begin(); 
    }

    IPAddress ip = WiFi.localIP();
    while (!client) { client = server.available(); }
  
    // 클라이언트로부터 데이터 수신을 기다림
    Serial.println("new client"); 
    while(!client.available()){ delay(1); }

    rec = client.readString();
    Serial.println(rec);
    //=================================================wifi
}

int num = 1;
void loop(void) {
  switch (stat){
    case 0:         //위험판단 실행
      Serial.println("case 0");
      readTempValues();
      break;
      
    case 1:         //위험신호 전달(200)
      Serial.print("case 1");
      for (int i = 0; i < 1000; i ++) {
        client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n</html>\n");
      }
      client.stop();                                        //연결 끊고 재연결
      Serial.println("disconnect");
      
      while (!client) { 
        client = server.available();
        delay(1); 
        Serial.println(num);
        num++;
      }
      num = 1;
      while (!client.available()){ 
        delay(1); 
        Serial.println(num);
        num++;
      }
      
      Serial.println("위험");
      ing = true;
      stat = 0;
      break;
      
    case 2:         //안전신호 전달(0)
      Serial.print("case 2");
      for (int i = 0; i < 1000; i ++) {
        client.print("HTTP/1.1 0 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n</html>\n");
      }
      client.stop();                                        //연결 끊고 재연결
      Serial.println("disconnect");
      
      while (!client) { 
        client = server.available();
        delay(1); 
        Serial.println(num);
        num++;
      }
      num = 1;
      while (!client.available()){ 
        delay(1); 
        Serial.println(num);
        num++;
      }
      
      Serial.println("안전");
      ing = false;
      stat = 0;
      break;
      
    default:
      break;
  }
}

void readTempValues(){
  if(stat == 0){
    delay(2000);
    if(ing) stat = 2;
    else    stat = 1;
  }
}
