#include "ANSI_MH_10_8.h"
#include <string.h>

ANSI_MH_10_8_DataMatrix ANSIDM;

bool ANSI_MH_10_8_DataMatrix::setBarcode(const char *barcode)
{
   bool yes(false);
   uint32_t len(strlen(barcode));
   if(len < RAW_BARCODE_MAX_LEN)
   {
      memset(rawString, '\0', RAW_BARCODE_MAX_LEN);
      memcpy(rawString, barcode, len);
      rawStringLen = strlen(rawString);
      rawString[rawStringLen++] = END_OF_TRANSMISSION;
      yes = true;
   }
   return(yes);
}

bool ANSI_MH_10_8_DataMatrix::getStorePartNumber(string &out)
{
   return(Parse(STORE_PN, out));
}

bool ANSI_MH_10_8_DataMatrix::getManufacturerPartNumber(string &out)
{
   return(Parse(MFG_PN, out));
}

bool ANSI_MH_10_8_DataMatrix::getCustomerPurchaseOrder(string &out)
{
   return(Parse(CUST_PO, out));
}

bool ANSI_MH_10_8_DataMatrix::getOrderNumber(string &out)
{
   return(Parse(ORDER_NUM, out));
}

bool ANSI_MH_10_8_DataMatrix::getInvoiceNumber(string &out)
{
   return(Parse(INVOICE_NUM, out));
}

bool ANSI_MH_10_8_DataMatrix::getMouserInvoiceNumber(string &out)
{
   return(Parse(INVOICE_NUM_MOUSER, out));
}

bool ANSI_MH_10_8_DataMatrix::getLineItem(string &out)
{
   return(Parse(LINE_ITEM, out));
}

bool ANSI_MH_10_8_DataMatrix::getCountryOfOrigin(string &out)
{
   return(Parse(COUNTRY_OF_ORIGIN, out));
}

bool ANSI_MH_10_8_DataMatrix::getQuantity(string &out)
{
   return(Parse(QUANTITY, out));
}

bool ANSI_MH_10_8_DataMatrix::getSupplierCode(string &out)
{
   return(Parse(SUPPLIER_CODE, out));
}

bool ANSI_MH_10_8_DataMatrix::getFreeText01(string &out)
{
   return(Parse(FREE_TEXT_01, out));
}

bool ANSI_MH_10_8_DataMatrix::getFreeText02(string &out)
{
   return(Parse(FREE_TEXT_02, out));
}

bool ANSI_MH_10_8_DataMatrix::getFreeText03(string &out)
{
   return(Parse(FREE_TEXT_03, out));
}

bool ANSI_MH_10_8_DataMatrix::getFreeText10(string &out)
{
   return(Parse(FREE_TEXT_10, out));
}

const char *ANSI_MH_10_8_DataMatrix::getRawString()
{
   return(rawString);
}

bool ANSI_MH_10_8_DataMatrix::Parse(const char *forToken, string &value)
{
   bool found(false);
   if(strncmp_P(rawString, STREAM_START, strlen_P(STREAM_START)) == 0)
   {
      char token[rawStringLen] = { '\0' };
      uint8_t t(0);
      uint16_t start(0);
      for(uint16_t i = 0; i < rawStringLen; ++i)
      {
         char c = rawString[i];
         //Serial.print(t);Serial.print(":");Serial.print(i);Serial.print(":");Serial.println(c);
         if(c == GROUP_SEPARATOR || c == RECORD_SEPARATOR)
         {
            uint8_t offset(0);
            //Serial.println("EOG");
            //Serial.print(start);Serial.print(",");
            //Serial.println(token);
            if(ProcessToken(token, forToken, &offset) == true)
            {
               //Serial.print(start);Serial.print(",");
               //Serial.print(i);Serial.print(",");
               //Serial.print(offset);Serial.print(",");
               value.mString = &rawString[start + offset];
               value.mLen = i - (start + offset);
               //Serial.print(offset);Serial.print(",");
               //Serial.print(value.mString[0]);Serial.print(",");
               //Serial.println(rawString[start + offset]);
               //value = token;
               found = true;
               break;
            }
            memset(token, '\0', 32);
            t = 0;
         }
         else if(rawString[i] == END_OF_TRANSMISSION)
         {
            //Serial.println("EOT");
            break;
         }
         else
         {
            if(t == 0)
            {
               start = i;
            }
            token[t++] = c;
         }
      }
   }
   //sprintf(p, "value2: %p", value);
   //Serial.println(p);
   return(found);
}

