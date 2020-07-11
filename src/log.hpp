#pragma once

// For SPDLOG_ACTIVE_LEVEL #defines
#include <spdlog/common.h>

// define SPDLOG_ACTIVE_LEVEL to one of those (before including spdlog.h):
// SPDLOG_LEVEL_TRACE = 0
// SPDLOG_LEVEL_DEBUG
// SPDLOG_LEVEL_INFO
// SPDLOG_LEVEL_WARN
// SPDLOG_LEVEL_ERROR
// SPDLOG_LEVEL_CRITICAL
// SPDLOG_LEVEL_OFF

#if !defined(SPDLOG_ACTIVE_LEVEL)
#	define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#include <spdlog/spdlog.h>

#define LOG_TRACE SPDLOG_TRACE
#define LOG_DEBUG SPDLOG_DEBUG
#define LOG_INFO SPDLOG_INFO
#define LOG_WARN SPDLOG_WARN
#define LOG_ERROR SPDLOG_ERROR
#define LOG_CRITICAL SPDLOG_CRITICAL