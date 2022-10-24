#include <M5Stack.h>
#include <mcp_can.h>
#include "esp_task_wdt.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include <CircularBuffer.h>
#include "RnetDictionnary.hpp"

#define __DEBUG__

TaskHandle_t CANTask;
volatile uint32_t now = 0;
volatile int prevValue = 0;
volatile int prevValueRX = 1;
volatile uint32_t highNow = 0;
volatile int prevDirection = 0;
volatile int currentDirection = 0;
bool canReceiving = false;
bool gpioCodeRunning = false;
bool recording = false;

CircularBuffer<RnetDefinition::rnet_msg_t, 5000> msgs; // circular buffer for Rnet msg buffering

#define CAN0_INT 15                              // Set INT to pin 2
MCP_CAN CAN0(12);                               // Set CS to pin 10


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

/* @brief read can RX and TX and determine msgs direction (JOY or PM) 
*/
void IRAM_ATTR gpioCode(){
  if(!gpioCodeRunning){
    gpioCodeRunning = true;
    now = micros();
    int valueRX = digitalRead(22);
    int valueTX = digitalRead(21);
    /* if no change for 80 ms, there is a new msg */
    if(canReceiving && prevValueRX && (uint32_t)(now - highNow) > 80){
      prevDirection = (currentDirection > 0);
      currentDirection = 0;
      canReceiving = false;
    }
    /* determine direction (JOY or PM) */
    if(!valueRX){
      canReceiving = true;
      if(!valueTX){
        currentDirection ++;
      } else {
        currentDirection --;
      }
    } else {
      if(!prevValueRX)
        highNow = now;
    }
    prevValueRX = valueRX;
    gpioCodeRunning = false;
  }
}

/* @brief buffering msgs in the circular buffer after setting the direction (JOY or PM) */
void canCode( void * parameter) {
  while(1){
    if(newCANMsg()){
      RnetDefinition::rnet_msg_t msg;
      msg.tstamp = micros();
      CAN0.readMsgBuf(&msg.canId, &msg.len, msg.buf);
      gpioCode();
      msg.direction = prevDirection;
      if(recording) msgs.push(msg);
#ifdef __DEBUG__
      if(prevDirection){
        digitalWrite(2,HIGH);
        delayMicroseconds(1);
        digitalWrite(2,LOW);
      } else {
        digitalWrite(5,HIGH);
        delayMicroseconds(1);
        digitalWrite(5,LOW);
      }
#endif
    }
  }
  feedTheDog();
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
#ifdef __DEBUG__
  pinMode(2,OUTPUT);
  digitalWrite(2,LOW);
  pinMode(5,OUTPUT);
  digitalWrite(5,LOW);
#endif
  pinMode(21, INPUT);
  pinMode(22, INPUT);
  attachInterrupt(22, gpioCode, CHANGE);

  /* Start serial communication and setup LCD screen */
  Serial.begin (115200) ;
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(0, 0);
  if(!init_can()){
    M5.Lcd.println("CAN KO");
    while(1);
  } else {
    M5.Lcd.println("CAN OK");
  }  

  now = micros();

 /* @brief thread for the can task
 */
 xTaskCreatePinnedToCore(canCode, 
                          "can", 
                          3000,  
                          NULL,  
                          10,  
                          &CANTask,
                          0);
}

//——————————————————————————————————————————————————————————————————————————————
//   LOOP
//——————————————————————————————————————————————————————————————————————————————

void loop () {
  M5.update();
  /* if Button A pressed, start recording can msgs */
  if(M5.BtnA.wasPressed()){
    msgs.clear();
    recording = true;
    M5.lcd.printf("Recording");
  }

  /* if Button B pressed, end recording can msgs and print dict.txt and write dict.bin */
  if(M5.BtnB.wasPressed()){
    recording = false;
    M5.lcd.printf(": Done\n");
    M5.lcd.printf("Writing");
    RnetDictionnary dict;
    File file;
    file = SD.open("/dump.txt","w", true);
    while(msgs.size()){
      RnetDefinition::rnet_msg_t msg = msgs.shift();
      /* following printf for debug only : adding delays that may miss some data on the can bus read with logic analyzer */
      file.printf("[%9d][%s][%x]:", msg.tstamp, (msg.direction?"JOY":"PM"), msg.canId);
      for(int i = 0 ; i < msg.len ; i++){
        file.printf("%02x", msg.buf[i]);
      }
      file.printf("\n");
      
      if(msg.direction){  //msg from JOY
        dict.addAnswerToLastTrigger(msg);
      } else {            //msg from PM
        dict.addPossibleTrigger(msg);
      }
    }
    file.close();
    file = SD.open("/dict.txt","w", true);
    dict.printToFile(file);
    file.close();
    file = SD.open("/dict.bin","w", true);
    dict.writeToFile(file);
    file.close();
    M5.lcd.printf(": Done\n");
  }

  if(M5.BtnC.wasPressed()){
    M5.lcd.printf("Cleaning");
    SD.remove("/dict.txt");
    SD.remove("/dict.bin");
    SD.remove("/dump.txt");
    M5.lcd.printf(": Done\n");
  }

  vTaskDelay(10);
  feedTheDog();
}