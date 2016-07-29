#pragma once
#include <fltKernel.h>

extern "C" {

#define PICODRV_LOG_DEBUG(format, ...) \
  LogpPrint(kLogpLevelDebug, __FUNCTION__,  (format), __VA_ARGS__)

#define PICODRV_LOG_INFO(format, ...) \
  LogpPrint(kLogpLevelInfo, __FUNCTION__,  (format), __VA_ARGS__)

#define PICODRV_LOG_WARN(format, ...) \
  LogpPrint(kLogpLevelWarn, __FUNCTION__,  (format), __VA_ARGS__)

#define PICODRV_LOG_ERROR(format, ...) \
  LogpPrint(kLogpLevelError, __FUNCTION__, (format), __VA_ARGS__)

/// Buffers a message as respective severity
/// @param format   A format string
/// @return STATUS_SUCCESS on success
///
#define PICODRV_LOG_DEBUG_SAFE(format, ...)                        \
  LogpPrint(kLogpLevelDebug | kLogpLevelOptSafe, __FUNCTION__, (format), \
            __VA_ARGS__)

#define PICODRV_LOG_INFO_SAFE(format, ...)                        \
  LogpPrint(kLogpLevelInfo | kLogpLevelOptSafe, __FUNCTION__, (format), \
            __VA_ARGS__)


#define PICODRV_LOG_WARN_SAFE(format, ...)                        \
  LogpPrint(kLogpLevelWarn | kLogpLevelOptSafe, __FUNCTION__, (format), \
            __VA_ARGS__)

#define PICODRV_LOG_ERROR_SAFE(format, ...)                        \
  LogpPrint(kLogpLevelError | kLogpLevelOptSafe, __FUNCTION__, (format), \
            __VA_ARGS__)


/// Save this log to buffer and not try to write to a log file.
static const auto kLogpLevelOptSafe = 0x1ul;

static const auto kLogpLevelDebug = 0x10ul;  ///< Bit mask for DEBUG level logs
static const auto kLogpLevelInfo = 0x20ul;   ///< Bit mask for INFO level logs
static const auto kLogpLevelWarn = 0x40ul;   ///< Bit mask for WARN level logs
static const auto kLogpLevelError = 0x80ul;  ///< Bit mask for ERROR level logs

/// For LogInitialization(). Enables all levels of logs
static const auto kLogPutLevelDebug =
    kLogpLevelError | kLogpLevelWarn | kLogpLevelInfo | kLogpLevelDebug;

/// For LogInitialization(). Enables ERROR, WARN and INFO levels of logs
static const auto kLogPutLevelInfo =
    kLogpLevelError | kLogpLevelWarn | kLogpLevelInfo;

/// For LogInitialization(). Enables ERROR and WARN levels of logs
static const auto kLogPutLevelWarn = kLogpLevelError | kLogpLevelWarn;

/// For LogInitialization(). Enables an ERROR level of logs
static const auto kLogPutLevelError = kLogpLevelError;

/// For LogInitialization(). Disables all levels of logs
static const auto kLogPutLevelDisable = 0x00ul;

/// For LogInitialization(). Do not log a current time.
static const auto kLogOptDisableTime = 0x100ul;

/// For LogInitialization(). Do not log a current function name.
static const auto kLogOptDisableFunctionName = 0x200ul;

/// For LogInitialization(). Do not log a current processor number.
static const auto kLogOptDisableProcessorNumber = 0x400ul;

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS
    LogInitialization(_In_ ULONG flag, _In_opt_ const wchar_t *file_path);

/// Registers re-initialization.
/// @param driver_object  A driver object being loaded
///
/// A driver must call this function, or call LogTermination() and return non
/// STATUS_SUCCESS from DriverEntry() if LogInitialization() returned
/// STATUS_REINITIALIZATION_NEEDED. If this function is called, DriverEntry()
/// must return STATUS_SUCCESS.
_IRQL_requires_max_(PASSIVE_LEVEL) void LogRegisterReinitialization(
    _In_ PDRIVER_OBJECT driver_object);

/// Terminates the log system. Should be called from an IRP_MJ_SHUTDOWN handler.
_IRQL_requires_max_(PASSIVE_LEVEL) void LogIrpShutdownHandler();

/// Terminates the log system. Should be called from a DriverUnload routine.
_IRQL_requires_max_(PASSIVE_LEVEL) void LogTermination();

/// @param level   Severity of a message
/// @param function_name   A name of a function called this function
/// @param format   A format string
/// @return STATUS_SUCCESS on success
NTSTATUS LogpPrint(_In_ ULONG level, _In_ const char *function_name,
                   _In_ const char *format, ...);


}  // extern "C"


