#include "Core/Logging/Log.h"

#include <Windows.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

namespace
{
struct FLogSinkEntry
{
    uint32 Id = 0;
    NLogging::FLogSink Sink;
};

std::mutex GLogMutex;
std::vector<FLogSinkEntry> GLogSinks;
uint32 GNextSinkId = 1;

bool EndsWithNewLine(const char* Message)
{
    if (Message == nullptr)
    {
        return true;
    }

    const size_t Length = std::strlen(Message);
    if (Length == 0)
    {
        return true;
    }

    const char Tail = Message[Length - 1];
    return Tail == '\n' || Tail == '\r';
}

void WriteToDefaultOutputs(const char* Message)
{
    if (Message == nullptr)
    {
        return;
    }

    std::fputs(Message, stdout);
    if (!EndsWithNewLine(Message))
    {
        std::fputc('\n', stdout);
    }
    std::fflush(stdout);

    OutputDebugStringA(Message);
    if (!EndsWithNewLine(Message))
    {
        OutputDebugStringA("\n");
    }
}
} // namespace

namespace NLogging
{
uint32 RegisterLogSink(FLogSink Sink)
{
    if (!Sink)
    {
        return 0;
    }

    std::lock_guard<std::mutex> Lock(GLogMutex);

    const uint32 SinkId = GNextSinkId++;
    GLogSinks.push_back({SinkId, std::move(Sink)});
    return SinkId;
}

void UnregisterLogSink(uint32 SinkHandle)
{
    if (SinkHandle == 0)
    {
        return;
    }

    std::lock_guard<std::mutex> Lock(GLogMutex);

    const auto It = std::remove_if(
        GLogSinks.begin(),
        GLogSinks.end(),
        [SinkHandle](const FLogSinkEntry& Entry)
        {
            return Entry.Id == SinkHandle;
        });

    GLogSinks.erase(It, GLogSinks.end());
}

void LogMessage(const char* Message)
{
    if (Message == nullptr)
    {
        return;
    }

    std::vector<FLogSinkEntry> SinksSnapshot;
    {
        std::lock_guard<std::mutex> Lock(GLogMutex);
        SinksSnapshot = GLogSinks;
    }

    WriteToDefaultOutputs(Message);

    for (const FLogSinkEntry& Entry : SinksSnapshot)
    {
        if (Entry.Sink)
        {
            Entry.Sink(Message);
        }
    }
}

void LogV(const char* Format, va_list Args)
{
    if (Format == nullptr)
    {
        return;
    }

    va_list Copy;
    va_copy(Copy, Args);
    const int32 Required = std::vsnprintf(nullptr, 0, Format, Copy);
    va_end(Copy);

    if (Required <= 0)
    {
        return;
    }

    std::vector<char> Buffer(static_cast<size_t>(Required) + 1u, '\0');
    std::vsnprintf(Buffer.data(), Buffer.size(), Format, Args);

    LogMessage(Buffer.data());
}

void Log(const char* Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    LogV(Format, Args);
    va_end(Args);
}
} // namespace NLogging
