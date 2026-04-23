#if defined(EMULATOR)
#pragma once
#include "../interface/persistent_storage.hpp"

class EmulatorPersistentStorage
    : AbstractPersistentStorage<EmulatorPersistentStorage>
{
      public:
        template <typename T> T &get(int offset, T &t);
        template <typename T> const T &put(int offset, const T &t);
        void setup() {}
};

#include "persistent_storage.inl"

#endif
