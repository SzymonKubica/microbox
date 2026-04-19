#include "logging.hpp"
#include "../platform/interface/time_provider.hpp"

/**
 * RAII utility for measuring the duration of things. To measure the of how
 * long it takes to execute a block of code, create a new 'dummy' scope around
 * the block of code and then initialize this `DurationLogger` at the start of
 * the block. The logger will then start counting on initialization and print
 * the elapsed time upon destruction. Note that the `format_log_string` should
 * have a single '%d' format parameter that will be used for logging of the
 * elapsed time.
 */
class DurationLogger
{

      public:
        const char *format_log_string;
        TimeProvider *time_provider;
        long start_ms;
        DurationLogger(TimeProvider *time_provider, const char *format_string)
            : time_provider(time_provider), format_log_string(format_string)
        {
                start_ms = time_provider->milliseconds();
        }
        ~DurationLogger()
        {
                long elapsed = time_provider->milliseconds() - this->start_ms;
                LOG_DEBUG("duration_logger", this->format_log_string, elapsed);
        }
};
