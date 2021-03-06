/*******************************************************************************
Copyright 2021
Steward Observatory Engineering & Technical Services, University of Arizona
This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*******************************************************************************/

/**
 * @file thermistor_Mux.cpp
 * @author Nestor Garcia (Nestor212@email.arizona.edu)
 * @brief Main file, complete code cycles through NUMBER_OF_THERMISTORS(32) mosfets; each conencted to a thermistor,
 * and uses ADC module MC3561R to convert analog to digital data. Internal temperature data of
 * ADC chip is also gathered. This file contains a calibration routine & implements teensy EEPROM saving & clearing capabilities. 
 * @version (see THERMISTOR_MUX_VERSION in thermistorMux_global.h)
 * @date 2022-05-02
 *
 * @copyright Copyright (c) 2022
 */

#include "command_ADC.h"
#include "thermistorMux_network.h"
#include "thermistorMux_hardware.h"
#include "thermistorMux_global.h"
#include "thermistor_Mux.h"

/*
Questions:
1) What kind of network will this be connected to? MQTT
2) How to set/change skew?
3) 
*/

   
#define CS 10
#define INTERRUPT_PIN 23

/*
Array representing 32 Mosfets
mosfet[0] = header pin 0; mosfet Q1
mosfet[1] = header pin 1; mosfet Q2
...
mosfet[31] = header pin 22; mosfet Q32
*/
static unsigned int mosfet[NUMBER_OF_THERMISTORS] = {0,1,2,3,4,5,6,7,8,9,24,25,26,27,28,29,30,31,
                                  32,36,37,40,41,14,15,16,17,18,19,20,21,22};                                 
static int irqFlag = 0;
unsigned int eeAddr;
bool setup_successful = false;
int mosfetRef;
bool calibrated = false;
static float ref_Low = {0.00};
static float ref_High = {0.00};
static float raw_Low[NUMBER_OF_THERMISTORS] = {0.00};
static float raw_High[NUMBER_OF_THERMISTORS] = {0.00};



void IRQ() {
  irqFlag = 1;
}


bool clear_cal_data() {

  EEPROM.write(0, 0x00);
  eeAddr = 1;
  mosfetRef = 0;
  int eeAddr_last = 0;

  //Clear reference temperature values from EEPROM
  EEPROM.write(eeAddr, 0x00);
  eeAddr += sizeof(ref_Low); //Move address to the next byte after float 'ref_Low'.
  EEPROM.write(eeAddr, 0x00);
  eeAddr += sizeof(ref_High); //Move address to the next byte after float 'ref_High'.

  //Clear raw temperature values from EEPROM
  while (eeAddr_last < (NUMBER_OF_THERMISTORS * 2)) {
    EEPROM.write(eeAddr, 0x00);
    eeAddr += sizeof(raw_Low[mosfetRef]); //Move address to the next byte after float 'raw_Low'.
    EEPROM.write(eeAddr, 0x00);
    eeAddr += sizeof(raw_High[mosfetRef]); //Move address to the next byte after float 'raw_High'.
    eeAddr_last = eeAddr_last + 2;
    mosfetRef++;
  }

  //Clear calibration values from firmware.
  ref_Low = {0.00};
  ref_High = {0.00};
  for( mosfetRef = 0; mosfetRef < NUMBER_OF_THERMISTORS; mosfetRef++){
    raw_Low[mosfetRef] = {0.00};
    raw_High[mosfetRef] = {0.00};
  }
  calibrated = false;
  return true;
}


bool cal_thermistor(float ref_temp, int tempNum){
    eeAddr = 1;
    irqFlag = 0;
    Serial.printf("Set temp is %0.2f, calibration begun.\n", ref_temp);
    setThermistorMuxRead();
    delay(1);
    for(int mosfetRef = 0; mosfetRef < NUMBER_OF_THERMISTORS; mosfetRef++) {
        digitalWrite(mosfet[mosfetRef], HIGH);
        start_conversion();
        
        while (irqFlag == 0) {
          delay(1); //Wait for interrupt 
        }
        irqFlag = 0;

        float raw_temp = read_ADCDATA();

        if (tempNum == 1) {
          ref_Low = ref_temp;
          raw_Low[mosfetRef] = raw_temp; 
          Serial.println("Cal data 1 INW");
        } 
        else if (tempNum == 2) {
          ref_High = ref_temp;
          raw_High[mosfetRef] = raw_temp;
          Serial.println("Cal data 2 INW");
          //raw_Low[mosfetRef] = (ref_High - ref_Low) / (raw_High[mosfetRef] - raw_Low[mosfetRef]);
         // raw_High[mosfetRef] = raw_High[mosfetRef] - (raw_Low[mosfetRef] * ref_High); 

        }
        digitalWrite(mosfet[mosfetRef], LOW);
        
        if (eeAddr == 1) {
          EEPROM.put(eeAddr, ref_Low);
          eeAddr += sizeof(ref_Low); //Move address to the next byte after float 'f'.
          EEPROM.put(eeAddr, ref_High);
          eeAddr += sizeof(ref_High); //Move address to the next byte after float 'f'.
        }
        Serial.printf("Read thermistor temp = %0.2f Calculated cal value 1 = %0.2f, cal value 2 = %0.2f\n", raw_temp, raw_Low[mosfetRef], raw_High[mosfetRef]);
        EEPROM.put(eeAddr, raw_Low[mosfetRef]);
        eeAddr += sizeof(raw_Low[mosfetRef]); //Move address to the next byte after float 'f'.
        EEPROM.put(eeAddr, raw_High[mosfetRef]);
        eeAddr += sizeof(raw_High[mosfetRef]); //Move address to the next byte after float 'f'.
    }
    Serial.println(eeAddr);
    
    if (tempNum == 2) {
      EEPROM.write(0, 0x01);
      calibrated = true;
      Serial.println("Calibration complete.");
      return true;
    }
    else {
      return false;
    }
}


