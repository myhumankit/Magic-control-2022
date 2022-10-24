#include "arduino.h"
#include <FastGPIO.h>
#define CAN0_RX_PIN 2 //RX -> INPUT
#define CAN0_TX_PIN 3 //TX -> OUTPUT
#define CAN1_RX_PIN 4
#define CAN1_TX_PIN 5
#define DELAY_NOP   12

int state = 0;

void setup(){
  FastGPIO::Pin<CAN0_TX_PIN>::setOutputHigh();
  FastGPIO::Pin<CAN0_RX_PIN>::setInput();
  FastGPIO::Pin<CAN1_TX_PIN>::setOutputHigh();
  FastGPIO::Pin<CAN1_RX_PIN>::setInput();
}



void loop(){
  noInterrupts(); // disable interrupts (issue with delays)
  
  while(1){
    if(!FastGPIO::Pin<CAN0_RX_PIN>::isInputHigh()){
      FastGPIO::Pin<CAN1_TX_PIN>::setOutputValueLow();
      while(!FastGPIO::Pin<CAN0_RX_PIN>::isInputHigh());
      FastGPIO::Pin<CAN1_TX_PIN>::setOutputValueHigh();
      /* delay using _NOP (execute clock cycles) */
      for(int i = 0 ; i < DELAY_NOP ; i++)
        _NOP();
    }
    if(!FastGPIO::Pin<CAN1_RX_PIN>::isInputHigh()){
      FastGPIO::Pin<CAN0_TX_PIN>::setOutputValueLow();
      while(!FastGPIO::Pin<CAN1_RX_PIN>::isInputHigh());
      FastGPIO::Pin<CAN0_TX_PIN>::setOutputValueHigh();
      /* delay using _NOP (execute clock cycles) */
      for(int i = 0 ; i < DELAY_NOP ; i++)
        _NOP();
    }
  }



/*
    switch(state){
      case 0:
        if(!FastGPIO::Pin<CAN0_RX_PIN>::isInputHigh()){
          FastGPIO::Pin<CAN1_TX_PIN>::setOutputValueLow();
          state = 1;
          break;
        }
        if(!FastGPIO::Pin<CAN1_RX_PIN>::isInputHigh()){
          FastGPIO::Pin<CAN0_TX_PIN>::setOutputValueLow();
          state = 2;
          break;
        }
      break;
      case 1:
        if(FastGPIO::Pin<CAN0_RX_PIN>::isInputHigh()){
          FastGPIO::Pin<CAN1_TX_PIN>::setOutputValueHigh();
          for(int i = 0 ; i < 8 ; i++)
            _NOP();
          state = 0;
        }
      break;
      case 2:
        if(FastGPIO::Pin<CAN1_RX_PIN>::isInputHigh()){
          FastGPIO::Pin<CAN0_TX_PIN>::setOutputValueHigh();
          for(int i = 0 ; i < 8 ; i++)
            _NOP();
          state = 0;
        }
      break;
    } 
    */
}