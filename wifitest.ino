//=======================================wifi
#include <SPI.h> 
#include <WiFiNINA.h>
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
      
      Serial.println("wifi connected");
      server.begin(); 
    }

    IPAddress ip = WiFi.localIP();
    while (!client) { client = server.available(); }
  
    // 클라이언트로부터 데이터 수신을 기다림
    Serial.println("new client"); 
    while(!client.available()){ delay(1); }
    Serial.println("open"); 
    //=================================================wifi
}

void loop(void) {
  switch (stat){
    case 0:         //위험판단 실행
      readTempValues();
      break;
      
    case 1:         //위험신호 전달(200)
      while true {
        client.flush();
        client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n</html>\n");
        delay(1);
        String rec = client.readStringUntil('\r');          //수신되었는지 확인
        Serial.println(rec);                                //수신되었는지 확인
        if(rec != 0) break;
      }

      Serial.print("위험위험");
      ing = true;
      stat = 0;
      break;
      
    case 2:         //안전신호 전달(0)
      while true {
        client.flush();
        client.print("HTTP/1.1 0 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n</html>\n");
        delay(1);
        String rec = client.readStringUntil('\r');          //수신되었는지 확인
        Serial.println(rec);                                //수신되었는지 확인
        if(rec != 0) break;
      }

      Serial.print("안전안전");
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
    if(ing) stat = 1;
    else    stat = 2;
  }
}
