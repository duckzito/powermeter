// Simple AC Power Monitoring By Solarduino //

// Note :  Safety is very important when dealing with high voltage source such as 240VAC power source. We take no responsibilities while you do it at your own risk.
// Note :  Simple AC Power Monitoring is designed for the purpose of user-friendly with no wiring modification needed specifically on high voltage power.
// Note :  AC Power Monitoring can measure and record (in SD card) instantaneous Current (I)(RMS), apparent Power (VA), daily Energy (kWH) consumption and Accumulated Energy (kWH)
// Note :  Since it does not have voltage sensor, the voltage is assuming constant all the time and required user to input the value.
// Note :  The power and energy calculated is based on Apparent Power (product of RMS of Current and Voltage).
// Note :  The unit provides reasonable accuracy and may not be comparable with other expensive branded datalogger and multimeter.
// Note :  All credit shall be given to Solarduino.

/*/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

        /* General */

        int decimalPrecision = 1;                   // decimal places for all values shown in LED Display & Serial Monitor
                                                    // small values such as current and daily energy will be 2 * decimal precision
                                                    // large values such as power and accumulated energy will be in decimal precision


        /* 1- AC Voltage Measurement */

        int inputVoltageValue = 230;                // Key in the RMS Voltage value such as 230Vac, 240Vac and 120Vac based on your utility grid


        /* 2- AC Current Measurement */

        int CurrentAnalogInputPin = A1;             // Which pin in Arduino to measure Current Value (Pin A0 is reserved for button function in LCD Shield).
        float mVperAmpValue = 20;                // Highly recommend using "Hall-Effect Split-Core Current Transformer" for higher accuracy in measuring current.
                                                    // Optionally can be replaced by normal Current Transformer (CT) (xxA/5A) and a ACS712 current module (5A) module (BOTH HAVE TO USE TOGETHER but may not be as accurate as Hall Effect CT).
                                                    // If using ACS712 current module with normal CT : for 5A module key in 185, for 20A module key in 100, for 30A module key in 66.
                                                    // If using "Hall-Effect" Current Transformer, key in value using this formula: mVperAmp = maximum voltage range (in milli volt) / current rating of CT
                                                    // For example, a 20A Hall-Effect Current Transformer rated at 20A, 2.5V +/- 0.625V, mVperAmp will be 625 mV / 20A = 31.25mV/A */
        float CTRatio = 1;                          // CT Ratio is the magnification size of your normal Current Transformer xxA/5A. if using 100A/5A CT, key in 20.
                                                    // High CT Ration will reduce accuracy when measuring low current, select your CT wisely
                                                    // If using "Hall-Effect Current Transformer, leave it as 1. Do not change to 0.
        float currentSampleRead  = 0;               /* to read the current value from a sample*/
        float currentLastSample  = 0;               /* Use for counting time for current sample */
        float currentSampleSum   = 0;               /* accumulation of current reading samples */
        float currentSampleCount = 0;               /* to count number of sample. */
        float currentMean ;                         /* to calculate the current average values from all samples*/
        float RMSCurrentMean ;                      /* square roof of currentMean*/
        float FinalRMSCurrent1 ;                    /* all raw readsings are calculated and convert to expected curent reading*/
        float FinalRMSCurrent2 ;                    /* added offset2. Is the final current value */


            /* 2.1 - AC Current Offset */

            float currentOffset1 =0.00;             // Different module might have different deviation value.
                                                    // This value is to make sure the current sensing wave always exact at middle point during no current or during fluctuation.
                                                    // This value will be automatically offset when you press the "SELECT" BUTTON on the LCD Sheild.
                                                    // Press the "SELECT" BUTTON only when there is no current flow during the measurement to make it at 0A.
                                                    // It will do its calculation for about 3 seconds before automatically going to the next offset settings (currentOffset2)
                                                    // The whole auto calibration takes about 6 seconds, kindly patient and do not repeatly pressing SELECT button.
            float currentOffset2 =0.00;             // Due to squaring up the current value for RMS, there are some data noises and current value is no longer at zero point during no current.
                                                    // So this offset is to offset the noise value and zerorised the values when current not in operation.
                                                    // This offset is automatically continued by first offset when the LCD "SELECT" is pressed once.
                                                    // For this offset, it requires another 3 seconds for calibration.
                                                    // Once both of these offset have been set, you may see the current becoming 0A during no current run.
                                                    // You may press "SELECT" button again if the callibration result not satisfied.
            int   OffsetRead = 0;                   /* To switch between functions for auto callibation purpose */
            float offset1LastSample = 0;            /* Use for counting time for currentOffset1 */
            float offset2LastSample = 0;            /* Use for counting time for currentOffset2 */
            float offset1SampleCount = 0;           /* to count number of sample. */
            float offset2SampleCount = 0;           /* to count number of sample. */
            float offsetCurrentSampleSum= 0;        /* Accumulated / Sum value use for currentOffset2 purpose*/
            float offsetCurrentSampleSumMean = 0;   /* Is the average value use for currentOffset2 purpose*/


            /* 2.2- AC Power Measurement */

            float apparentPower =   0;              /* recorded by multiplying RMS voltage and RMS current*/
            float powerLastSample = 0;              /* Use for counting time for Apparent Power */
            float powerSampleCount= 0;              /* to count number of sample. */
            float powerSampleSum  = 0;              /* accumulation of sample readings */


            /* 2.3 - Daily Energy Measurement*/

            float energyLastSample = 0;             /* Use for counting time for Apparent Power */
            float energySampleCount= 0;             /* to count number of sample. */
            float energySampleSum  = 0;             /* accumulation of sample readings */
            float finalEnergyValue = 0;             /* total accumulate energy */
            float accumulateEnergy = 0;             /* accumulate of energy readings*/

        /* 4 - LCD Display  */

        #include<LiquidCrystal.h>                   /* Load the liquid Crystal Library (by default already built-it with arduino solftware)*/
        LiquidCrystal LCD(8,9,4,5,6,7);             /* Creating the LiquidCrystal object named LCD. The pin may be varies based on LCD module that you use*/
        unsigned long startMillisLCD;               /* start counting time for LCD Display */
        unsigned long currentMillisLCD;             /* current counting time for LCD Display */
        const unsigned long periodLCD = 1000;       // refresh every X seconds (in milli seconds) in LED Display. Default 1000 = 1 second


