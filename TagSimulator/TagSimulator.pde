void setup() {
  
  // Initialize the built-in LED pin as an output:
  pinMode(BOARD_LED_PIN, OUTPUT);
  // Initialize the built-in button (labeled BUT) as an input:
  pinMode(BOARD_BUTTON_PIN, INPUT);

  Serial1.begin(38400);
  

}

void loop() {
  if(isButtonPressed())
  {
    toggleLED();
    SerialUSB.println(0b0001010101010101, BIN);
    delay(20);
  }
  
  Serial1.println("Hello!");
  delay(20);
}
