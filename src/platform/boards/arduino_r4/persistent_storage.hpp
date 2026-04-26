#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
#pragma once
#include "../../interface/persistent_storage.hpp"
#include <EEPROM.h>

class ArduinoPersistentStorage
    : AbstractPersistentStorage<ArduinoPersistentStorage>
{

      public:
        template <typename T> T &get(int offset, T &t)
        {
                return EEPROM.get(offset, t);
        }
        template <typename T> const T &put(int offset, const T &t)
        {
                EEPROM.put(offset, t);
                return t;
        }

        /**
         * On the Arduino Uno R4, real EEPROM is available, so we don't need to
         * initialize it in any special way. Hence this is a no-op, in contrast
         * to the esp32 implementtation where we need to 'begin' the EEPROM.
         */
        void setup() {}
};

#endif
