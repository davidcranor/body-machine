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
#include </Users/davidcranor/Documents/libmaple/libraries/EasyTransfer/EasyTransfer.h>

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
#define RED 4

#define DEBUG_PIN 21

uint8_t debugPinState;

MCP2035 Receiver;

//create Easy Transfer object
EasyTransfer ET; 

struct SEND_DATA_STRUCTURE
{
  //put your variable definitions here for the data you want to send
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO

  uint8_t oeh;
  uint8_t oel;
  uint8_t alrtind;
  uint8_t lcxen;

  uint8_t datout;
  uint8_t tunecap;

  uint8_t rssifet;
  uint8_t clkdiv;

  uint8_t sensctl;

  uint8_t agcsig;
  uint8_t modmin;

  uint8_t led;

};


//give a name to the group of data
SEND_DATA_STRUCTURE receiver_settings;




// We'll use timer 2
HardwareTimer timer(2);

uint8_t samplesReady;
uint16_t samplePointer;
uint8_t currentSample;

uint8_t sampleArray[NUM_SAMPLES];
uint16_t data = 0;

void initSampleTimer();
void handler_sampleTime();
void resetSampling();
void beginSampling();

void processCommands();
uint16_t processSamples(uint8_t* samples, uint8_t packetLength, uint8_t samplesPerBit);

uint8_t computeParity(uint16_t data);

void readSettings();
uint8_t stripRegister(uint8_t registerNumber, uint8_t offset, uint8_t mask);




int main()
{
    //For debugging
    //delay(3000);


    pinMode(DEBUG_PIN, OUTPUT);
    digitalWrite(DEBUG_PIN, LOW);
    debugPinState = 0;

    pinMode(RED, OUTPUT);
    pinMode(BLUE, OUTPUT);

    //Set up the timer
    initSampleTimer();

    //Set up the receiver
    Receiver.begin(MCP2035_IO_PIN, MCP2035_CLK_ALERT_PIN, MCP2035_CS_PIN);

    //Serial port for wireless radio
    Serial1.begin(38400);
    //Serial1.println("Hello!");

    //ET.begin(details(guino_data), &Serial1);  //Old version
    //ET.begin( (uint8_t*)&receiver_settings, 255, &Serial1);  //This one works, but buffer is 255 big.
    ET.begin( (uint8_t*)&receiver_settings, sizeof(receiver_settings), &Serial1);

    //Reset timer, flags, and buffers
    resetSampling();

    while(1)
    {
        // digitalWrite(GREEN, HIGH);
        // delay(100);
        // digitalWrite(GREEN, LOW);
        // delay(100);

        //First, check the serial port for commands
        while(Serial1.available())
        {
            //uint8_t poop = Serial1.read();
            //Serial1.println(poop, HEX);

            //ET.receiveData();

            if(ET.receiveData())
            {
                //Serial1.println("Hello!");
                processCommands();
            }
        }
        
        //Then, wait for data pin to go high (first incoming bit must be a 0)
        if(digitalRead(MCP2035_IO_PIN))
        {
            //digitalWrite(RED, HIGH);

            //Begin sampling, interrupt takes over from here
            beginSampling();

            //Wait for interrupt to indicate sampling is done
            while(samplesReady == 0)
            {
                delay(1);
            }

            // for(int i = 0; i < NUM_SAMPLES; i++)
            // {
            //     Serial1.print(sampleArray[i], BIN);
            // }
            // Serial1.println();

            data = 0;

            data = processSamples(sampleArray, PACKET_LENGTH, SAMPLES_PER_BIT);

            Serial1.println(data);
            SerialUSB.println(data);

            resetSampling();

            //digitalWrite(RED, LOW);
        }
    }
}

void readSettings()
{
    receiver_settings.oeh = stripRegister(CONFIG_REGISTER_0, OUTPUT_ENABLE_FILTER_HIGH_BITS_OFFSET, OUTPUT_ENABLE_FILTER_HIGH_BITS_MASK);
    receiver_settings.oel = stripRegister(CONFIG_REGISTER_0, OUTPUT_ENABLE_FILTER_LOW_BITS_OFFSET, OUTPUT_ENABLE_FILTER_LOW_BITS_MASK);
    receiver_settings.alrtind = stripRegister(CONFIG_REGISTER_0, ALERT_TRIGGER_BITS_OFFSET, ALERT_TRIGGER_BITS_MASK);
    receiver_settings.lcxen = stripRegister(CONFIG_REGISTER_0, INPUT_CHANNEL_SETTING_BITS_OFFSET, INPUT_CHANNEL_SETTING_BITS_MASK);

    receiver_settings.datout = stripRegister(CONFIG_REGISTER_1, LFDATA_OUTPUT_BITS_OFFSET, LFDATA_OUTPUT_BITS_MASK);
    receiver_settings.tunecap = stripRegister(CONFIG_REGISTER_1, TUNING_CAP_BITS_OFFSET, TUNING_CAP_BITS_MASK);

    receiver_settings.rssifet = stripRegister(CONFIG_REGISTER_2, RSSI_PULL_DOWN_SETTING_OFFSET, RSSI_PULL_DOWN_SETTING_MASK);
    receiver_settings.clkdiv = stripRegister(CONFIG_REGISTER_2, CARRIER_CLOCK_DIVIDE_BITS_OFFSET, CARRIER_CLOCK_DIVIDE_BITS_MASK);

    receiver_settings.sensctl = stripRegister(CONFIG_REGISTER_4, INPUT_SENSITIVITY_REDUCTION_BITS_OFFSET, INPUT_SENSITIVITY_REDUCTION_BITS_MASK);

    receiver_settings.agcsig = stripRegister(CONFIG_REGISTER_5, DEMOD_OUTPUT_AGC_DEPENDENT_BITS_OFFSET, DEMOD_OUTPUT_AGC_DEPENDENT_BITS_MASK);
    receiver_settings.modmin = stripRegister(CONFIG_REGISTER_5, MIN_MOD_DEPTH_BITS_OFFSET, MIN_MOD_DEPTH_BITS_MASK);

    receiver_settings.led = digitalRead(BLUE);
}

