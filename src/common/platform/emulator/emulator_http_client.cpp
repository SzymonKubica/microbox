#ifdef EMULATOR
#include "./emulator_http_client.hpp"
#include <iostream>

std::optional<std::string>
EmulatorHttpClient::get(const ConnectionConfig &config, const std::string &url)
{
        std::cout << "Tried to use the emulated http client, this is not "
                     "implemented yet"
                  << std::endl;
        return std::nullopt;
}
#endif
