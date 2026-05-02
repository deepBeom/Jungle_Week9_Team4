#pragma once

#include "Core/CoreTypes.h"

#include <cstdarg>
#include <functional>

namespace NLogging
{
using FLogSink = std::function<void(const char* Message)>;

// Registers a runtime log sink and returns a handle used to unregister it.
uint32 RegisterLogSink(FLogSink Sink);

// Unregisters a previously registered sink. Invalid handles are ignored.
void UnregisterLogSink(uint32 SinkHandle);

// Broadcasts a preformatted log message to all outputs.
void LogMessage(const char* Message);

// Variadic helpers used by UE_LOG.
void LogV(const char* Format, va_list Args);
void Log(const char* Format, ...);
} // namespace NLogging

#define UE_LOG(Format, ...) \
    ::NLogging::Log((Format), ##__VA_ARGS__)
