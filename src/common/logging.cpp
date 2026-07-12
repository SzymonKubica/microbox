// Copyright (c) 2023 Szymon Kubica
// SPDX-License-Identifier: MIT
#include "logging.hpp"

LogLevel log_run_level = LogLevel::LOG_LVL_DEBUG;

const char *log_level_strings[] = {
    [static_cast<uint8_t>(LogLevel::LOG_LVL_NONE)] = "NONE",
    [static_cast<uint8_t>(LogLevel::LOG_LVL_ERROR)] = "ERROR",
    [static_cast<uint8_t>(LogLevel::LOG_LVL_INFO)] = "INFO",
    [static_cast<uint8_t>(LogLevel::LOG_LVL_DEBUG)] = "DEBUG",
    [static_cast<uint8_t>(LogLevel::LOG_LVL_TRACE)] = "TRACE",
};
