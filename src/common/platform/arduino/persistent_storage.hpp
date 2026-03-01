#ifndef EMULATOR
#include "../interface/persistent_storage.hpp"
#if !defined(ARDUINO_UNO_Q)
#include <EEPROM.h>
#endif

template <typename T> T &PersistentStorage::get(int offset, T &t)
{
#if !defined(ARDUINO_UNO_Q)
        return EEPROM.get(offset, t);
#endif
}
template <typename T> const T &PersistentStorage::put(int offset, const T &t)
{
#if !defined(ARDUINO_UNO_Q)
        return EEPROM.put(offset, t);
#endif
}
#endif
