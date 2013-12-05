/******************************************************************************
main.cpp for transmittitting bodycomm data

By David Cranor
cranor@mit.edu
8/20/2013

 * The MIT License
 *
 * Copyright (c) 2013, MIT Media Lab
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

#include <wirish/wirish.h>

//#define OUTPUT_PERIOD_US 8
#define AGC_INIT_TIME_US 6000
#define AGC_STABILIZATION_TIME_US 500
#define AGC_GAP_TIME_US 500
#define OUTPUT_ENABLE_HIGH_TIME_US 4200
#define OUTPUT_ENABLE_LOW_TIME_US 4200

//May need to be radically slower (like 800) for manchester encoding
//At 800, bitrate is 417bps, ick.

#define BIT_TIME_US 400
#define HALF_BIT_TIME_US (BIT_TIME_US/2)

#define BIT_HIGH_TIME_US 135
#define BIT_LOW_TIME_US 275

#define OUTPUT_PIN 8
#define TEST_PIN 21 //changed for design
#define QUENCH_PIN 11  //Changed for design

 //uint16_t testData = 0b0101010101010101;
 uint16_t testData = 0b0001010101010101;
 //uint16_t testData = 0xAAAA;

HardwareTimer carrierTimer(2);
uint8_t outputStatus = 0;

void initTimer();

void enableOutput();
void disableOutput();

void sendDataNRZ(uint16_t data);

void sendDataManchester(uint16_t data);
void manchesterZero();
void manchesterOne();

uint8_t computeParity(uint16_t data);


int main()
{
    //For debugging
    //delay(3000);

    //Start the timer for carrier wave 
    initTimer();
    pinMode(TEST_PIN, OUTPUT);
    digitalWrite(TEST_PIN, LOW);

    pinMode(QUENCH_PIN, OUTPUT);
    digitalWrite(QUENCH_PIN, HIGH);


    while(1)
    {
        //This is experimental.  For 'good' data transmission, use sendDataNRZ();
        sendDataManchester(testData);
        //sendDataNRZ(testData);

        //enableOutput();
        //while(1);
        // delayMicroseconds(300);

        // disableOutput();
        // delayMicroseconds(300);

        //carrierTimer.resume();

        //LEAVE THIS HERE OR THE RECEIVER WILL NEVER RESET
        delay(30);
    }
}


// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void premain() {
    init();
}


void enableOutput()
{
    //Now with the TRIAC
    digitalWrite(QUENCH_PIN, LOW);
    carrierTimer.resume();
    
}

void disableOutput()
{
    carrierTimer.pause();
    digitalWrite(QUENCH_PIN, HIGH);
    //carrierTimer.refresh();
    carrierTimer.setCount(289);
}


uint8_t computeParity(uint16_t data)
{
    //Parity bit needs to be set such that the data contains an odd number of set bits.
    uint8_t parityBit = 1;

    for(uint8_t i = 16; i > 0; i--)
    {
        if( ((data >> (i - 1)) & 1 ) == 1)
        {
            parityBit = !parityBit; 
        }
    }
    return parityBit;
}

void sendDataNRZ(uint16_t data)
{
    //See page 37 of the datasheet.

    //Stabalize the AGC
    //carrierTimer.resume();
    enableOutput();
    delayMicroseconds(AGC_INIT_TIME_US);

    //Hold it
    delayMicroseconds(AGC_STABILIZATION_TIME_US);

    //AGC gap pulse
    //carrierTimer.pause();
    //carrierTimer.refresh();
    disableOutput();
    delayMicroseconds(AGC_GAP_TIME_US);

    //Activate Output Enable filter with a high and low pulse
    //carrierTimer.resume();
    enableOutput();
    delayMicroseconds(OUTPUT_ENABLE_HIGH_TIME_US);

    //carrierTimer.pause();
    //carrierTimer.refresh();

    disableOutput();
    delayMicroseconds(OUTPUT_ENABLE_LOW_TIME_US);

    //Send the data, MSB first, NRZ encoding
    for(uint8_t i = 16; i > 0; i--)
    {
        if( ((data >> (i - 1)) & 0x01 ) == 1)
        {
            digitalWrite(TEST_PIN, HIGH);
            //carrierTimer.resume();
            enableOutput();
            delayMicroseconds(BIT_TIME_US);
            digitalWrite(TEST_PIN, LOW);
        } else {
            digitalWrite(TEST_PIN, HIGH);
            //carrierTimer.pause();
            //carrierTimer.refresh();
            disableOutput();
            delayMicroseconds(BIT_TIME_US);
            digitalWrite(TEST_PIN, LOW);
        }
        disableOutput();
        //carrierTimer.pause();
        //carrierTimer.refresh();
    }
}

void sendDataManchester(uint16_t data)
{
    //See page 37 of the datasheet.

    //Stabalize the AGC
    //carrierTimer.resume();
    enableOutput();
    delayMicroseconds(AGC_INIT_TIME_US);

    //Hold it
    delayMicroseconds(AGC_STABILIZATION_TIME_US);

    //AGC gap pulse
    //carrierTimer.pause();
    //carrierTimer.refresh();
    disableOutput();
    delayMicroseconds(AGC_GAP_TIME_US);

    //Activate Output Enable filter with a high and low pulse
    //carrierTimer.resume();
    enableOutput();
    delayMicroseconds(OUTPUT_ENABLE_HIGH_TIME_US);

    //carrierTimer.pause();
    //carrierTimer.refresh();
    disableOutput();
    delayMicroseconds(OUTPUT_ENABLE_LOW_TIME_US);

    //enableOutput();
    //delayMicroseconds(BIT_TIME_US/2);

    //Send the data, MSB first, Manchester encoding
    for(uint8_t i = 16; i > 0; i--)
    {
        if( ((data >> (i - 1)) & 0x01 ) == 1)
        {
            //digitalWrite(TEST_PIN, HIGH);
            manchesterOne();
            //digitalWrite(TEST_PIN, LOW);
        } else {
            //digitalWrite(TEST_PIN, HIGH);
            manchesterZero();
            //digitalWrite(TEST_PIN, LOW);
        }

        //delayMicroseconds(BIT_TIME_US);

    }

    disableOutput();
    digitalWrite(TEST_PIN, LOW);
}

/********************************************************************
NOTE: For manchester encoding:

1 0 (falling edge) is a 0
0 1 (rising edge) is a 1

Need each transmission to start with 0 so that receiver can align.
********************************************************************/
void manchesterZero()
{
    //High
    //carrierTimer.resume();
    digitalWrite(TEST_PIN, HIGH);
    enableOutput();
    delayMicroseconds(HALF_BIT_TIME_US);
    
    //Low
    //carrierTimer.pause();
    //carrierTimer.refresh();
    digitalWrite(TEST_PIN, LOW);
    disableOutput();
    delayMicroseconds(HALF_BIT_TIME_US);
}

void manchesterOne()
{
    //Low
    //carrierTimer.pause();
    //carrierTimer.refresh();
    digitalWrite(TEST_PIN, LOW);
    disableOutput();
    delayMicroseconds(HALF_BIT_TIME_US);


    //High
    //carrierTimer.resume();
    digitalWrite(TEST_PIN, HIGH);
    enableOutput();
    delayMicroseconds(HALF_BIT_TIME_US);
}

void initTimer()
{
    pinMode(OUTPUT_PIN, PWM);

    //Setup the output timer
    carrierTimer.pause();
  
    //Set up period
    //carrierTimer.setPeriod(OUTPUT_PERIOD_US); //in microseconds
    carrierTimer.setPrescaleFactor(1);
    carrierTimer.setOverflow(576);
  
     // Set up an interrupt on channel 1
    carrierTimer.setMode(TIMER_CH4, TIMER_PWM);
    carrierTimer.setCompare(TIMER_CH4, 288);  // Interrupt 1 count after each update

    // Refresh the timer's count, prescale, and overflow
    carrierTimer.refresh();
}
