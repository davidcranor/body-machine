/******************************************************************************
main.cpp for testing Microchip MCP2035

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

//sorry.
#include </Users/davidcranor/Documents/libmaple/libraries/MCP2035/MCP2035.h>

//#define OUTPUT_PERIOD_US 8
#define AGC_INIT_TIME_US 3500
#define AGC_STABILIZATION_TIME_US 500
#define AGC_GAP_TIME_US 500
#define OUTPUT_ENABLE_HIGH_TIME_US 4300
#define OUTPUT_ENABLE_LOW_TIME_US 4300

//Bit time must be divisible by samples per bit
#define BIT_TIME_US 400
#define HALF_BIT_TIME_US (BIT_TIME_US/2)

//How many bits in packet?  For now, includes start bit.
#define PACKET_LENGTH 16

//Set oversampling here.  Try to pick a number that makes the math line up nicely.
#define SAMPLES_PER_BIT 12
#define SAMPLES_PER_HALF_BIT (SAMPLES_PER_BIT / 2)
#define PHASE_TOLERANCE (SAMPLES_PER_HALF_BIT / 2) //50% tolerance
#define NUM_SAMPLES (PACKET_LENGTH * SAMPLES_PER_BIT)

//DOES THIS WORK OR DOES THE DIVISION MESS IT UP?
#define SAMPLING_RATE_US (BIT_TIME_US / SAMPLES_PER_BIT)


#define MCP2035_IO_PIN 12
#define MCP2035_CLK_ALERT_PIN 13
#define MCP2035_CS_PIN 10

#define MCP_2035_RSSI_PIN 9

#define BLUE 3
#define GREEN 4

#define DEBUG_PIN 21

#define WAITING 0
#define DECODING 1

uint8_t debugPinState;

MCP2035 Receiver;

// We'll use timer 2
HardwareTimer timer(2);

uint8_t samplesReady = 0;
uint16_t samplePointer = 0;


uint8_t currentSample = 0;
uint8_t lastSample = 0;

uint8_t sampleArray[NUM_SAMPLES];
uint16_t data;

uint8_t packetReady;
uint8_t decodeMode;
uint8_t samplesCount = 0;

uint16_t receivedData = 0;

uint8_t currentBit = 0;

void initSampleTimer();
void handler_sampleTime();
void resetSampling();
void beginSampling();

uint16_t processSamples(uint8_t* samples, uint8_t packetLength, uint8_t samplesPerBit);

uint8_t computeParity(uint16_t data);




int main()
{
    //For debugging
    //delay(3000);
    pinMode(DEBUG_PIN, OUTPUT);
    digitalWrite(DEBUG_PIN, LOW);
    debugPinState = 0;

    decodeMode = WAITING;

    //Set up the timer
    initSampleTimer();

    //Set up the receiver
    Receiver.begin(MCP2035_IO_PIN, MCP2035_CLK_ALERT_PIN, MCP2035_CS_PIN);

    //Serial port for wireless radio
    Serial1.begin(38400);

    //Reset timer, flags, and buffers
    resetSampling();

    //Go! In this version, sampling and decoding happens in realtime.
    beginSampling();

    packetReady = 0;

    while(1)
    {

        if(packetReady != 0)
        {
            SerialUSB.print(millis()); 
            SerialUSB.print(": ");
            SerialUSB.println(data, BIN);

            packetReady = 0;
        }
    }
}

void initSampleTimer()
{

    // Pause the timer while we're configuring it
    timer.pause();

    // Set up period
    timer.setPeriod(SAMPLING_RATE_US); // in microseconds

    // Set up an interrupt on channel 1
    timer.setChannel1Mode(TIMER_OUTPUT_COMPARE);
    timer.setCompare(TIMER_CH1, 1);  // Interrupt 1 count after each update
    timer.attachCompare1Interrupt(handler_sampleTime);

    // Refresh the timer's count, prescale, and overflow
    timer.refresh();

}

void handler_sampleTime()
{

    //Backup last sample
    lastSample = currentSample;

    //Take the sample
    currentSample = digitalRead(MCP2035_IO_PIN);
    
    //sampleArray[samplePointer] = currentSample;
    
    //Check to see if we are at the end and if so go back to the beginning
    if(samplePointer >= NUM_SAMPLES)
    {
        samplePointer = 0;
    }

    switch(decodeMode)
    {
        case WAITING:
            //See if a low-to-high transition happened (because if we are waiting and see a high/low transition, something is messed up anyway.)
            if(currentSample > lastSample)
            {
                //If so, set decoding flag, start counting and return.
                decodeMode = DECODING;
                currentBit = 0;
                samplesCount++;
                return;

            } else {
            
                //If not, just return and wait for next sample time.
                return;
            }

        break;

        case DECODING:
            //See if it's been too long since last transition and reset/abort if so
            if(samplesCount > SAMPLES_PER_BIT + PHASE_TOLERANCE)
            {
                samplesCount = 0;
                currentSample = 0;
                currentBit = 0;
                decodeMode = WAITING;
                return;
            }
            
            //See if a transition happened
            if(currentSample != lastSample)
            {
                if(samplesCount < (SAMPLES_PER_HALF_BIT + PHASE_TOLERANCE) )
                {
                
                //It was not a bit transition we care about - it was a setup for one.  Just keep counting.
                samplesCount++;
                
                } else if(samplesCount >= (SAMPLES_PER_BIT - PHASE_TOLERANCE)) {     
                    
                    //Add value to decoded packet
                    receivedData = receivedData | (currentSample << (PACKET_LENGTH - currentBit - 1) );
    
                    //Increment current bit
                    currentBit++;

                    //Reset count
                    samplesCount = 0;

                    if(currentBit > 16)
                    {
                        //We made it!  Set the flag and reset everything else.
                        packetReady = 1;

                        samplesCount = 0;
                        currentBit = 0;

                        currentSample = 0;
                        lastSample = 0;

                        decodeMode = WAITING;
                    }
                }
            }

        break;
    }

    //Increment the pointer - DO WE EVEN NEED TO DO THIS?
    //samplePointer++;
}

void resetSampling()
{
    //Pause the timer while we're configuring it
    timer.pause();
    
    //Refresh the timer
    timer.refresh();

    //Reset flags, sample array, and data
    samplesReady = 0;
    samplePointer = 0;
    samplesCount = 0;

    for(uint8_t i = 0; i < NUM_SAMPLES; i++)
    {
        sampleArray[i] = 0;
    }
}

void beginSampling()
{
    //Start the timer counting, interrupt takes over from here
    timer.resume();
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

uint16_t processSamples(uint8_t* samples, uint8_t packetLength, uint8_t samplesPerBit)
{
    uint16_t receivedData = 0;

    uint8_t numSamples = packetLength * samplesPerBit;

    //Process the data

    uint8_t count = 0;

    //"Pointer" to the bit we are currently attempting to detect
    uint8_t currentBit = 0;
    //uint8_t currentBit = packetLength;

    //Prime the main decoding loop.  Because we know that 0 is sent as start bit (high to low transition), sample buffer
    //should start with a string of 1's.
    uint8_t currentSample = 1;
    uint8_t lastSample = 1;

    //Offset of first falling edge to start clocking data from
    uint8_t startSample = 0;

    //Wait for initial falling edge
    for(uint8_t i = 0; currentSample == lastSample && i < numSamples; i++)
    {

        lastSample = currentSample;

        //Look at the current sample and update the offset for first falling edge
        currentSample = sampleArray[i];
        startSample = i;

    }

    //Ok, found the first falling edge - now figure out the data stream
    for (uint8_t i = startSample; currentBit < packetLength && i < numSamples; i++)
    //for (uint8_t i = startSample; currentBit > 0 && i < numSamples; i++)
    {

        lastSample = currentSample;

        currentSample = sampleArray[i];

        if(currentSample != lastSample)
        {
            //Caught an edge.  Need to figure out if it was an actual data transition or just a set-up one.
            if(count < (SAMPLES_PER_HALF_BIT + PHASE_TOLERANCE) )
            {
                //It was not a bit transition - it was a setup for one.  Just keep counting.
                count++;

            } else if(count >= (SAMPLES_PER_BIT - PHASE_TOLERANCE)) {     
                //Add value to data array
                receivedData = receivedData | (currentSample << (packetLength - currentBit - 1) );
                //receivedData = receivedData | (currentSample << currentBit );
                //Serial1.println((currentSample << (packetLength - currentBit) ), BIN);

                //Increment current bit
                currentBit++;

                //Reset count
                count = 0;

            }

        } else {
            //Nothing happened, increment count

            if(count > 2 * SAMPLES_PER_BIT)
            {
                return 0;
            } else {

                count++;

            }

            
        }
        
    }

    //Serial1.println();
    //Serial1.println(receivedData, BIN);

    for(uint8_t i = 0; i < packetLength; i++)
    {
        //Serial1.print(1 & (receivedData >> (packetLength - i)), BIN);
        //SerialUSB.print(1 & (receivedData >> (packetLength - i)), BIN);
    }

    //Serial1.println();
    //SerialUSB.println();

    return receivedData;
}

// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void premain() {
    init();
}



            //Data is now ready, do something with it!
            //SerialUSB.println("Sampling done!");

            //Print out the data
            // for(uint8_t i = 0; i < NUM_SAMPLES; i++)
            // {
            //     SerialUSB.print(sampleArray[i]);
            // }
            
            // SerialUSB.println();