#pragma once
class PersistentStorage
{
      public:
        template <typename T> T &get(int offset, T &t);
        template <typename T> const T &put(int offset, const T &t);

        void setup();
};

/*
 * Below we include the actual implementation of the templated methods. Note
 * that it is required that the templted methods have their definitions in the
 * header file as the compiler generated the concrete implementations on demand
 * depending on what parameters get passed into them. We use a different
 * implementation depending on emulator/target device, hence we need to include
 * the *.inl files conditionally.
 */
#ifdef EMULATOR
#include "../emulator/persistent_storage.inl"
#endif
