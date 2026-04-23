#if defined(ARDUINO_ARCH_ESP32)
#pragma once
#include "../../interface/persistent_storage.hpp"
#include <EEPROM.h>

class Esp32PersistentStorage : AbstractPersistentStorage<Esp32PersistentStorage>
{
      public:
        template <typename T> T &get(int offset, T &t)
        {
                return EEPROM.get(offset, t);
        }
        template <typename T> const T &put(int offset, const T &t)
        {
                EEPROM.put(offset, t);
                EEPROM.commit();
                return t;
        }

        /**
         * On esp32 EEPROM is simulated in the flash storage. Because of this
         * we need to initialize it here explicitly. Note that writing to flash
         * has an even worse write cycle limit than regular EEPROM, so we need
         * to be careful we are handling it properly and not writing in tight
         * loops or by default where there is no explicit request from the user.
         */
        void setup() { EEPROM.begin(3072); }
};

#endif
