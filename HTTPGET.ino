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

#define RST_PIN             9           // Configurable, see typical pin layout above
#define SS_PIN              10          // Configurable, see typical pin layout above
#define BUZZER_PIN          8
#define STDBY_TOGGLE_PIN    7
#define READY_TOGGLE_PIN    6
#define SUCCESS             0
#define STD_BY              2
#define FAIL_REPETITION     4

void (*resetFunc)(void) = 0;
String cleanStartBrace(String&);
String cleanEndBrace(String&);
void flushHWSerial(void);
void refreshCardID();

#define SSID        "TP-LINK_BOSSD"
#define PASSWORD    "newwirel3ss"
#define CUSTOM_IP   "192.168.1.171"
#define HOST_NAME   "192.168.1.201"
#define DEVICE_ID   1

volatile bool readCard = false;
volatile bool hasStdby = false;

char ctmpChar;
char cardID[10];
char    sendHTTP[128];
bool    bREINIT = true;
bool    bRecvCardID = false;

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
    pinMode(READY_TOGGLE_PIN, OUTPUT);

    if(digitalRead(STDBY_TOGGLE_PIN) == HIGH){
        hasStdby = true;
    }

    Serial.begin(19200);
    while(!Serial){
      ;
    }
    Serial.print("setup begin\r\n");

    if( bREINIT ){
        
        if (wifi.setOprToStation()) {
            if( hasStdby ){
                Serial.print("to station + softap ok\r\n");
            }
        } else {
            if( hasStdby ){
                Serial.print("to station + softap err\r\n");
            }
        }

        while(!wifi.joinAP(SSID, PASSWORD)){
            if( hasStdby ){
                Serial.print("Join AP failure\r\n");
            }
        }
        
        if( hasStdby ){
            Serial.print("Join AP success\r\n");
            Serial.print("IP:");
            Serial.println( wifi.getLocalIP().c_str());   

            Serial.print("Set Local IP to ");
            Serial.println(CUSTOM_IP);

            Serial.print("New IP:");
            Serial.println( wifi.getLocalIP().c_str());
        }
    }
    
    if( bREINIT ){
    
        if (wifi.disableMUX()) {
            if( hasStdby ){
                Serial.println("single ok");
            }
        } else {
            if( hasStdby ){
                Serial.println("single err");
            }

            while(!wifi.disableMUX()){
                if( hasStdby ){
                    Serial.println("Retry singleMUX");
                }
            }
        }
    }
    
    if( hasStdby ){
        Serial.println("setup end");
        flushHWSerial(); 
    }

    // FIXME send read signal to other arduino
}
 
void loop(void)
{
    clean_res = Serial.readString();

    if( clean_res != "" ){
        if( clean_res.charAt(0) == '@'){
            bRecvCardID = true;
            readCard = true;
            clean_res = clean_res.substring(1);
            for(uint8_t idx=0; idx < clean_res.length(); idx++){
                cardID[idx] = clean_res.charAt(idx);
            }
        }
    }

    if( readCard ){

        clean_res = "";
        bRecvCardID = false;
        readCard = false;

        if (!wifi.createTCP(HOST_NAME, HOST_PORT)) {
            Serial.println("create tcp err");
            refreshCardID();
            return;
        }

        sprintf(sendHTTP,"GET /hellocard.php?id=%i&cid=%s HTTP/1.1\r\nHost: 192.168.1.201\r\nConnection: close\r\n\r\n", DEVICE_ID, (char*)cardID);
        wifi.send((const uint8_t*)sendHTTP, strlen(sendHTTP));

        clean_res = Serial1.readString();
        //Serial.println(clean_res);

        clean_res = cleanStartBrace(clean_res);
        clean_res = cleanEndBrace(clean_res);

        //StaticJsonBuffer<96> jsonBuffer;
        //JsonObject &jsonObject = jsonBuffer.parseObject(clean_res);

        //if( !jsonObject.success()){
        //    Serial.println( "Error Load" ); 
        //    wifi.releaseTCP();
        //    refreshCardID();
        //    return;
        //}
        //
        //String cr = jsonObject["cr"];
        //Serial.print( "CR : " ); 
        Serial.println( clean_res ); 
        digitalWrite(READY_TOGGLE_PIN,HIGH);
        delay(100);
        digitalWrite(READY_TOGGLE_PIN,LOW);

        wifi.releaseTCP();
        refreshCardID();
    }
    
    //delay(3000);
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

void flushHWSerial(void){
    while(Serial.available()){
      ; // wait until all buffer are flush
    }
}

void refreshCardID(){
    for(uint8_t i=0; i < 10; i++){
        cardID[i] = ' ';
    }
    //for(uint8_t i=0; i < 128; i++){
    //    sendHTTP[i] = '\0';
    //}
}
