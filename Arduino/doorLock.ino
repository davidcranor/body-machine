/*
Servo control for door lock/unlock

scottgwald
*/

#include <SoftwareSerial.h>
#include <Servo.h> 

Servo myservo;  
SoftwareSerial rfid(7, 8);

char incomingByte;

void setup()  
{
  Serial.begin(9600);
  Serial.println("Start");
  myservo.attach(9);
}

void loop()
{
  //Serial.println("Loop " + String(loopInc));
  if (Serial.available() > 0) {
        // read the incoming byte:
        incomingByte = Serial.read();
        if (incomingByte == 'u') {
          Serial.println("Unlocking.");
          myservo.write(170);
          delay(1000);
        } else if (incomingByte == 'l') {
          Serial.println("Locking.");          
          myservo.write(70);
          delay(1000);  
        } else {
          Serial.println("I don't know this " + String(incomingByte) + " of which you speak.");
        }
  }
}

void halt()
{
  myservo.detach();
}
