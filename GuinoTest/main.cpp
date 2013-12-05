/*
  GUINO DASHBOARD TEMPLATE FOR THE ARDUINO. 
 Done by Mads Hobye as a part of Instructables (AIR Program) & Medea (PhD Student).
 Licens: Creative Commons — Attribution-ShareAlike
 
 It should be used with the GUINO Dashboard app.
 
 More info can be found here: www.hobye.dk
 
 Connect potmeter to analog in, ground and plus 5v.
 Connect diode to pin 11 and ground.
 
 # This is your main template to edit.
 */


#include <wirish/wirish.h>
#include "Guino_library.h"
// Dummy example variables


//int16_t width = 0;

int16_t potMeter = 0;



// This is where you setup your interface 
void gInit()
{
  gAddMovingGraph("POTMETER",0,5000, &potMeter, 30);
  gAddSlider(0,5000,"POTMETER",&potMeter);

}

// Method called everytime a button has been pressed in the interface.
void gButtonPressed(int16_t id)
{
 
}

void gItemUpdated(int16_t id)
{

}



int main()
{

delay(1000);

  //pinMode(BOARD_LED_PIN, PWM);
  //analogWrite(BOARD_LED_PIN, 0);

  //pinMode(11, INPUT_ANALOG);
  pinMode(11, INPUT);


  // Start the guino dashboard interface.
  // The number is your personal key for saving data. This should be unique for each sketch
  // This key should also be changed if you change the gui structure. Hence the saved data vill not match.
  gBegin(34236);

//SerialUSB.println("Init Finished!");

	while(1)
	{
		  // **** Main update call for the guino
		guino_update();

		  // Read analog
		// int poop = digitalRead(21);
		// SerialUSB.println(poop);
		// 
		// if (poop == HIGH)
		// {
		// 	potMeter = 4000;
		// } else {
		// 	potMeter = 0;
		// }
		//potMeter = analogRead(11);
		potMeter = digitalRead(11);
		//potMeter++;
		  


		  // Attach a led to pin 11 to control it.
		  //analogWrite(BOARD_LED_PIN,graphValue);

		  // Send the value to the gui.
		  gUpdateValue(&potMeter);
		  //gUpdateValue(&graphValue);
		 // gUpdateValue(&angle);
		
		  //SerialUSB.println("Loop");
		  delay(20); //cranor added this
	}
}


// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void premain() {
    init();
}