void setup()                                        /*codes to run once */

{

        /* 0- General */

        Serial.begin(9600);                               /* to display readings in Serial Monitor at 9600 baud rates */


        /* 4 - LCD Display  */

        LCD.begin(16,2);                                  /* Tell Arduino that our LCD has 16 columns and 2 rows*/
        LCD.setCursor(0,0);                               /* Set LCD to start with upper left corner of display*/
        startMillisLCD = millis();                        /* Start counting time for LCD display*/

}


void loop()                                                                   /*codes to run again and again */
{

        /* 0- General */


              /* 0.1- Button Function */

              int buttonRead;
              buttonRead = analogRead (0);                                    // Read analog pin A0

              /*Right button is pressed */
              if (buttonRead < 60)
              {   LCD.setCursor(0,0);
                  LCD.print ("PRESS <SELECT>   ");
                  LCD.setCursor(0,1);
                  LCD.print ("TO CALIBRATE      ");
              }

              /* Up button is pressed */
              else if (buttonRead < 200)
              {   LCD.setCursor(0,0);
                  LCD.print ("PRESS <SELECT>   ");
                  LCD.setCursor(0,1);
                  LCD.print ("TO CALIBRATE      ");
              }

              /* Down button is pressed */
              else if (buttonRead < 400)
              {   LCD.setCursor(0,0);
                  LCD.print ("PRESS <SELECT>  ");
                  LCD.setCursor(0,1);
                  LCD.print ("TO CALIBRATE      ");
              }

              /* Left button is pressed */
              else if (buttonRead < 600)
              {   LCD.setCursor(0,0);
                  LCD.print ("PRESS <SELECT>   ");
                  LCD.setCursor(0,1);
                  LCD.print ("TO CALIBRATE      ");
              }

              /* Select button is pressed */
              else if (buttonRead < 800)
              {
              OffsetRead = 1;                                                 // to activate offset
              LCD.setCursor(0,0);
              LCD.print ("INITIALIZING..... ");
              LCD.setCursor(0,1);
              LCD.print ("WAIT 5 SEC ..... ");
              }



        /* 2- AC Current Measurement */

        if(millis() >= currentLastSample + 1)                                                                /* every 1 milli second taking 1 sample reading */
          {
            currentSampleRead = analogRead(CurrentAnalogInputPin)-512 + currentOffset1;                      /* read the sample value */
            currentSampleSum = currentSampleSum + sq(currentSampleRead);                                     /* accumulate value with older sample readings*/
            currentSampleCount = currentSampleCount + 1;                                                     /* to move on to the next following count */
            currentLastSample = millis();                                                                    /* to reset the time again so that next cycle can start again*/

            offsetCurrentSampleSum = offsetCurrentSampleSum + currentSampleRead ;                            /* accumulate value for offsetcurrent2*/
          }

        if(currentSampleCount == 1000)                                                                       /* after 1000 count or 1000 milli seconds (1 second), do the calculation and display value*/
          {
            offsetCurrentSampleSumMean = offsetCurrentSampleSum/currentSampleCount;                          /* use for currentoffset1 purpose*/

            currentMean = currentSampleSum/currentSampleCount;                                               /* calculate average value of all sample readings taken*/
            RMSCurrentMean = sqrt(currentMean);                                                              /* square root of the average value*/
            FinalRMSCurrent1 = ((((RMSCurrentMean /1024) *5000) /mVperAmpValue)*(CTRatio));                  /* calculate the final RMS current*/
            FinalRMSCurrent2 = FinalRMSCurrent1 + currentOffset2 ;                                           /* including offset2 */
            Serial.print(FinalRMSCurrent2,2*decimalPrecision);
	    Serial.print(" A   ");
            currentSampleSum =0;                                                                              /* to reset accumulate sample values for the next cycle */
            offsetCurrentSampleSum =0;                                                                        /* to reset accumulate value for offset*/
            currentSampleCount=0;                                                                             /* to reset number of sample for the next cycle */
          }


          /* 2.1 - Offset AC Current */

          if(OffsetRead == 1)
            {
             currentOffset1 = 0;                                                              /* set back currentOffset as default*/
               if(millis() >= offset1LastSample + 1)                                          /* offset 1 - to centralise analogRead waveform*/
                {
                  offset1SampleCount = offset1SampleCount + 1;
                  offset1LastSample = millis();
                }

                  if(offset1SampleCount == 2500)                                              /* need to wait first offset take into effect.  */
                {                                                                             /* So this code is to delay 2.5 seconds after button pressed */
                  currentOffset1 = - offsetCurrentSampleSumMean;                              /* to offset values */
                  OffsetRead = 2;                                                             /* until next offset button is pressed*/
                  offset1SampleCount = 0;                                                     /* to reset the time again so that next cycle can start again */
                }
            }


          if(OffsetRead == 2)                                                                 /* offset 2 - to zerolise non-zero values during RMS calculation, */
            {
              currentOffset2 = 0;
               if(millis() >= offset2LastSample + 1)
                {
                  offset2SampleCount = offset2SampleCount + 1;
                  offset2LastSample = millis();
                }
                  if(offset2SampleCount == 2500)
                {
                  currentOffset2 =  - FinalRMSCurrent1 ;
                  OffsetRead = 0;
                  offset2SampleCount = 0;                                                     /*   to reset the time again so that next cycle can start again */
                }
            }


          /* 2.2- AC Power Measurement */

          if(millis() >= powerLastSample + 1)                                                 /* every 1 milli second taking 1 reading */
            {
              powerSampleCount = powerSampleCount + 1;
              powerLastSample = millis();
            }
          if(powerSampleCount == 1000)                                                        /* after 1000 count or 1000 milli seconds (1 second), do the calculation and display value*/
            {
              apparentPower = inputVoltageValue*FinalRMSCurrent2;                             /* calculate average value of all sample readings */
              Serial.print(apparentPower,decimalPrecision);
	      Serial.print(" VA   ");
              powerSampleSum =0;                                                              /* to reset accumulate sample values for the next cycle */
              powerSampleCount=0;                                                             /* to reset number of sample for the next cycle */
            }


          /* 2.3 - Accumulate & Daily Energy Measurement*/

          if(millis() >= energyLastSample + 1)                                                /* every 1 milli second taking 1 reading */
          {
            energySampleCount = energySampleCount + 1;
            energyLastSample = millis();
          }
          if(energySampleCount == 1000)                                                       /* after 1000 count or 1000 milli seconds (1 second), do the calculation and display value*/
          {
            accumulateEnergy = apparentPower/3600;                                            /* daily and accumulative seperated*/
            finalEnergyValue = finalEnergyValue + accumulateEnergy;
            Serial.print(finalEnergyValue/1000,2*decimalPrecision);
            Serial.println(" kWh");
            energySampleCount = 0 ;                                                           /* Set the starting point again for next counting time */
          }



        /* 5 - LCD Display  */

        currentMillisLCD = millis();                                                          /* Set current counting time */
        if (currentMillisLCD - startMillisLCD >= periodLCD)                                   /* for every x seconds, run the codes below*/
          {
            LCD.setCursor(0,0);                                                               /* Set cursor to first colum 0 and second row 1  */
            LCD.print(FinalRMSCurrent2,2*decimalPrecision);                                     /* display current value in LCD in first row  */
            LCD.print("A                ");
            LCD.setCursor(0,1);
            LCD.print(apparentPower,decimalPrecision);                                        /* display power value in LCD in first row  */
            LCD.print("W  ");
            LCD.setCursor(10,1);
            LCD.print(finalEnergyValue/1000,decimalPrecision);                                /* display total energy value in LCD in first row  */
            LCD.print("kWh  ");
            startMillisLCD = currentMillisLCD ;                                               /* Set the starting point again for next counting time */



	  }


}
