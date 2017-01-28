#include "Wire.h"
#define DS3231_I2C_ADDRESS 0x68
#define DATA_PIN 2
#define CLK_PIN 4
#define STR_PIN 7
#define BRIGHTNESS_OE_PIN 9
#define BRIGHTNESS_IN_PIN 0 // analog in pin for pot
#define TIME_SETTING_MODE_PIN 10 
#define HOUR_SET_PIN 12
#define MINUTE_SET_PIN 8

#define LED_INTERVAL 1000 //in microseconds


word ledData;
int currentPositiveLead;
unsigned long currentMillis;
unsigned long lastMillis;
unsigned long lastLight;
unsigned long currentLight;
byte firstRowData[2];
byte secondRowData[2];
byte thirdRowData[2];
bool firstRun;
byte minute, hour;

// time setting variables
int timeSettingState;
unsigned long hourDebounceTime, minuteDebounceTime;
bool hourState, minuteState, lastHourState, lastMinuteState;
bool prevTimeSetState;
/*
byte alarmHour, alarmMinute;
bool alarmSet;
bool alarmCleared;
*/
// used to ensure rows are only set once per cycle
bool firstRowSet;
bool secondRowSet;
bool thirdRowSet;


// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void setup()
{
  Wire.begin();
  Serial.begin(9600);
  
  // set up pins
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLK_PIN, OUTPUT);
  pinMode(STR_PIN, OUTPUT);
  pinMode(BRIGHTNESS_OE_PIN, OUTPUT);
  pinMode(TIME_SETTING_MODE_PIN, INPUT_PULLUP);
  pinMode(HOUR_SET_PIN, INPUT_PULLUP);
  pinMode(MINUTE_SET_PIN, INPUT_PULLUP);
  
  // lastMillis and currentMillis for grabbing the time from the RTC
  lastMillis = millis();
  currentMillis = lastMillis;
  // lastLight and currentLight for turning on the LEDs in grid formation at set intervals
  lastLight = micros();
  currentLight = lastLight;
  firstRun = true;
  
  //setup for time setting
  timeSettingState = 0;
  lastHourState = true; // button not pressed
  lastMinuteState = true; // button not pressed
  prevTimeSetState = false;
  
  // temp for testing the alarm
  /*
  alarmHour = 10;
  alarmMinute = 02;
  alarmSet = true;
  alarmCleared = true;
  */
  
  // FOR TESTING
  /*
  ledData = 0x0004; //TEMP - prepping to turn on first light
  currentPositiveLead = 1;
  */
  
  TCCR1B = TCCR1B & 0b11111000 | 0x01; // sets PWM of pin 9 and 10 
                                       // to 31250/1 = 31250 Hz
  
  // set the initial time here:
  // DS3231 seconds, minutes, hours, day, date, month, year
  // setDS3231time(0,3,13,7,20,8,16);
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}

// simpler function for only setting hour and minute
void setHourMinuteTime(byte minute, byte hour)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(1); // set input to minutes register
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.endTransmission();
}

void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

// more efficient version of readDS3231time, only minutes and hours for word clock
void readTimeHoursMinutes(byte *minute, byte *hour)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(1); // set DS3231 register pointer to 01h (for minutes)
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 2);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
}

void displayTime()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
  &year);
  // send it to the serial monitor
  Serial.print(hour, DEC);
  // convert the byte variable to a decimal number when displayed
  Serial.print(":");
  if (minute<10)
  {
    Serial.print("0");
  }
  Serial.print(minute, DEC);
  Serial.print(":");
  if (second<10)
  {
    Serial.print("0");
  }
  Serial.print(second, DEC);
  Serial.print(" ");
  Serial.print(dayOfMonth, DEC);
  Serial.print("/");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(year, DEC);
  Serial.print(" Day of week: ");
  switch(dayOfWeek){
  case 1:
    Serial.println("Sunday");
    break;
  case 2:
    Serial.println("Monday");
    break;
  case 3:
    Serial.println("Tuesday");
    break;
  case 4:
    Serial.println("Wednesday");
    break;
  case 5:
    Serial.println("Thursday");
    break;
  case 6:
    Serial.println("Friday");
    break;
  case 7:
    Serial.println("Saturday");
    break;
  }
}

