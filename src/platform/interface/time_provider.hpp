#pragma once

class TimeProvider
{
      public:
        virtual void delay_ms(int ms) = 0;
        virtual long milliseconds() = 0;
};
