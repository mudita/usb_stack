// Copyright (c) 2017-2022, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#pragma once

#ifdef USB_ENABLE_LOGS
#include <log/log.hpp>
#define log_info(...) LOG_INFO(__VA_ARGS__)
#define log_debug(...) LOG_DEBUG(__VA_ARGS__)
#define log_error(...) LOG_ERROR(__VA_ARGS__)
#else
#define log_info(...)
#define log_debug(...)
#define log_error(...)
#endif
