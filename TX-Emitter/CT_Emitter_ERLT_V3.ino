/*
  This Code is published by Yannik Wertz under the "Creativ Common Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)" License
  For more Information see : http://creativecommons.org/licenses/by-nc-sa/4.0/
*/

/*#######################################################
  #########################################################
  OpenLap
  Open Source IR Laptimer
  For more infos see :
  www.openlap.de
  https://github.com/YannikW/Open-Lap
  https://www.facebook.com/groups/1047398441948165/
  IR Transponder with fixed ID V0.6 (22.12.15) (for Attiny 45 or 85 with internal 8Mhz clock)
  #########################################################
  #######################################################*/

/*#######################################################
  ##                      Libraries                      ##
  #######################################################*/
#include <EEPROM.h>

/*#######################################################
  ##                       Defines                       ##
  #######################################################*/

#define ID             1  //Put in your wanted ID here! Between 1 and 63.

#define ZERO          250  //µS
#define ONE           650  //µS

#define SIGNALSPACE    5  //mS (+ 0-10ms Random Space for colision avoidance)
#define RANDOMSPACE    3  //ms (Care : 5 is exclusive)
#define DATABITS        6  //6 Databits -> 0-63
#define MAXID          63  //2^6 - 1 = 63

#define LEDPIN          4  //LED on PB1 (Pin 6 on SOIC package)
#define BUTTONPIN          3  //BUTTON on PB0 (Pin 5 on SOIC package)

#define LONGPRESS      20 //Long button press duration in units of 50ms (20 = 1000ms)

#define ledOn() digitalWrite(LEDPIN, HIGH)
#define ledOff() digitalWrite(LEDPIN, LOW)



/*#######################################################
  ##                      Variables                      ##
  #######################################################*/

uint8_t id = ID;  //Do not change this one - change the "ID" above under Defines.


/*#######################################################
  ##                        Setup                        ##
  #######################################################*/

void setup()
{
  pinMode(LEDPIN, OUTPUT);  //Set Pin A6 as output

  pinMode(BUTTONPIN, INPUT_PULLUP);
  digitalWrite(BUTTONPIN, HIGH);

  ledOff();
  delay(500);
  id = EEPROM.read(0);
  if(id>MAXID) id = ID;                              //When you flash the device, memory defaults to id 255 which is invalid.
  
  if (!digitalRead(BUTTONPIN)) {                    //If button pressed on boot, go into config mode
    ledOn();
    delay(500);
    ledOff();
    delay(2000);
    while (!digitalRead(BUTTONPIN)) {               //Wait for button to be released
      delay(10);
    }
    
    bool interrupted = flashID();                   //Flash current ID on boot
    while(true)
    {
      while(interrupted){
        int cnt = timeButton();
        if(cnt>0) interrupted = pressed(cnt);       //Handles changing ID and displays value, can be interrupted
        else {
          ledOff();
          break;
        }
      }
      delay(10);
          
      if (!digitalRead(BUTTONPIN)) {
        delay(50);
        if (!digitalRead(BUTTONPIN)) {  //Make sure its not just a random blip
          interrupted = true;
        }
      }
    }
  }
  
  //clear Timer1
  TCCR1 = 0;
  GTCCR = 0;

  //set timer1 on 76kHz (2*38) toggle Pin 6
  GTCCR |= (1 << COM1B0);     //Toggle OC1A on compare match
  TCCR1 |= (1 << CTC1);       //CTC Mode

  TCCR1 |= (1 << CS10);       //No Prescaler

  OCR1C = 104;                //(8000000 / 76000) - 1

  randomSeed(analogRead(1)); //Generate Random Seed.
  
  ledOn();
}

/*#######################################################
  ##                        Loop                         ##
  #######################################################*/

void loop()
{
  sendOut();                  //Generate signal
  delay(SIGNALSPACE);         //Wait some static time
  delay(random(RANDOMSPACE)); //Wait another random time for collision avoidance
}

