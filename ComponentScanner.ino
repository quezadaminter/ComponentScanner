#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>

#include <WiFi.h>

#include <SPIFFS.h>

#include <splash.h>
#include <Adafruit_SSD1306.h>

#include "Timeout.hpp"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_I2C_ADDRESS 0x3C

// I2C Pins on ESP32-Dev board are:
// GPIO 22 SCL
// GPIO 21 SDA

#define INVALID_INT 0xFFFFFFFF
#define EMPTY_INT 0xEFFFFFFF

#define MODE_NOT_SET 0
#define MODE_ADD_COMPONENT 1
#define MODE_USE_COMPONENT 2

#define ACTIVE_BIN_NAME_LEN 64
char activeBinName[ACTIVE_BIN_NAME_LEN] = { '\0' };
uint8_t scanMode = MODE_NOT_SET;

uint8_t stateMachineMode = 0;

#define SCANNED_STRING_LEN 1024
char scannedString[SCANNED_STRING_LEN] = { '\0' };

Adafruit_SSD1306 lcd(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void WaitForScanReport();
Timeout waitForScan(0, 5000, WaitForScanReport);

class string
{
   public:
      char *mString = nullptr;
      size_t mLen = 0;
};

void DisplayError(const __FlashStringHelper *msg)
{
   lcd.clearDisplay();
   lcd.setTextSize(3);
   lcd.setTextColor(WHITE);
   lcd.setCursor(0, 0);
   lcd.print(F("ERROR:"));
   lcd.setCursor(2, 2);
   lcd.setTextSize(1);
   lcd.print(msg);
   lcd.display();
   Serial.print(F("ERROR:"));
   Serial.println(msg);
}

void DisplayMessage(const __FlashStringHelper *msg)
{
   lcd.clearDisplay();
   lcd.setTextSize(3);
   lcd.setTextColor(WHITE);
   lcd.setCursor(0, 0);
   lcd.print(msg);
   lcd.display();
   Serial.println(msg);
}


void ListComponents()
{
   File comp = SPIFFS.open("/components.csv");
   if(!comp)
   {
      DisplayError(F("No components stored yet"));
   }
   else
   {
      Serial.println(F("Captured components:"));
      while(comp.available())
      {
         Serial.write(comp.read());
      }
   }
   comp.close();
}

void WaitForScanReport()
{
   lcd.setCursor(32, 32);
   lcd.setTextSize(3);
   bool pix(lcd.getPixel(32, 32));
   if(pix == true)
   {
      lcd.setTextColor(BLACK);
   }
   else
   {
      lcd.setTextColor(WHITE);
   }
   lcd.print(F("SCAN"));
   lcd.display();
   Serial.println(F("Waiting for scan"));
}

void PromptForScan(char *msg)
{
   //lcd.clearDisplay();
   
}

string PromptForScan(const __FlashStringHelper *msg)
{
   lcd.clearDisplay();
   lcd.setTextSize(3);
   lcd.setTextColor(WHITE);
   lcd.setCursor(0, 0);
   lcd.print(F("Scan Code"));

   Serial.println(msg);

   memset(scannedString, 0, SCANNED_STRING_LEN);

      //waitForScan.Active(true);
      while(Serial2.available() == 0)
      {
         //waitForScan.RunOn(millis());
         //waitForScan.Active(false);
      }

      delay(1000);
      Serial.print(F("Available: "));
      Serial.println(Serial2.available());
      //waitForScan.Active(false);
      uint16_t i(0);
      while(Serial2.available())
      {
         scannedString[i++] = Serial2.read();
         if(i >= (SCANNED_STRING_LEN - 1))
         {
            Serial.println(F("Exceeded scannedString buffer size"));
            break;
         }
      }
      string ret;
      ret.mString = scannedString;
      ret.mLen = i;
      Serial.print(F("Scanned: "));
      Serial.println(scannedString);
      
   return(ret);
}

int32_t ReadNumericInput(int32_t in = INVALID_INT)
{
   char input[128] = { '\0' };
   uint8_t i(0);
   char c('\0');
   while(1)
   {
      c = Serial.read();
      if(c == '\n' || i >= 127)
      {
         break;
      }
      else if((c >= '0' && c <= '9') ||
              (c == '-' && i == 0))
      {
         input[i++] = c;
      }
   }

   int32_t ret(INVALID_INT);
   if(i > 0)
   {
      ret = atoi(input);
   }
   else if(c == '\n')
   {
      ret = (in == INVALID_INT ? EMPTY_INT : in);
   }
   return(ret);
}

void AskForBin()
{
   lcd.clearDisplay();
   lcd.setTextSize(2);
   lcd.setTextColor(WHITE);
   lcd.setCursor(2, 2);
   lcd.print(F("Scan Bin Code..."));
   lcd.display();

   //File binNames = SPIFFS.open("/BinNames.csv");
   //if(!file)
   //{
   //   DisplayError("Failed to open BinNames.csv");
   //}

   //while(binNames.available())
   //{
      
   //}

   memset(activeBinName, 0, ACTIVE_BIN_NAME_LEN);
   string binVal = PromptForScan(F("Scan a bin QR code."));
   if(binVal.mString != nullptr)
   {
      lcd.clearDisplay();
      lcd.setTextSize(3);
      lcd.setTextColor(WHITE);
      lcd.setCursor(2, 2);
      lcd.print(F("Active Bin:"));
      lcd.setTextSize(2);
      lcd.setCursor(32, 32);
      memcpy(activeBinName, binVal.mString, ACTIVE_BIN_NAME_LEN);
      lcd.print(activeBinName);
      Serial.print(F("Active Bin: "));
      Serial.println(activeBinName);
      stateMachineMode = 1;
   }
   else
   {
      Serial.println(F("Bin was not selected."));
      DisplayError(F("Bin was not selected."));
   }
}

void AskForScanMode()
{
   lcd.clearDisplay();
   lcd.setTextSize(2);
   lcd.setTextColor(WHITE);
   lcd.setCursor(2, 2);
   lcd.print(F("Select Scan Mode:"));
   lcd.setCursor(4, 2);
   lcd.print(F("1 ADD Components"));
   lcd.setCursor(4, 3);
   lcd.print(F("2 USE Components"));
   lcd.display();

//   waitForScan.Active(true);
//   while(Serial2.available())
//   {
//      waitForScan.RunOn(millis());
//      waitForScan.Active(false);
//   }

//   waitForScan.Active(false);
   stateMachineMode = 2;
   scanMode = MODE_ADD_COMPONENT;
}

void AddComponentsToBin()
{
   File outputFile = SPIFFS.open("/components.csv", FILE_WRITE);
   if(!outputFile)
   {
      DisplayError(F("Could not access component file for storage, halting."));
      while(1){}
   }

   size_t i(0);
   while(1)
   {
      string scanned = PromptForScan(F("Scan next component..."));
      if(scanned.mString != nullptr)
      {
         DisplayMessage(F("Enter order quantity: "));
         int32_t oqty = ReadNumericInput();
         Serial.println(oqty, DEC);
         //DisplayMessage(F("Enter current quantity: "));
         Serial.print(F("Enter current quantity ("));
         Serial.print(oqty, DEC);Serial.print(F("):"));
         int32_t rqty = ReadNumericInput(oqty);
         Serial.println(rqty, DEC);
         if(oqty != INVALID_INT)
         {
            if(rqty <= oqty)
            {
                outputFile.print(scanned.mString);
                outputFile.print(F(","));
                outputFile.print(oqty, DEC);
                outputFile.print(F(","));
                outputFile.print(rqty, DEC);
                outputFile.print(F(","));
                outputFile.println(activeBinName);
                ++i;
            }
            else
            {
               Serial.println(F("Remaining quantity CANNOT be larger than order quantity."));
            }
         }
      }
      else
      {
         DisplayError(F("Failed to scan."));
      }

      if(i >= 5)
      {
         DisplayMessage(F("Done For Now."));
         stateMachineMode = 10;
         break;
      }
   }
   outputFile.close();
   ListComponents();
}

void setup() {
  WiFi.mode(WIFI_MODE_NULL);
  // put your setup code here, to run once:
   Serial.begin(115200);
   Serial2.begin(115200);

   if(SPIFFS.begin(true) == false)
   {
      Serial.println(F("Error mounting SPIFFS, halting!"));
      while(1){}
   }
   
   if(lcd.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS) == false)
   {
      Serial.println(F("Failed to start LCD, halting!"));
      while(1){}
   }

   lcd.clearDisplay();
   lcd.setTextSize(1);
   lcd.setTextColor(WHITE);
   lcd.setCursor(0, 0);
   lcd.print(F("Starting up!"));
   lcd.display();

   ListComponents();
   
   Serial.println(F("Here we go"));
}

void loop() {
  // put your main code here, to run repeatedly:  
//  while(Serial2.available())
//  {
//    Serial.write(Serial2.read());
//  }

  if(scanMode == MODE_NOT_SET)
  {
     if(stateMachineMode == 0)
     {
        AskForBin();
     }
     else if(stateMachineMode == 1)
     {
       AskForScanMode();
     }
  }
  else if(stateMachineMode == 2)
  {
     if(scanMode == MODE_ADD_COMPONENT)
     {
        AddComponentsToBin();
     }
  }

  // TODOs:
  // Erase/Reset component file
  // Upload component file
  // Parse datamatrix here for quantity
  // Connect OLED
  // Setup encoder reader to select options
  // Component use/removal mode
}
