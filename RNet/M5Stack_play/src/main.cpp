#include <M5Stack.h>
#include <mcp_can.h>
#include "esp_task_wdt.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "RnetDictionnary.hpp"
#include "RnetPlayer.hpp"
#include "SDServer.hpp"


//TODO: Recupérer identifiant 8 bytes Joystick dans identifier
uint8_t identifier[8] = {0xE5,0x80,0x13,0x7C,0x00,0x00,0x00,0x00};
//TODO: Récupérer identifiant message Joystick
#define JOY_ID  0x82000100
//TODO: Récupérer automatiquement Trigger Joy et Trigger HB
#define START_TIMER_JOY_MSG 0x9C300204
#define START_TIMER_HB_MSG  0x9C0C0200


#define SABLIER_JSM   0
#define INIT_JSM      1
#define SABLIER_LOCAL 2
#define INIT_LOCAL    3

#define JOY_WIFI_TIMEOUT_MS   1000 

String dict_filename[4] = {"/dictSablierJSM.bin", "/dictInitJSM.bin","/dictSablierLocal.bin","/dictInitLocal.bin"};


#define NBMSGSTART  128
bool flagStartTimerJoy = false;
bool flagStartTimerHB = false;
bool start = false;
bool stop = false;
uint32_t nbStartTimerJoy = 0;

int cptMsgStart = 0;

TaskHandle_t CANTask;
bool listenOnly = false;
uint32_t lastMsgTS = 0;
bool reseted = true;
uint32_t canSendTS = 0;
uint32_t joyWifiTS = 0;

File file;



RnetDictionnary dictionnaries[4];
int chosenDict = -1;


#define CAN0_INT 15                              // Set INT to pin 2
MCP_CAN CAN0(12);                               // Set CS to pin 10

RnetPlayer player(&CAN0);

uint32_t now;

typedef enum {IDLE, STARTING, RUNNING, HOURGLASS} RnetState_t;
RnetState_t rnetState = IDLE;


uint8_t joy_data[2] = {0x00, 0x00};
uint8_t hb_data[7] = {0x87,0x87,0x87,0x87,0x87,0x87,0x87};

int state = 0;
bool enabletimerE = false;
uint32_t timerE_ts = 0;

bool enabletimerJoy = false;
uint32_t timerJoy_ts = 0;

bool enabletimerHB = false;
uint32_t timerHB_ts = 0;
bool local = false;

uint32_t flagTimer = 0;

bool flagSendJoy = false;





void feedTheDog(){
  // feed dog 0
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE; // write enable
  TIMERG0.wdt_feed = 1;                       // feed dog
  TIMERG0.wdt_wprotect = 0;                   // write protect
  // feed dog 1
  TIMERG1.wdt_wprotect = TIMG_WDT_WKEY_VALUE; // write enable
  TIMERG1.wdt_feed = 1;                       // feed dog
  TIMERG1.wdt_wprotect = 0;                   // write protect
}

bool newCANMsg(){
  return !digitalRead(CAN0_INT);
}

void sendE(){
  CAN0.sendMsgBuf(0xE, 8, identifier);
}

void sendC(){
  CAN0.sendMsgBuf(0xC, 0, identifier);
}

void sendJoy(){
  CAN0.sendMsgBuf(JOY_ID, 2, joy_data);
}

void sendHB(){
  CAN0.sendMsgBuf(0x83c30f0f, 7, hb_data);
}

bool checkStartTimerHB(){
  return flagStartTimerHB;
}

bool checkStartTimerJoy(){
  if(nbStartTimerJoy>=2){
    return true;
  }
  return false;
}

bool AckReceived(){
  return ((CAN0.getTXB0CTRL() & 0x08) != 0x08);
}

void timerE(){
  if(enabletimerE){
    uint32_t now = millis();
    if((uint32_t)(now - timerE_ts) >= 50){
      sendE();
      timerE_ts = millis();
    }
  }
}

void timerJoy(){
  if(enabletimerJoy){
    uint32_t now = millis();
    if((uint32_t)(now - timerJoy_ts) >= 10){
      sendJoy();
      timerJoy_ts = millis();
    }
  }
}