uint8_t stripRegister(uint8_t registerNumber, uint8_t offset, uint8_t mask)
{
    uint8_t result = 0;
    uint8_t rawRegister = 0;

    rawRegister = Receiver.readRegister(registerNumber);

    result = (rawRegister & (mask ^ 0xFF) ) >> offset;

    return result;
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

    currentSample = digitalRead(MCP2035_IO_PIN);
    
    //Check to see if we are at the end
    if(samplePointer < NUM_SAMPLES)
    {
        //If not, take the sample and increment the pointer
        sampleArray[samplePointer] = digitalRead(MCP2035_IO_PIN);
        samplePointer++;

    } else {
        //Otherwise, end sampling and set the done flag
        //Stop the timer (and interrupt)
        timer.pause();

        debugPinState = 0;
        digitalWrite(DEBUG_PIN, debugPinState);

        //Done!
        samplesReady = 1;
    }
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

void processCommands()
{
    //ARE THESE SET UP CORRECTLY??


    // while(1)
    // {

    // Serial1.print("OEH: ");
    // Serial1.println(receiver_settings.oeh);

    // Serial1.print("OEL: ");
    // Serial1.println(receiver_settings.oel);

    // Serial1.print("Alert trigger: ");
    // Serial1.println(receiver_settings.alrtind);

    // Serial1.print("Input Channel: ");
    // Serial1.println(receiver_settings.lcxen);

    // Serial1.print("LFDATA output type: ");
    // Serial1.println(receiver_settings.datout);

    // Serial1.print("Tuning cap: ");
    // Serial1.println(receiver_settings.tunecap);

    // Serial1.print("RSSI fet status: ");
    // Serial1.println(receiver_settings.rssifet);

    // Serial1.print("Carrier clock divide: ");
    // Serial1.println(receiver_settings.clkdiv);

    // Serial1.print("Sensitivity reduction: ");
    // Serial1.println(receiver_settings.sensctl);

    // Serial1.print("Demodulator output dependent: ");
    // Serial1.println(receiver_settings.agcsig);

    // Serial1.print("Min modulation depth: ");
    // Serial1.println(receiver_settings.modmin);

    // Serial1.print("LED status: ");
    // Serial1.println(receiver_settings.led);

    // digitalWrite(BLUE, receiver_settings.led);

    // }



    //Reset and clear baseband registers
    Receiver.softReset();
    Receiver.clearRegisters();

    Receiver.setOutputEnableHighTime(receiver_settings.oeh);
    Receiver.setOutputEnableLowTime(receiver_settings.oel);
    Receiver.setAlertTrigger(receiver_settings.alrtind);
    Receiver.setInputChannel(receiver_settings.lcxen);
    Receiver.setLFDATAOutputType(receiver_settings.datout);
    Receiver.setTuningCap(receiver_settings.tunecap);
    Receiver.setRSSIMosfet(receiver_settings.rssifet);
    Receiver.setCarrierClockDivide(receiver_settings.clkdiv);
    Receiver.setSensitivityReduction(receiver_settings.sensctl);
    Receiver.setDemodulatorOutput(receiver_settings.agcsig);
    Receiver.setMinimumModulationDepth(receiver_settings.modmin);

    digitalWrite(BLUE, receiver_settings.led);

    Receiver.updateColumnParity();

    readSettings();

    ET.sendData();

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
            count++;
        }
        
    }

    //Serial1.println();
    //Serial1.println(receivedData, BIN);

    for(uint8_t i = 0; i < packetLength; i++)
    {
        //Serial1.print(1 & (receivedData >> (packetLength - i)), BIN);
        ////SerialUSB.print(1 & (receivedData >> (packetLength - i)), BIN);
    }

   // Serial1.println(receivedData);
    ////SerialUSB.println();

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