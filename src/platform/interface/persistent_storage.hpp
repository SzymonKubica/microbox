#pragma once
/**
 * TODO: document why we are using CRTP here and how it works
 */
template <typename Impl> class AbstractPersistentStorage
{

      public:
        template <typename T> T &get(int offset, T &t)
        {
                return static_cast<Impl *>(this)->get(offset, t);
        }
        template <typename T> const T &put(int offset, const T &t)
        {
                return static_cast<Impl *>(this)->put(offset, t);
        }

        void setup() { static_cast<Impl *>(this)->setup(); }
};

#if defined(EMULATOR)
#include "../emulator/persistent_storage.hpp"
using PersistentStorage = EmulatorPersistentStorage;
#elif defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
#include "../boards/arduino_r4/persistent_storage.hpp"
using PersistentStorage = ArduinoPersistentStorage;
#elif defined(ARDUINO_ARCH_ESP32)
#include "../boards/esp32/persistent_storage.hpp"
using PersistentStorage = Esp32PersistentStorage;
#else
#error "Unknown platform"
#endif