/*#######################################################
  ##                      Functions                      ##
  #######################################################*/

void enableSignal()
{
  TCCR1 |= (1 << CS10);    //Enable Timer
  GTCCR |= (1 << COM1B0);  //Enable toggle OC1A on compare match
  //PORTB |= (1 << LEDPIN);  //Set Led to low
}

void disableSignal()
{
  TCCR1 &= ~(1 << CS10);    //Disable Timer
  GTCCR &= ~(1 << COM1B0);  //Get access for manualy set OC1A to low
  PORTB &= ~(1 << LEDPIN);  //Set Led to low
}

void sendOut()
{
  boolean checksum = 0;

  //Start Bits
  enableSignal();           //Startbit 1
  delayMicroseconds(ZERO);

  disableSignal();          //Startbit 2
  delayMicroseconds(ZERO);

  //Data bits
  for (int8_t dataCount = (DATABITS - 1) ; dataCount >= 0 ; dataCount--)
  {
    if (dataCount % 2) //Bit 5, 3 and 1
    {
      enableSignal();
    }
    else            //Bit 4, 2 and 0
    {
      disableSignal();
    }
    if ((id >> dataCount) % 2) //Bit is "1"
    {
      delayMicroseconds(ONE);
      checksum = !checksum;  //Toggle Checksum if a "1" is send
    }
    else  //Bit is "0"
      delayMicroseconds(ZERO);
  }


  enableSignal();           //Checksum ("0" if number of 1s is even or "1" if it's odd)
  if (checksum)
    delayMicroseconds(ONE);
  else
    delayMicroseconds(ZERO);

  disableSignal();          //Complete last pulse
}

int timeButton(){ //Times button press, returns number of 50ms intervals, first 50ms are done seperately
  int cnt = 0;
  ledOff();
  do {         //Measure how long button is pressed
    if(cnt<200) cnt++;                      //Max time is 10 seconds
    if(cnt==LONGPRESS) ledOn(); //Turn on LED after 2 seconds
    if(cnt==100) ledOff(); //Turn off LED after 5 seconds
    delay(50);                              //50ms intervals
  }while (!digitalRead(BUTTONPIN));
  ledOff();
  delay(50);                                //For debounce
  return cnt;
}

bool delayInterruptible(int ms){  //Delay for multiple of 10 ms, return's true if interrupted by button press
  for(int j=0;j<ms/10;j++){
    delay(10);
    if (!digitalRead(BUTTONPIN)){
      delay(50);
      if (!digitalRead(BUTTONPIN)) {  //Make sure its not just a random blip
        return true;
      }
    }
  }
  return false;
}

bool pressed(int cnt) {//Returns true if interrupted
  if(cnt>=100){                 //5s button press resets back to 1
    id = 1;
  }
  else if(cnt>=LONGPRESS){             //2s button press adds 10
    id = id + 10;
    if(id>63) id = id % 10;     //ie. 67 wraps back to 7
    if(id<=0) id = 10;          //60 wraps back to 10
  }
  else{                         //Short button press goes up 1
    id = id + 1;
    if(id<=0) id = 1;
    if(id>63) id = 1;
  }
  EEPROM.write(0, id);  //Save ID to EEPROM (first Byte)
  delay(500);
  return flashID();
}

bool flashID()  //Flashes current ID with long and short flashes, returns true if interrupted by button press
{
  int disp = id/10;
  for (int i = 0; i < disp; i++) {            //Flash 10's digit with long flashes
    if(delayInterruptible(500))return true;
    ledOn();
    if(delayInterruptible(1000))return true;
    ledOff();
  }
  delay(500);
   disp = id%10;
  for (int i = 0; i < disp; i++) {            //Flash 1's digit with short flashes
    if(delayInterruptible(600))return true;
    ledOn();
    if(delayInterruptible(150))return true;
    ledOff();
  }
  return false;
}

