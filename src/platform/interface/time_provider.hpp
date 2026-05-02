#pragma once

class TimeProvider
{
      public:
        virtual void delay_ms(int ms) const = 0;
        virtual long milliseconds() const = 0;
};