void setLED(int numBytes, byte data[])
{
  digitalWrite(STR_PIN, 0); // don't set register pins until data has been shifted
  
  for (int i = 0; i < numBytes; i++)
  {
    shiftOut(DATA_PIN, CLK_PIN, LSBFIRST, data[i]);
  }
  
  digitalWrite(STR_PIN, 1);
}

void setLEDBytes(byte row1[], byte row2[], byte row3[], byte minute, byte hour)
{
  row1[0] = 0;
  row1[1] = 0;
  row2[0] = 0;
  row2[1] = 0;
  row3[0] = 0;
  row3[1] = 0;
  
  // row 1 settings
  // ---------------------------------------------
  if (hour > 4 && hour < 12) row1[0] = row1[0] | 0x10; // morning
  else if (hour >= 12 && hour < 18) row1[0] = row1[0] | 0x80; // afternoon
  else if (hour >= 18 && hour < 21) row1[1] = row1[1] | 0x04; // evening
  else row1[0] = row1[0] | 0x20; // night
  
  if (minute >= 28 && minute < 33) row1[1] = row1[1] | 0x01; // half
  else if ((minute >= 8 && minute < 13) || (minute >= 48 && minute < 53))
    row1[1] = row1[1] | 0x08; // ten (for minutes)
  else if ((minute >= 13 && minute < 18) || (minute >= 43 && minute < 48)) 
    row1[1] = row1[1] | 0x02; // quarter
  else if ((minute >= 18 && minute < 28) || (minute >= 33 && minute < 43))
    row1[1] = row1[1] | 0x20; // twenty    

  // row 2/3 settings
  // ---------------------------------------------
  if ((minute >= 3 && minute < 8) || (minute >= 23 && minute < 28) ||
    (minute >= 33 && minute < 38) || (minute >= 53 && minute < 58))
    row2[0] = row2[0] | 0x08; // five (for minutes)
 
  if ((minute >= 3 && minute < 13) || (minute >= 18 && minute < 28) ||
    (minute >= 33 && minute <  43) || (minute >= 48 && minute < 58))
    row2[0] = row2[0] | 0x10; // minutes
  if (minute >= 33 && minute < 58) row2[0] = row2[0] | 0x020; // to
  if (minute >= 3 && minute < 33) row2[1] = row2[1] | 0x04; // past

  if (minute >= 33)
  {
    if (hour == 0 || hour == 12) row2[1] = row2[1] | 0x10; // one
    else if (hour == 1 || hour == 13) row2[0] = row2[0] | 0x80; // two
    else if (hour == 2 || hour == 14) row2[1] = row2[1] | 0x01; // three
    else if (hour == 3 || hour == 15) row2[1] = row2[1] | 0x08; // four
    else if (hour == 4 || hour == 16) row2[1] = row2[1] | 0x02; // five
    else if (hour == 5 || hour == 17) row2[1] = row2[1] | 0x20; // six
    else if (hour == 6 || hour == 18) row3[0] = row3[0] | 0x08; // seven
    else if (hour == 7 || hour == 19) row3[0] = row3[0] | 0x10; // eight
    else if (hour == 8 || hour == 20) row3[0] = row3[0] | 0x20; // nine
    else if (hour == 9 || hour == 21) row3[1] = row3[1] | 0x10; // ten
    else if (hour == 10 || hour == 22) row3[0] = row3[0] | 0x80; // eleven
    else if (hour == 11 || hour == 23) row3[1] = row3[1] | 0x01; // twelve
  }
  else 
  {
    if (hour == 1 || hour == 13) row2[1] = row2[1] | 0x10; // one
    else if (hour == 2 || hour == 14) row2[0] = row2[0] | 0x80; // two
    else if (hour == 3 || hour == 15) row2[1] = row2[1] | 0x01; // three
    else if (hour == 4 || hour == 16) row2[1] = row2[1] | 0x08; // four
    else if (hour == 5 || hour == 17) row2[1] = row2[1] | 0x02; // five
    else if (hour == 6 || hour == 18) row2[1] = row2[1] | 0x20; // six
    else if (hour == 7 || hour == 19) row3[0] = row3[0] | 0x08; // seven
    else if (hour == 8 || hour == 20) row3[0] = row3[0] | 0x10; // eight
    else if (hour == 9 || hour == 21) row3[0] = row3[0] | 0x20; // nine
    else if (hour == 10 || hour == 22) row3[1] = row3[1] | 0x10; // ten
    else if (hour == 11 || hour == 23) row3[0] = row3[0] | 0x80; // eleven
    else if (hour == 12 || hour == 0) row3[1] = row3[1] | 0x01; // twelve
  }

  // row 3 settings
  // --------------------------------------------- 
  
  if ((hour == 22 && minute > 30) || (hour >= 23) || (hour < 6))
  {
    row3[1] = row3[1] | 0x28; // to sleep, go
    row3[0] = row3[0] | 0x40; // the fuck
  }
  
  if (hour > 6 && hour < 11)
  {
    row3[1] = row3[1] | 0x06; // up, wake
    row3[0] = row3[0] | 0x40; // the fuck
  }
  
  // could add this in if want to add in an alarm
  /*
  if (alarmSet || (hour > 11 && hour < 6))
  {
    row3[1] = row3[1] | 0x28; // to sleep, go
    row3[0] = row3[0] | 0x40; // the fuck
  }
  
  if (alarmSet && (hour == alarmHour && minute == alarmMinute))
    alarmCleared = false;
  
  if ((hour == alarmHour && minute == alarmMinute) || !alarmCleared)
  {
    row3[1] = row3[1] | 0x06; // up, wake
    row3[0] = row3[0] | 0x40; // the fuck
    alarmSet = false;
  }
  */
  
  // always on lights AND positive leads
  // ------------------------------------------------------
  row1[0] = row1[0] | 0x09; // Good, first positive lead
  row1[1] = row1[1] | 0x10; // It is
  row2[0] = row2[0] | 0x02; // second positive lead
  row3[0] = row3[0] | 0x04; // third positive lead
  
}

