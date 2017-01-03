#include <SoftwareSerial.h>

// Pin assignments
// AW stands for (Arduino WiFi)
#define AIN1 3
#define AIN2 4
#define APWM 5
#define BIN1 12
#define BIN2 13
#define BPWM 11
#define STBY 6
#define AWRX 8  // Arduino receives data from ESP8266 on Pin 8
#define AWTX 9  // Arduino transmits data to ESP8266 on Pin 9

// Constants for motor control functions
#define STEPTIME 600 
#define STRAIGHTSPEED 200
#define TURNSPEED 120
#define TURNTIME 300

// Constant to enable or disable WiFi debugging
#define DEBUG true

// Initialize SoftwareSerial communication with ESP8266
SoftwareSerial esp8266(AWRX,AWTX);

// Array to track current PWM values for each channel (A and B)
int pwms[] = {APWM, BPWM};

// Offsets to be used to compensate for one motor being more powerful
byte leftOffset = 0;
byte rightOffset = 0;

// Variable to track remaining time
unsigned long pwmTimeout = 0;

// Function to write out pwm values
void writePwms ( int left, int right) {
    analogWrite (pwms[0], left);
    analogWrite (pwms[1], right);
}

// Move the robot forward for STEPTIME
void goForward() {
    digitalWrite(STBY, HIGH);
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);
    writePwms (STRAIGHTSPEED-leftOffset,STRAIGHTSPEED-rightOffset);
    pwmTimeout = millis() + STEPTIME;
}

// Move the robot backward for STEPTIME
void goBack() {
    digitalWrite(STBY, HIGH);
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
    writePwms (STRAIGHTSPEED-leftOffset,STRAIGHTSPEED-rightOffset);
    pwmTimeout = millis() + STEPTIME;
}

// Turn the robot left for TURNTIME
void goLeft() {
    digitalWrite(STBY, HIGH);
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);

    writePwms (TURNSPEED,TURNSPEED);
    pwmTimeout = millis() + TURNTIME;
}

// Turn the robot right for TURNTIME
void goRight() {
    digitalWrite(STBY, HIGH);
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);

    writePwms (TURNSPEED,TURNSPEED);
    pwmTimeout = millis() + TURNTIME;
}

// Autonomous roaming using the Ultrasonic sensor
void autonomous(String command) {
  if(command == "disable") {
    Serial.print("Disable autonomous roaming.");
  } else if (command == "enable") {
    Serial.print("Enable autonomous roaming.");
  }
}

// Stop the robot (using standby)
void stop() {
    digitalWrite(STBY, LOW); 
}

/*
* Name: sendData
* Description: Function used to send data to ESP8266.
* Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
* Returns: The response from the esp8266 (if there is a reponse)
* Source: http://allaboutee.com/2015/01/02/esp8266-arduino-led-control-from-webpage/
*/
String sendData(String command, const int timeout, boolean debug) {
    String response = "";
    
    esp8266.print(command); // Send the read character to the esp8266
    
    long int time = millis();
    
    while( (time+timeout) > millis()) {
        while(esp8266.available()){
            // The esp has data so display its output to the serial window 
            char c = esp8266.read(); // Read the next character.
            response+=c;
        }  
    }
    
    if(debug) {
    Serial.print("Message from ESP:" + response);
    }
    
    return response;
}

// Arduino setup function
void setup() {
    // Initialize pins as outputs
    pinMode (STBY, OUTPUT);
    pinMode (AIN1, OUTPUT);
    pinMode (AIN2, OUTPUT);
    pinMode (APWM, OUTPUT);
    pinMode (BIN1, OUTPUT);
    pinMode (BIN2, OUTPUT);
    pinMode (BPWM, OUTPUT);
  
  Serial.begin(9600);
  esp8266.begin(9600); // Your esp's baud rate might be different
  
  sendData("AT+RST\r\n",2000,DEBUG); // Reset module
  sendData("AT+CWSAP=\"ThimbleBot\",\"1234567890\",1,0\r\n",1000,DEBUG); // Set softAP configuration
  sendData("AT+CWMODE=2\r\n",1000,DEBUG); // Configure as access point
  sendData("AT+CIPAP_CUR=\"192.168.5.1\"\r\n",1000,DEBUG); // Set ip address
  sendData("AT+CIPMUX=1\r\n",1000,DEBUG); // Configure for multiple connections
  sendData("AT+CIPSERVER=1,80\r\n",1000,DEBUG); // Turn on server on port 80
}

// Loop (code between {}'s repeats over and over again)
void loop() {

  if(esp8266.available()) { // Check if the esp is sending a message
    if(esp8266.find("+IPD,")) {
      delay(1000); // Wait for the serial buffer to fill up (read all the serial data)
     
      // Get the connection id so that we can then disconnect
      int connectionId = esp8266.read()-48; // Subtract 48 because the read() function returns 
                          // the ASCII decimal value and 0 (the first decimal number) starts at 48
          
      esp8266.find("move="); // Advance cursor to "move="
     
      int remoteSelection = esp8266.read()-48; // Get the number for the user selection
      
      switch (remoteSelection) {
        case 1:
          //User has selected the option to move forward
          goForward();
          break;
        case 2:
          //User has selected the option to move backward
          goBack();
          break;
        case 3:
          //User has selected the option to turn left
          goLeft();
          break;
        case 4:
          //User has selected the option to turn right
          goRight();
          break;
        case 5:
          //User has selected the option to turn left
          autonomous("enable");
          break;
        case 6:
          //User has selected the option to turn right
          autonomous("disable");
          break;
      }

      delay(1000); // Wait for one second
      stop(); // Make the robot stop
      
      String closeCommand = "AT+CIPCLOSE="; // Make close command
      closeCommand+=connectionId; // Append connection id
      closeCommand+="\r\n"; // Append proper line ending
     
      sendData(closeCommand,1000,DEBUG); // Close connection
    }
  }
}
