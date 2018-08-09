#include <LiquidCrystal.h>
#include <LCDMenu.h>
#include <LCDMenuItem.h>

LiquidCrystal* LCD;  //LCDMenu class requires LiquidCrystal to work, but can be easily modified to work with other libraries
LCDMenu* menu;       
int lastButton = 0;  // used in loop() to debounce reading from LCD shield's keypad (in my case Arduino LCD KeyPad Shield SKU: DFR0009)

int currentPin   = 1;
int votimeterPin = 2;
double kilos = 0;
int peakPower = 0;

unsigned long startMillis;
unsigned long endMillis;
unsigned long autoSelect;
/*  
  This section of #define's as well as function read_LCD_buttons is copied (with one small change) from example
  available here: http://www.dfrobot.com/wiki/index.php?title=Arduino_LCD_KeyPad_Shield_(SKU:_DFR0009)
*/

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

double getRMSCurrent() {
  int cycle = 0;
  int current = 0;
  int maxCurrent = 0;
  int minCurrent = 1000;

  for (int i = 0 ; i <= 200 ; i++) //Monitors and logs the current input for 200 cycles to determine max and min current
  {
    current = analogRead(currentPin);    //Reads current input and records maximum and minimum current

    if (current >= maxCurrent)
      maxCurrent = current;
    else if (current <= minCurrent)
      minCurrent = current;
  }

  if (maxCurrent <= 512)
  {
    maxCurrent = 511;
  }

  double rmsCurrent = (((maxCurrent - 511) * 0.707) / 3.1337); //Calculates RMS current based on maximum value

  Serial.print(" current ") ; 
  Serial.print(rmsCurrent)  ; 
  Serial.println();
  
  return rmsCurrent;
}

long getVoltage() {
  long avgVoltage = 0;

  for (int j = 0 ; j <= 200 ; j++) //Monitors and logs the voltage input for 200 cycles to determine avg voltage
  {
    avgVoltage = avgVoltage + analogRead(votimeterPin);
  }
  avgVoltage = avgVoltage / 200;


  long finalVoltage = (avgVoltage *.4296); // converts analog value(volts) into input ac supply value using this formula

  Serial.print(" voltage ") ; 
  Serial.print(finalVoltage)  ; 
  Serial.println();
  
  return finalVoltage;

}

int getRMSPower(long finalVoltage, double RMSCurrent) {
  int RMSPower = finalVoltage * RMSCurrent;
  if (RMSPower > peakPower)
  {
    peakPower = RMSPower;
  }

  Serial.print(" power ") ; 
  Serial.print(RMSPower)  ; 
  Serial.println();

  return RMSPower;
}

int read_LCD_buttons()
{
  int adc_key_in = analogRead(0);      // read the value from the sensor 
  // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
  // we add approx 50 to those values and check to see if we are close
  if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
  if (adc_key_in < 50)   return btnRIGHT;  
  if (adc_key_in < 100)  return btnUP; 
  if (adc_key_in < 300)  return btnDOWN; 
  if (adc_key_in < 450)  return btnLEFT; 
  if (adc_key_in < 700)  return btnSELECT;   
  return btnNONE;  // when all others fail, return this...
  
}

void displayData(long finalVoltage, double RMSCurrent, int RMSPower, double kilos) {
  delay (2000);
  menu->getLCD()->clear();
  menu->getLCD()->setCursor(0,0);
  menu->getLCD()->print(RMSCurrent);
  menu->getLCD()->print("A");
  menu->getLCD()->setCursor(10, 0);
  menu->getLCD()->print(RMSPower);
  menu->getLCD()->print("W");
  menu->getLCD()->setCursor(0, 1);
  menu->getLCD()->print(kilos);
  menu->getLCD()->print("kWh");
  menu->getLCD()->setCursor(10, 1);
  menu->getLCD()->print(finalVoltage);
  menu->getLCD()->print("V");
}

void showInfoAction()  //this function will be executed if you choose "Settings option".
{
  menu->getLCD()->clear();

  while(true) {
    long finalVoltage = getVoltage();
    double RMSCurrent = getRMSCurrent();
    int RMSPower = getRMSPower(finalVoltage, RMSCurrent);
  
    endMillis = millis();
    unsigned long time = endMillis - startMillis;
    kilos = kilos + ((double)RMSPower * ((double)time / 60 / 60 / 1000000)); //Calculate kilowatt hours used
    startMillis = millis();

    displayData(finalVoltage, RMSCurrent, RMSPower, kilos);

    int buttonState = read_LCD_buttons();

    if (buttonState == btnSELECT) {
       menu->display();
       lastButton = buttonState;
       break;  
    }
  }
}

void resetAction()  //this function will be executed if you choose "Settings option".
{
  kilos = 0;
  peakPower = 0;
  menu->getLCD()->clear();
  menu->getLCD()->setCursor(0,0);
  menu->getLCD()->print("reseting...");
  delay (2000);
}

void setup()
{
  Serial.begin(9600);
  LCD = new LiquidCrystal(8, 9, 4, 5, 6, 7);
  LCD->begin(16,2);
  LCD->clear();
  LCD->setCursor(0, 0);          // set cursor to column 0, row 0 (the first row)
  LCD->print("Arduino");
  LCD->setCursor(0, 1);          // set cursor to column 0, row 1 (the second row)
  LCD->print("Energy Meter");
  startMillis = millis();
  autoSelect = millis();
  delay(2000);
  menu = new LCDMenu("Main Menu", LCD);        //menu gets created
  LCDMenuItem *newItem;
  newItem = new LCDMenuItem("Show data");       //lets define some menu entries
  newItem->setAction(&showInfoAction);         //here function gets assigned to menu entry - it will be executed if you select this option (Settings)
  menu->addMenuItem(newItem);
  newItem = new LCDMenuItem("Reset data");
  newItem->setAction(&resetAction);
  menu->addMenuItem(newItem);
  menu->display();
}

void loop()
{
  int buttonState = read_LCD_buttons();

  if (buttonState != lastButton)
  {
    menu->getLCD()->setCursor(0,0); 
    switch (buttonState)
    {
    case btnDOWN : 
      menu->getLCD()->print(menu->next()->getName());
      break;
    case btnUP : 
      menu->getLCD()->print(menu->prev()->getName());
      break;
    case btnSELECT : 
      menu->selectOption();
      break;  
    }
    menu->display();
    lastButton = buttonState;
  }
}

