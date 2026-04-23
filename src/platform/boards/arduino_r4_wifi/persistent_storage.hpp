#if defined(ARDUINO_UNOR4_WIFI)
#pragma once
#include "../../interface/persistent_storage.hpp"
#include <EEPROM.h>

template <typename T> T &PersistentStorage::get(int offset, T &t)
{
        return EEPROM.get(offset, t);
}
template <typename T> const T &PersistentStorage::put(int offset, const T &t)
{
        T result = EEPROM.put(offset, t);
        EEPROM.commit();
        return result;
}

/**
 * On the Arduino Uno R4, real EEPROM is available, so we don't need to
 * initialize it in any special way. Hence this is a no-op, in contrast to the
 * esp32 implementtation where we need to 'begin' the EEPROM.
 */
void PersistentStorage::setup() {}
#endif
