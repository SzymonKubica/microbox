#pragma once
/**
 * Here we are using the CRTP (curiously recursive template pattern) to ensure
 * that all implementations of the persistent storage 'interface' expose
 * funtions for get/put and setup. the idea here is that depending on the
 * platform, we then pull in here the correct implementation and set up a
 * type alias. All console code is then programmed against this type alias.
 * This is done to achieve portability and hide details of how we are
 * interfacing with the specific hardware/software implementation of the
 * persistent storage.
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
