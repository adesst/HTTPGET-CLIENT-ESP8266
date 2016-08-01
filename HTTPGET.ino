/**
 * @example HTTPGET.ino
 * @brief The HTTPGET demo of library WeeESP8266. 
 * @author Wu Pengfei<pengfei.wu@itead.cc> 
 * @date 2015.03
 * 
 * @par Copyright:
 * Copyright (c) 2015 ITEAD Intelligent Systems Co., Ltd. \n\n
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version. \n\n
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <ArduinoJson.h>
#include <ESP8266.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN             9           // Configurable, see typical pin layout above
#define SS_PIN              10          // Configurable, see typical pin layout above
#define BUZZER_PIN          8
#define STDBY_TOGGLE_PIN    7
#define SUCCESS             0
#define STD_BY              2
#define FAIL_REPETITION     4

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
void ringBuzzer(uint8_t);
void (*resetFunc)(void) = 0;
String cleanStartBrace(String&);
String cleanEndBrace(String&);
void dump_byte_array(byte* , byte);
void ringBuzzer(uint8_t);
void flushHWSerial(void);
void refreshCardID();

#define SSID        "TP-LINK_BOSSD"
#define PASSWORD    "newwirel3ss"
#define CUSTOM_IP   "192.168.1.171"
#define HOST_NAME   "192.168.1.201"
#define DEVICE_ID   1

volatile bool readCard = false;
volatile bool hasStdby = false;

char cardID[10];
char    sendHTTP[128];
bool    bREINIT = true;

uint8_t  RECV_STRING_EXTRA_CHAR = 1;
uint32_t HOST_PORT = 80;
char*   cvtStrJSON;
String clean_res = "";

SoftwareSerial Serial1 = SoftwareSerial(A0,A1);
ESP8266 wifi(Serial1);

void system_halt(void){
    while(1){
        ;
    }
}

void setup(void)
{
    pinMode(STDBY_TOGGLE_PIN, INPUT);
    if(digitalRead(STDBY_TOGGLE_PIN) == HIGH){
        hasStdby = true;
    }

    Serial.begin(19200);
    while(!Serial){
      ;
    }
    Serial.print("setup begin\r\n");

    SPI.begin();                // Init SPI bus
    mfrc522.PCD_Init();         // Init MFRC522 card

    pinMode(BUZZER_PIN, OUTPUT);
    ringBuzzer(0);

    if( bREINIT ){
        
        if (wifi.setOprToStation()) {
            Serial.print("to station + softap ok\r\n");
        } else {
            Serial.print("to station + softap err\r\n");
        }

        while(!wifi.joinAP(SSID, PASSWORD)){
          Serial.print("Join AP failure\r\n");
        }
        
        Serial.print("Join AP success\r\n");
        Serial.print("IP:");
        Serial.println( wifi.getLocalIP().c_str());   

        Serial.print("Set Local IP to ");
        Serial.println(CUSTOM_IP);

        Serial.print("New IP:");
        Serial.println( wifi.getLocalIP().c_str());
    }
    
    if( bREINIT ){
    
        if (wifi.disableMUX()) {
            Serial.println("single ok");
        } else {
            Serial.println("single err");

            while(!wifi.disableMUX()){
              Serial.println("Retry singleMUX");
            }
        }
    }
    
    Serial.println("setup end");
    flushHWSerial(); 

    //char *hello = "GET /sjson.php HTTP/1.1\r\nHost: 192.168.1.201\r\nConnection: close\r\n\r\n";

    //if (wifi.createTCP(HOST_NAME, HOST_PORT)) {
    //    Serial.println("create tcp ok");
    //} else {
    //    Serial.println("create tcp err");

    //    while(!wifi.createTCP(HOST_NAME, HOST_PORT)){
    //      Serial.println("Retry to create tcp");
    //    }
    //}
    //
    //wifi.send((const uint8_t*)hello, strlen(hello));

    //clean_res = Serial1.readString();
    //
    //Serial.print( "Clean Res: " ); 
    //Serial.println( clean_res ); 
    //if( clean_res.length() == 0 ){
    //    Serial.print( "Clean Res: HTTP Error " ); 
    //    system_halt();
    //}

    //clean_res = cleanStartBrace(clean_res);
    //clean_res = cleanEndBrace(clean_res);

    //Serial.print( "Clean Res: " ); 
    //Serial.println( clean_res ); 

    //StaticJsonBuffer<96> jsonBuffer;
    //JsonObject &jsonObject = jsonBuffer.parseObject(clean_res);

    //if( !jsonObject.success()){
    //    Serial.println( "Success Load" ); 
    //}

    //if( jsonObject.containsKey("cr")){
    //    Serial.println( "Has balance key" ); 
    //}
    //else{
    //    Serial.println( "No balance key" ); 
    //}

    //Serial.print( "jsonCr : "); 
    //Serial.println( (long) jsonObject["cr"], DEC ); 

    //Serial.print( "jsonE : "); 
    //Serial.println( (long) jsonObject["e"], DEC );

    //Serial.print( "jsonM : "); 
    //Serial.println( (const char*) jsonObject["m"] );

    //Serial.println( "-End-" ); 

    //wifi.releaseTCP();
    //flushHWSerial();
}
 
void loop(void)
{
    if( hasStdby ){
        ringBuzzer(STD_BY);
    }
    
    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent())
        return;

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial())
        return;

    clean_res = "";
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println("");  

    clean_res.toCharArray(cardID, 10);
    Serial.println((char*)cardID);  
    
    ringBuzzer(SUCCESS);
    Serial1.println("AT+CIPSTART=\"TCP\",\"192.168.1.201\",80");
    Serial.println(Serial1.readString());

    sprintf(sendHTTP,"GET /hellocard.php?cardID=%s&id=%i HTTP/1.1\r\nHost: 192.168.1.201\r\nConnection: close\r\n\r\n", (char*)cardID, DEVICE_ID);

    wifi.send((const uint8_t*)sendHTTP, strlen(sendHTTP));
    clean_res = Serial1.readString();
    Serial.println(clean_res);

    //Serial.println(sendHTTP);  
    //if (wifi.createTCP(HOST_NAME, HOST_PORT)) {
    //    Serial.println("create tcp ok");
    //} else {
    //    Serial.println("create tcp err");

    //    while(!wifi.createTCP(HOST_NAME, HOST_PORT)){
    //      Serial.println("Retry to create tcp");
    //    }
    //}
    //wifi.send((const uint8_t*)sendHTTP.c_str(), (uint32_t)sendHTTP.length());

    //clean_res = Serial1.readString();
    //
    //Serial.print( "Clean Res: " ); 
    //Serial.println( clean_res ); 
    //if( clean_res.length() == 0 ){
    //    Serial.print( "Clean Res: HTTP Error " ); 
    //    system_halt();
    //}

    //clean_res = cleanStartBrace(clean_res);
    //clean_res = cleanEndBrace(clean_res);

    //Serial.print( "Clean Res: " ); 
    //Serial.println( clean_res ); 

    //StaticJsonBuffer<96> jsonBuffer;
    //JsonObject &jsonObject = jsonBuffer.parseObject(clean_res);

    //if( !jsonObject.success()){
    //    Serial.println( "Success Load" ); 
    //}

    Serial1.println( "AT+CIPCLOSE" ); 
    delay(500);
    Serial.println( Serial1.readString() ); 
    Serial.println( "-End-" ); 

    wifi.releaseTCP();
    flushHWSerial();
    refreshCardID();

    delay(3000);
}

String fixStartBrace(String &str){
    uint8_t max = str.length();
    int idx = 0;
    bool bflag = false;
    String strBuffer = "";

    while(idx != max){
        if( str.charAt(idx) == '\r' && bflag == false){
            bflag = true;
        }

        if( bflag == true){
            strBuffer += str.charAt(idx);
        }
        idx +=1;
    }

    return strBuffer;
}
String cleanStartBrace(String &str){
    uint32_t max = str.length();
    uint32_t idx = 0;
    bool bflag = false;
    String strBuffer = "";

    while(idx != max){

        if( (char)str.charAt(idx) == '{' && bflag == false){
            bflag = true;
        }

        if( bflag == true){
            strBuffer += (char)str.charAt(idx);
        }
        idx +=1;
    }

    return strBuffer;
}
     
String cleanEndBrace(String &str){
    uint32_t idx = str.length();

    if( idx == 0 ){
        return "";
    }

    bool bflag = false;
    String strBuffer = "";

    while(idx != 0){
        idx -= 1;
        if( (char)str.charAt(idx) == '}' && bflag == false){
            break;
        }
    }

    for(uint32_t i=0; i <= idx; i++){
        strBuffer += (char)str.charAt(i);
    }

    return strBuffer;
}

/*
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        clean_res += String(buffer[i], HEX);
    }
    Serial.print(clean_res);
    Serial.print('\0');
}

void ringBuzzer(uint8_t mode = 1){
   
  if(mode == SUCCESS){
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
  }
  else if(mode == STD_BY){

    for(uint8_t iLoop = 0; iLoop < 2; iLoop++){

      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
    }

    delay(3000);
  }
  else{
    for(uint8_t iLoop = 0; iLoop < FAIL_REPETITION; iLoop++){

      digitalWrite(BUZZER_PIN, HIGH);
      delay(25);
      digitalWrite(BUZZER_PIN, LOW);
      delay(25);
    }
  }
}

void flushHWSerial(void){
    while(Serial.available()){
      ; // wait until all buffer are flush
    }
}

void refreshCardID(){
    for(uint8_t i=0; i < 10; i++){
        cardID[i] = '\0';
    }
    for(uint8_t i=0; i < 128; i++){
        sendHTTP[i] = '\0';
    }
}
