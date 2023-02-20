#ifndef _CS_CODE
#define _CS_CODE

#include "String.h"

// Type   Name
// -----------------------
// 0x01   EAN-13
// 0x02   EAN-8
// 0x03   UPC-A
// 0x04   UPC-E
// 0x05   GS1-128（UCC/EAN-128）
// 0x06   Code 128
// 0x07   Code 39
// 0x08   Codebar
// 0x09   Interleaved 2 of 5
// 0x0A   Code 93
// 0x0B   Industrial 2 of 5/Standard 2 of 5(IATA)
// 0x0C   Matrix 2 of 5
// 0x0D   Datalogic 2 of 5（China Post）
// 0x0E   Code 11
// 0x10   MSI
// 0x11   GS1 DataBar
// 0x12   GS1 Limit
// 0x13   GS1 DataBar Expand
// 0x18   PDF417
// 0x19   QR code
// 0x1A   Data Matix
// 0x1B   Micro PDF417
// 0x1C   Aztec Code

class Code
{
   public:
      string data;
      uint8_t type = 0;

      void set(const char *code);
      static uint8_t Type();
};

#endif
