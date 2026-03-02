#ifndef EMULATOR
#include "../interface/persistent_storage.hpp"
#include <EEPROM.h>

template <typename T> T &PersistentStorage::get(int offset, T &t)
{
        return EEPROM.get(offset, t);
}
template <typename T> const T &PersistentStorage::put(int offset, const T &t)
{
        return EEPROM.put(offset, t);
}
#endif
