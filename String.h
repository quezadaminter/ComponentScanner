#ifndef _CB_STRING
#define _CB_STRING

#include <Arduino.h>

class string
{
   public:
      char *mString = nullptr;
      size_t mLen = 0;

      void Printn(Print &stream) const
      {
         for(size_t i = 0; i < mLen; ++i)
         {
            stream.print(mString[i]);
         }
      }

      void Println(Print &stream)
      {
         Printn(stream);
         stream.println();
      }
};

#endif