void loop()
{
  currentMillis = millis();
  // when time setting pin is high (internally pulled up), time set mode is on
  bool timeSettingPinState = digitalRead(TIME_SETTING_MODE_PIN);
  // ALARM STUFF
  //tone(11, 1000, 500);
  
  // BRIGHTNESS
  // ------------------------------------------------------------------------
  int brightness = analogRead(BRIGHTNESS_IN_PIN);
  brightness = brightness/4 - 25;
  if (brightness < 0) brightness = 0;
  // only set the brightness pin if time setting mode isn't on
  if (!timeSettingPinState) analogWrite(BRIGHTNESS_OE_PIN, brightness);
  //analogWrite(BRIGHTNESS_OE_PIN, 0); //temporarily make brightness max
  // ------------------------------------------------------------------------
  
  // TIME SETTING
  // ------------------------------------------------------------------------
  if (timeSettingPinState) {
    prevTimeSetState = true;
    if (timeSettingState == 0) {
      lastMillis = currentMillis;
      timeSettingState++;
    }
    
    // blink the text so that user knows time is being set
    unsigned long timeDiff = currentMillis - lastMillis;
    if (timeDiff < 500) {
      analogWrite(BRIGHTNESS_OE_PIN, brightness);
    }
    else if (timeDiff >= 500 && timeDiff < 1000) {
      analogWrite(BRIGHTNESS_OE_PIN, 255); //lights off, creates blinking effect
    }
    else {
//      Serial.println(timeDiff);
      lastMillis = currentMillis;
    }
    
    // every time hour button is pressed, increase hour
    // buttons are FALSE when pressed
    // debounce hour and minute buttons
    bool hourReading = digitalRead(HOUR_SET_PIN);
    bool minuteReading = digitalRead(MINUTE_SET_PIN);
    
    //debounce hour button
    if (hourReading != lastHourState) {
      hourDebounceTime = millis();
    }
    if ((millis() - hourDebounceTime) > 50) {
      if (hourReading != hourState) {
        hourState = hourReading;
        if (!hourState) {
          if (hour < 23) hour++;
          else hour = 0;
          setLEDBytes(firstRowData, secondRowData, thirdRowData, minute, hour);
        }
      }
    }
    
    // debounce minute button
    // right now minutes must be set on five minute increments..consider adding
    // ability to set minutes by precise minutes
    if (minuteReading != lastMinuteState) {
      minuteDebounceTime = millis();
    }
    if ((millis() - minuteDebounceTime) > 50) {
      if (minuteReading != minuteState) {
        minuteState = minuteReading;
        if (!minuteState) {
          if (minute < 55) {
            int minuteMod = minute % 5;
            if (minuteMod == 0) minute = minute + 5;
            else minute = minute + minuteMod;
          }
          else minute = 0;
          setLEDBytes(firstRowData, secondRowData, thirdRowData, minute, hour);
        }
      }
    }
    
    lastHourState = hourReading;
    lastMinuteState = minuteReading;
    
  }
  else {
    timeSettingState = 0;
    // this should only be true immediately after the switch is turned off
    if (prevTimeSetState) {
      prevTimeSetState = false;
      setHourMinuteTime(minute, hour);
    }
  }
  
  // ------------------------------------------------------------------------
  
  
  // TIME PROCESSING EVERY 5 SECONDS
  // ------------------------------------------------------------------------
  // as long as the checking time is over 5000, won't interfere with time setting
  if ((abs(currentMillis - lastMillis) > 5000) || firstRun)
  {
    // retrieve data from DS3231
    readTimeHoursMinutes(&minute, &hour);
    lastMillis = currentMillis;
    // set the bytes to be used for setting LEDs
    setLEDBytes(firstRowData, secondRowData, thirdRowData, minute, hour);
    firstRun = false;
    
   // displayTime();
  }
  // ------------------------------------------------------------------------
   
  /* // for testing every light. change currentPositiveLead to 1, 2, or 4
  byte data[2];
  ledData = ledData << 1; // move to the next negative lead
  ledData = ledData + currentPositiveLead; // turn on the current positive lead
  data[1] = (unsigned int) ledData >> 8; // top end of ledData stored in lower byte
  data[0] = ledData & 0x00FF;
  
  setLED(2, data);
  ledData = ledData - currentPositiveLead;
  if (ledData == 32768) ledData = 0x0004;
  */
  
  currentLight = micros();
  
  // LIGHT SETTING
  // ------------------------------------------------------------------------
  // set the first row's LEDs on
  if ((currentLight - lastLight) > LED_INTERVAL && !firstRowSet) {
    setLED(2, firstRowData);
    firstRowSet = true;
  }
  // set the 2nd row's LEDs on
  else if ((currentLight - lastLight) > LED_INTERVAL * 2 && !secondRowSet) {
    setLED(2, secondRowData);
    secondRowSet = true;
  }
  // set the 3rd row's LEDs on
  else if ((currentLight - lastLight) > LED_INTERVAL * 3 && !thirdRowSet) {
    setLED(2, thirdRowData);
    firstRowSet = false;
    secondRowSet = false;
    lastLight = currentLight;
  }
  // this should occur during the micros() overflow, once every 70 minutes
  else if (currentLight < lastLight) {
    firstRowSet = false;
    secondRowSet = false;
    lastLight = currentLight;
  }
  // ------------------------------------------------------------------------
}