void timerHB(){
  if(enabletimerHB){
    uint32_t now = millis();
    if((uint32_t)(now - timerHB_ts) >= 100){
      sendHB();
      timerHB_ts = millis();
    }
  }
}

bool isEnd(const RnetDefinition::rnet_msg_t &msg){
  if((msg.canId & 0x3FFFFFFF) == 0x00 && msg.len == 0){
    return true;
  } else {
    return false;
  }
}

void ResetCAN(){
  if(!reseted){
    state = 0;
    enabletimerE = false;
    enabletimerJoy = false;
    enabletimerHB = false;
    cptMsgStart = 0;
    for(int i = 0 ; i < 4 ; i++) dictionnaries[i].resetDone();
    chosenDict = -1;
    CAN0.setMode(MCP_LISTENONLY);  
    reseted = true;
  }
}

void timerStateMachine(){
  switch(state){
      case 0 :
      enabletimerE = false;
      rnetState = IDLE;
      if(start){
        rnetState = STARTING;
        start = false;
        reseted = false;
        sendC();
        canSendTS = millis();
        state = 10;
      }
      if(stop) 
        state = 0xFF;
      break;
      case 10:
        if((uint32_t)(millis() - canSendTS) > 10){
          state = 1;
        }
      break;
      case 1 :
        if(AckReceived()){
          enabletimerE = true;
          flagStartTimerHB = false;
          state = 2;
        }
        if(stop) 
          state = 0xFF;
      break;
      case 2:
        if(checkStartTimerHB()){
          flagStartTimerJoy = false;
          nbStartTimerJoy = 0;
          enabletimerHB = true;
          state = 3;
        }
        if(stop) 
          state = 0xFF;
      break;
      case 3:
        if(checkStartTimerJoy()){
          flagStartTimerJoy = false;
          enabletimerJoy = true;
          state = 0xFF;
        }
        if(stop) 
          state = 0xFF;
      break;
      case 0xFF:
        rnetState = RUNNING;
        if(stop){
          stop = false;
          ResetCAN();
          state = 0;
        }
      break;
      default:
      break;
    }
    timerE();
    timerHB();
    timerJoy();
}

void canCode(void * parameter) {
  while(1){
    if(newCANMsg()){
      RnetDefinition::rnet_msg_t msg;
      msg.tstamp = micros();
      CAN0.readMsgBuf(&msg.canId, &msg.len, msg.buf);
      lastMsgTS = millis();
      if(msg.canId == START_TIMER_JOY_MSG){
        nbStartTimerJoy++;
      }
      if(msg.canId == START_TIMER_HB_MSG){
        flagStartTimerHB = true;
      }
      if(isEnd(msg)){
        ResetCAN();
      } else {
        if(!local && cptMsgStart < NBMSGSTART){
          cptMsgStart++;
          if(!listenOnly && cptMsgStart >= NBMSGSTART){
            CAN0.setMode(MCP_NORMAL); 
            reseted = false;
          }
        }else{
          if(chosenDict == -1){
            for(int i = 0 ; i < 4 && chosenDict == -1 ; i++){
              if((local) && (i < 2)) continue;
              if((!local) && (i >= 2)) continue;
              if(dictionnaries[i].getTriggerId(msg) == 0){
                chosenDict = i;
              }
            }
          } 
          if(chosenDict >= 0)
          {
            int32_t triggerId = dictionnaries[chosenDict].getTriggerId(msg);
            if(triggerId >= 0){
              Serial.printf("%d\n", triggerId);
              player.setDefAndTS(dictionnaries[chosenDict].getDefinition(triggerId), msg.tstamp);
              dictionnaries[chosenDict].setDone(triggerId);
            }
          }
        }
      }
    }

    player.play();

    if(local)
      timerStateMachine(); 

    feedTheDog();
  }
}


//——————————————————————————————————————————————————————————————————————————————
//   SETUP
//——————————————————————————————————————————————————————————————————————————————



/* @brief init can communication 
*/
bool init_can(){
  bool ret = false;
  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(CAN0.begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK)
    ret = true;
  CAN0.setMode(MCP_LISTENONLY);  
  pinMode(CAN0_INT, INPUT);                            // Configuring pin for /INT input
  return ret;
}

