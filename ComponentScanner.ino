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
#include "FlashStrings.h"

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
#define MODE_VIEW_SCAN_CODES 3

#define ACTIVE_BIN_NAME_LEN 64
char activeBinName[ACTIVE_BIN_NAME_LEN] = { '\0' };
uint8_t scanMode = MODE_NOT_SET;

uint8_t stateMachineMode = 0;

#define SCANNED_STRING_LEN 1024
char scannedString[SCANNED_STRING_LEN] = { '\0' };
#define USER_INPUT_LEN 1024
char userInput[USER_INPUT_LEN] = { '\0' };

Adafruit_SSD1306 lcd(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void WaitForScanReport();
Timeout waitForScan(0, 5000, WaitForScanReport);

class string
{
   public:
      char *mString = nullptr;
      size_t mLen = 0;
};

class ReadUSBTask
{
   public:
      uint8_t status = 0;
      char input[128] = { '\0' };
      char token[64] = { '\0' };

      // ========================================
      // These methods are to be used by the
      // other thread/task.
      void join()
      {
         while(status != 3)
         {
            //Serial.println(rut.status);
            delay(1);
         }
      }

      void init()
      {
         while(status == 0)
         {
            // Wait for task to get going...
            //Serial.println(F("Waiting for task"));
            delay(1);
         }
      }

      bool running()
      {
         return(status == 1);
      }
      // ========================================
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

void ReadFromUSBSerialTask(void *params)
{
   Serial.print(F("ReadFRomUSBSerialTask running on Core: "));
   Serial.println(xPortGetCoreID());

   memset(userInput, 0, USER_INPUT_LEN);
   ReadUSBTask *task((ReadUSBTask *)params);

  uint16_t i(0);
  task->status = 1;
  while(task->status == 1)
  {
     if(Serial.available() > 0)
     {
        char c = Serial.read();
        if(c == '\n')
        {
           // Signal end
           task->status = 2;
        }
        else
        {
          task->input[i++] = c;
        }
     }
   }

   if(task->status == 2)
   {
      memcpy(userInput, task->input, i);
      Serial.print(F("user> "));
      Serial.println(task->input);
   }

   task->status = 3;
   delay(100);
   vTaskDelete(NULL);
}

string PromptForScan(const __FlashStringHelper *msg)
{
   char buf[strlen_P((const char PROGMEM *)msg) + 1] = { '\0' };
   strncpy_P(buf, (const char PROGMEM *)msg, sizeof(buf));
   return(PromptForScan(buf));
}

string PromptForScan(const char *msg)
{
   lcd.clearDisplay();
   lcd.setTextSize(3);
   lcd.setTextColor(WHITE);
   lcd.setCursor(0, 0);
   lcd.print(F("Scan Code"));

   TaskHandle_t readUSBSerialTask;
   ReadUSBTask rut;
   xTaskCreatePinnedToCore(ReadFromUSBSerialTask, "ReadUSBSerial", 10000, &rut, 0, &readUSBSerialTask, 0);

   rut.init();
   
   Serial.println(msg);

   memset(scannedString, 0, SCANNED_STRING_LEN);

   //waitForScan.Active(true);
   uint16_t i(0);
   char c(0);
   while(rut.running())
   {
      //waitForScan.RunOn(millis());
      if(Serial2.available() > 0)
      {
         c = Serial2.read();

         if(c == '\r')
         {
            // End of scan
            rut.status = 10;
            break;
         }
         scannedString[i++] = c;
         if(i >= (SCANNED_STRING_LEN - 1))
         {
            Serial.println(F("Exceeded scannedString buffer size"));
            break;
         }
      }
   }
   //waitForScan.Active(false);

   string ret;
   if(rut.status == 10)
   {
      ret.mString = scannedString;
      ret.mLen = i;
      Serial.print(F("Scanned: "));
      Serial.println(scannedString);
   }
   else
   {
      ret.mString = userInput;
      ret.mLen = strlen(userInput);
      rut.join();
   }

   Serial.println("End scan");
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
   Serial.println("setting bin");
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
      stateMachineMode = 2;
   }
   else
   {
      Serial.println(F("Bin was not selected."));
      DisplayError(F("Bin was not selected."));
   }
}

void MainMenu()
{
   stateMachineMode = 0;
   scanMode = MODE_NOT_SET;
}

void AskForScanMode()
{
   lcd.clearDisplay();
   lcd.setTextSize(2);
   lcd.setTextColor(WHITE);
   lcd.setCursor(2, 2);
   lcd.print(mode_TITLE_STR);
   lcd.setCursor(4, 2);
   lcd.print(mode_ADD_STR);
   lcd.setCursor(4, 3);
   lcd.print(mode_USE_STR);
   lcd.setCursor(4, 4);
   lcd.print(mode_VIEW_STR);
   lcd.display();

   char c(0);
   scanMode = MODE_NOT_SET;
   stateMachineMode = 0;

   while(1)
   {
      uint16_t i(0);
      memset(userInput, 0, USER_INPUT_LEN);
      Serial.println();
      Serial.println(separator_STR);
      Serial.println(mode_TITLE_STR);
      Serial.println(mode_ADD_STR);
      Serial.println(mode_USE_STR);
      Serial.println(mode_VIEW_STR);
      Serial.print(user_PROMPT_STR);
      while(1)
      {
         if(Serial.available() > 0)
         {
            c = Serial.read();
            if(c == '\n')
            {
               Serial.println(userInput);
               uint8_t v(atoi(userInput));
               if(v == 1)
               {
                  scanMode = MODE_ADD_COMPONENT;
               }
               else if(v == 2)
               {
                  scanMode = MODE_USE_COMPONENT;
               }
               else if(v == 3)
               {
                  scanMode = MODE_VIEW_SCAN_CODES;
               }
               break;
            }
            else
            {
               userInput[i++] = c;
            }
         }
      }

      if(scanMode != MODE_NOT_SET)
      {
         stateMachineMode = 1;
         Serial.print("Mode is: ");
         scanMode == MODE_ADD_COMPONENT ? Serial.println("ADD") :
                     scanMode == MODE_USE_COMPONENT ? Serial.println("USE") :
                     Serial.println("VIEW");
         break;
      }
      else
      {
         Serial.println(F("Invalid Selection!"));
      }
   }
   Serial.println(separator_STR);
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
      string scanned = PromptForScan(scan_NEXT_STR);
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

void UseComponentFromBin()
{
}

void DecodeScan()
{
   while(1)
   {
      Serial.println();
      Serial.println(separator_STR);
      string scanned = PromptForScan(scan_NEXT_STR);
      if(scanned.mString != nullptr)
      {
         if(scanned.mString == userInput)
         {
            MainMenu();
            break;
         }
         else if(scanned.mString == scannedString)
         {
            Serial.println(scanned.mString);
         }
      }
   }
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
        AskForScanMode();
     }
  }
  else if(scanMode == MODE_ADD_COMPONENT)
  {
     if(stateMachineMode == 1)
     {
        AskForBin();
     }
     AddComponentsToBin();
  }
  else if(scanMode == MODE_USE_COMPONENT)
  {
     if(stateMachineMode == 1)
     {
        AskForBin();
     }
     UseComponentFromBin();
  }
  else if(scanMode == MODE_VIEW_SCAN_CODES)
  {
     DecodeScan();
  }

  // TODOs:
  // Erase/Reset component file
  // Upload component file
  // Parse datamatrix here for quantity
  // Connect OLED
  // Setup encoder reader to select options
  // Component use/removal mode
}