void setup() {
  //MOSFET digital control I/O ports, set to output. All MOSFETS turned off (pins set to LOW).
  for (int mosfetRef = 0; mosfetRef < NUMBER_OF_THERMISTORS; mosfetRef++) {
    pinMode(mosfet[mosfetRef], OUTPUT);  
    digitalWrite(mosfet[mosfetRef], LOW);
  }
  //INW: figure out how to set skew

  /*
  Enable global interrupts. 
  Set up ADC interrupt feature on teensy pin 23. 
  Upon recieving an interrupt from ADC(indicating new data is available in ADC),
  IRQ flag is triggered.
  */
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), IRQ, FALLING);
  sei();

  setup_successful = hardwareID_init() && initTeensySPI() && initADC() && network_init();
  
  if(setup_successful){
    Serial.println("Setup successful.");
    check_brokers();
  }
  else {
    Serial.println("Setup Failed.");
  }

  if (EEPROM.read(0) == 0x01) {
    calibrated = true;
    eeAddr = 1;
    mosfetRef = 0;
    int eeAddr_last = 0;
    if (eeAddr == 1) {
      EEPROM.get(eeAddr, ref_Low);
      eeAddr += sizeof(ref_Low); //Move address to the next byte after float 'f'.
      EEPROM.get(eeAddr, ref_High);
      eeAddr += sizeof(ref_High); //Move address to the next byte after float 'f'.
    }
    while (eeAddr_last < 64) {
      EEPROM.get(eeAddr, raw_Low[mosfetRef]);
      eeAddr += sizeof(raw_Low[mosfetRef]); //Move address to the next byte after float 'f'.
      EEPROM.get(eeAddr, raw_High[mosfetRef]);
      eeAddr += sizeof(raw_High[mosfetRef]); //Move address to the next byte after float 'f'.
      eeAddr_last = eeAddr_last + 2;
      mosfetRef++;
    }
    publish_refs(ref_Low, ref_High);
  }
}


void loop() {
  int avgCount = 0;
  float thermistor_temp[NUMBER_OF_THERMISTORS] = {0.00};
  float ADC_internal_temp = 0;

  check_brokers();


  //Cycle through mofets; setting digital control pin high, calls on 
  //function that sets mux register to read thermistor inputs.
  //Takes average of 5 data values for each mosfet & internal temp, then resets data buffers.
  while(avgCount < 5) {
    setThermistorMuxRead();
    delay(1);
    for(mosfetRef = 0; mosfetRef < NUMBER_OF_THERMISTORS; mosfetRef++) {
      digitalWrite(mosfet[mosfetRef], HIGH);
      start_conversion();
      
      while (irqFlag == 0) {
        delay(1); //Wait for interrupt 
      }
      irqFlag = 0;

    if(thermistor_temp[mosfetRef] == 0.00) {
        thermistor_temp[mosfetRef] = read_ADCDATA();
      }
      else {
        thermistor_temp[mosfetRef] = (thermistor_temp[mosfetRef] + read_ADCDATA()) / (2); 
      }
      digitalWrite(mosfet[mosfetRef], LOW);
    }
    //Calls on function that sets mux register to read internal ADC temperature. 
    setADCInternalTempRead();
    delay(1);
    start_conversion();
    
    while (irqFlag == 0) {
      delay(1); //Wait for interrupt
    }
    irqFlag = 0;

    if (ADC_internal_temp == 0) {
      ADC_internal_temp = read_ADCDATA();
    }
    else {
      ADC_internal_temp = (ADC_internal_temp + read_ADCDATA()) / (2);
    }
    avgCount++;
  }

  Serial.printf("Internal ADC temperature: %0.2f ??C\n", ADC_internal_temp);

  if (calibrated == true) {
    for (mosfetRef = 0; mosfetRef < NUMBER_OF_THERMISTORS; mosfetRef++){
      thermistor_temp[mosfetRef] = (((thermistor_temp[mosfetRef] - raw_Low[mosfetRef]) * (ref_High - ref_Low)) / (raw_High[mosfetRef] - raw_Low[mosfetRef])) + ref_Low;
      Serial.printf("Thermistor %d temperature: [((raw temp - %0.2f) * (%0.2f - %0.2f)) / (%0.2f - %0.2f)] + %0.2f =  %0.2f ??C\n", 
                    mosfetRef + 1, raw_Low[mosfetRef], ref_High, ref_Low, raw_High[mosfetRef], raw_Low[mosfetRef], ref_Low, thermistor_temp[mosfetRef]);
    }
  }
  else {
    for (mosfetRef = 0; mosfetRef < NUMBER_OF_THERMISTORS; mosfetRef++){
      Serial.printf("Thermistor uncalibrated temperature = %0.2f ??C\n", thermistor_temp[mosfetRef]);
    }
  }
  Serial.println();
  publish_data(thermistor_temp, ADC_internal_temp);
}





