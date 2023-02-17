#ifndef _CS_ANSI_MH_10_8
#define _CS_ANSI_MH_10_8

#include <Arduino.h>
#include "String.h"

class ANSI_MH_10_8_DataMatrix
{
   public:
#define RAW_BARCODE_MAX_LEN 1024
      bool setBarcode(const char *barcode);

      bool getStorePartNumber(string &out);
      bool getManufacturerPartNumber(string &out);
      bool getCustomerPurchaseOrder(string &out);
      bool getOrderNumber(string &out);
      bool getInvoiceNumber(string &out);
      bool getMouserInvoiceNumber(string &out);
      bool getLineItem(string &out);
      bool getCountryOfOrigin(string &out);
      bool getQuantity(string &out);
      bool getSupplierCode(string &out);
      bool getFreeText01(string &out);
#define getDigiKeyPartID getFreeText02
      bool getFreeText02(string &out);
#define getDigiKeyLoadID getFreeText03
      bool getFreeText03(string &out);
      bool getFreeText10(string &out);
      const char *getRawString();
      void toHumanReadable(Print &stream);
      void unitTest(Print &stream);

      static constexpr char *BARCODE_TEST_DK_BAG PROGMEM = "[)>\u001E06\u001DPA100012-ND\u001D1P440129-3\u001DK\u001D1K77789699\u001D10K92395800\u001D11K1\u001D4LCN\u001DQ25\u001D11ZPICK\u001D12Z2077919\u001D13Z498235\u001D20Z000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
   private:
      // ANSI MH 10.8 identifier codes
      static constexpr char *STREAM_START PROGMEM = "[)>";

      static constexpr char *STORE_PN PROGMEM = "P";
      static constexpr char *MFG_PN PROGMEM = "1P";
      static constexpr char *CUST_PO PROGMEM = "K";
      static constexpr char *ORDER_NUM PROGMEM = "1K";
      static constexpr char *INVOICE_NUM PROGMEM = "10K";
      static constexpr char *INVOICE_NUM_MOUSER PROGMEM = "11K";
      static constexpr char *LINE_ITEM PROGMEM = "14K";
      static constexpr char *COUNTRY_OF_ORIGIN PROGMEM = "4L";
      static constexpr char *QUANTITY PROGMEM = "Q";
      static constexpr char *SUPPLIER_CODE PROGMEM = "1V";
      static constexpr char *FREE_TEXT_01 PROGMEM = "11Z";
      static constexpr char *FREE_TEXT_02 PROGMEM = "12Z"; //DIGIKEY Part ID
      static constexpr char *FREE_TEXT_03 PROGMEM = "13Z"; //DIGIKEY Load ID
      static constexpr char *FREE_TEXT_10 PROGMEM = "20Z";

      // These are values from the ASCII table.
      const char GROUP_SEPARATOR PROGMEM = 29;    // 0x1D
      const char RECORD_SEPARATOR PROGMEM = 30;   // 0x1E
      const char END_OF_TRANSMISSION PROGMEM = 4; // 0x04

      char rawString[RAW_BARCODE_MAX_LEN] = { '\0' };
      uint16_t rawStringLen = 0;


      bool Parse(const char *forToken, string &value);
      //bool Parse(const char *forToken, char *value, uint8_t *len);

      // Compare token to the reuested record type.
      bool ProcessToken(char *token, const char *forToken, uint8_t *offset);
};

extern ANSI_MH_10_8_DataMatrix ANSIDM;
#endif