void setup() {
  M5.Power.begin();
  M5.begin(true, true, false, false);
  setCpuFrequencyMhz(240);

  SDServerSetup();


  /* Start serial */
  Serial.begin(115200) ;
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(0, 0);
  if(!init_can()){
    M5.Lcd.println("CAN KO");
    while(1);
  } else {
    M5.Lcd.println("CAN OK");
  }  

  M5.Lcd.setTextSize(2);
  for(int i = 0 ; i < 4 ; i++){
    M5.lcd.print(dict_filename[i]);
    M5.Lcd.print(":");
    file = SD.open(dict_filename[i].c_str(),"rb", false);
    if(file){
      if(dictionnaries[i].readFromFile(file)){
        M5.Lcd.println("OK"); 
      } else {
        M5.Lcd.println("NOK"); 
      }
      file.close();
      dictionnaries[i].print();
    } else {
        M5.Lcd.println("NOK"); 
    }
  }

  now = micros();

 xTaskCreatePinnedToCore(canCode, 
                          "can", 
                          3000,  
                          NULL,  
                          10,  
                          &CANTask,
                          0);

  server.on("/start", HTTP_GET, []() {
    if(rnetState == IDLE){
      CAN0.setMode(MCP_NORMAL);
      local = true;
      start = true;
      M5.lcd.println("Start");
      String message = "Wheelchair Started\n\n";
      server.send(200, "text/plain", message);
    } else {
      String message = "Wheelchair not IDLE\n\n";
      server.send(200, "text/plain", message);
    }

  }
  );

  server.on("/stop", HTTP_GET, []() {
    if(rnetState != IDLE){
      ResetCAN();
      local = false;
      M5.lcd.println("Stop");
      String message = "Wheelchair Stopped\n\n";
      server.send(200, "text/plain", message);
    } else {
      String message = "Wheelchair IDLE\n\n";
      server.send(200, "text/plain", message);
    }
  }
  );

  server.on("/joy", HTTP_GET, []() {
    if(rnetState == RUNNING){
      joyWifiTS = millis();
      if(server.hasArg("x") && server.hasArg("y")){
        int8_t joyX = atoi(server.arg("x").c_str());
        int8_t joyY = atoi(server.arg("y").c_str());

        if(abs(joyX) > 50){
          joyX = 0;
        }
        if(abs(joyY) > 50){
          joyY = 0;
        }
        joy_data[0] = joyY;
        joy_data[1] = joyX;
        String message = "Joy Position sent\n\n";
        server.send(200, "text/plain", message);
      } else {
        String message = "Args x and/or y missing\n\n";
        server.send(200, "text/plain", message);
      }
    } else {
      String message = "Wheelchair not running\n\n";
      server.send(200, "text/plain", message);
    }
  }
  );



}

//——————————————————————————————————————————————————————————————————————————————
//   LOOP
//——————————————————————————————————————————————————————————————————————————————

void loop () {
  M5.update();
  SDServerLoop();
  /* if Button A Pressed, set listen only mode/Normal mode for debug purposes */
  if(M5.BtnA.wasPressed()){
    if(listenOnly){
      listenOnly = false;
      CAN0.setMode(MCP_NORMAL);
      M5.lcd.println("Normal");
    } else {
      listenOnly = true;
      CAN0.setMode(MCP_LISTENONLY);
      M5.lcd.println("ListenOnly");

    }
  }
  /* if Button B Pressed, start/stop the wheelchair */
  if(M5.BtnB.wasPressed()){
    if(local){
      ResetCAN();
      local = false;
      M5.lcd.println("Stop");
    } else {
      CAN0.setMode(MCP_NORMAL);
      local = true;
      start = true;
      M5.lcd.println("Start");
    }
  }

  /* if Button B Pressed, move the wheelchair slightly to hear the brakes (DEBUG) */
  if(M5.BtnC.wasPressed()){
    if(!flagSendJoy){
      joy_data[1] = 50;
      flagSendJoy = true;
    } else {
      joy_data[1] = 0;
      flagSendJoy = false;      
    }
  }

  if((uint32_t)(millis() - joyWifiTS) > JOY_WIFI_TIMEOUT_MS){
    joy_data[0] = 0;
    joy_data[1] = 0;
  }

  vTaskDelay(10);
  feedTheDog();
}