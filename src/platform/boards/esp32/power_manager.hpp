#pragma once
#include "../../interface/power_manager.hpp"

class EspPowerManager : public PowerManager
{
      public:
        virtual void enter_deep_sleep() const override;
};