// Compare token to the reuested record type.
bool ANSI_MH_10_8_DataMatrix::ProcessToken(char *token, const char *forToken, uint8_t *offset)
{
   bool found(false);
   if(token != nullptr && strlen(token) > 0)
   {
      char identifier[8] = { '\0' };
      if(token[0] >= 'A' && token[0] <= 'Z')
      {
         // If first character is a letter then
         // single character identifier.
         //identifier = token.substring(0, 1);
         //token = token.substring(1);
         memcpy(identifier, token, 1);
         uint8_t tlen(strlen(&token[1]));
         memcpy(token, &token[1], tlen);
         token[tlen] = '\0';
         *offset = 1;
      }
      else
      {
         // TODO: There is no need to keep the "if" part
         // of this if/else statement... The portion in
         // the else should accomodate the single character
         // identifier case as well...
         uint8_t end(0);
         for(uint8_t i = 0; i < strlen(token); ++i)
         {
            // Find first non-numeric character.
            if(token[i] >= 'A' && token[i] <= 'Z')
            {
               end = i + 1;
               break;
            }
         }
         //identifier = token.substring(0, end);
         //token = token.substring(end);
         memcpy(identifier, token, end);
         uint8_t tlen(strlen(&token[end]));
         memcpy(token, &token[end], tlen);
         token[tlen] = '\0';
         *offset = end;
      }

      if(forToken != NULL && strncasecmp_P(identifier, forToken, strlen_P(forToken)) == 0)
      {
         found = true;
      }
      else
      {
         *offset = 0;
      }
   }
   return(found);
}

void ANSI_MH_10_8_DataMatrix::toHumanReadable(Print &stream)
{
   string value;
   if(getStorePartNumber(value) == true);
   {
      stream.print(F("STO PN: "));
      value.Println(stream);
   }
   if(getManufacturerPartNumber(value) == true)
   {
      stream.print(F("MAN PN: "));
      value.Println(stream);
   }
   if(getCustomerPurchaseOrder(value) == true)
   {
      stream.print(F("CUS PO: "));
      value.Println(stream);
   }
   if(getOrderNumber(value) == true)
   {
      stream.print(F("ORD N : "));
      value.Println(stream);
   }
   if(getInvoiceNumber(value) == true)
   {
      stream.print(F("INV N : "));
      value.Println(stream);
   }
   if(getMouserInvoiceNumber(value) == true)
   {
      stream.print(F("MINV N: "));
      value.Println(stream);
   }
   if(getLineItem(value) == true)
   {
      stream.print(F("LN ITM: "));
      value.Println(stream);
   }
   if(getCountryOfOrigin(value) == true)
   {
      stream.print(F("CTY OR: "));
      value.Println(stream);
   }
   if(getQuantity(value) == true)
   {
      stream.print(F("QTY   : "));
      value.Println(stream);
   }
   if(getSupplierCode(value) == true)
   {
      stream.print(F("SUP CD: "));
      value.Println(stream);
   }
   if(getFreeText01(value) == true)
   {
      stream.print(F("FTX 01: "));
      value.Println(stream);
   }
   if(getFreeText02(value) == true)
   {
      stream.print(F("FTX 02: "));
      value.Println(stream);
   }
   if(getFreeText03(value) == true)
   {
      stream.print(F("FTX 03: "));
      value.Println(stream);
   }
   if(getFreeText10(value) == true)
   {
      stream.print(F("FTX 10: "));
      value.Println(stream);
   }
}

void ANSI_MH_10_8_DataMatrix::unitTest(Print &stream)
{
   stream.println(F("Parsing ANSI MH Data Matrix"));
   setBarcode(BARCODE_TEST_DK_BAG);
   stream.println(rawString);
   toHumanReadable(stream);
}
