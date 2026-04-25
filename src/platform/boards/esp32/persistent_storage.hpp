#if defined(ARDUINO_ARCH_ESP32)
#pragma once
#include "../../interface/persistent_storage.hpp"
#include <EEPROM.h>
#include <nvs_flash.h>

const bool ERASE_FLASH = true;

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
        void setup()
        {
                /**
                 * When I originally experimented with the esp32 flash-based
                 * EERPROM, I would get lots of crashes with some out of memory
                 * issues. The hypothesis was that this was due to memory
                 * fragmentation. This code below allows to clean the flash if
                 * the configuration parameters that we are writing into cause
                 * too much fragmentation.
                 *
                 * TODO: this is likely not required and could be removed.
                 */
                if (ERASE_FLASH) {
                        nvs_flash_erase();
                        nvs_flash_init();
                }

                EEPROM.begin(3072);
        }
};

#endif
