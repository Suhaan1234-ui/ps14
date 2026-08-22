#ifndef PS14_LOGGER_H
#define PS14_LOGGER_H

#pragma once

#include "ps14.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Logger System
//
// Provides logging functionality with different levels and outputs.
// Supports file, console, and network logging.
// ============================================================================

// Log output targets
typedef enum Ps14LogTarget {
    PS14_LOG_TARGET_CONSOLE = 1 << 0,
    PS14_LOG_TARGET_FILE = 1 << 1,
    PS14_LOG_TARGET_NETWORK = 1 << 2,
    PS14_LOG_TARGET_ALL = 0xFFFFFFFF
} Ps14LogTarget;

// Log entry structure
typedef struct Ps14LogEntry {
    Ps14LogLevel level;
    u64 timestamp;
    u32 thread_id;
    const char* file;
    u32 line;
    const char* function;
    const char* message;
    struct Ps14LogEntry* next;
} Ps14LogEntry;

// Log callback function type
typedef void (*Ps14LogCallback)(Ps14LogLevel level, const char* message, void* user_data);

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

/**
 * @brief Initialize the logging system
 * @param log_level Minimum log level to display
 * @param targets Bitmask of log targets (console, file, network)
 * @param log_file Path to the log file (NULL for no file logging)
 * @return PS14_SUCCESS on success, error code on failure
 */
PS14_API i32 ps14_logger_init(Ps14LogLevel log_level, u32 targets, const char* log_file);

/**
 * @brief Shutdown the logging system
 */
PS14_API void ps14_logger_shutdown(void);

/**
 * @brief Set the minimum log level
 * @param level Minimum log level
 */
PS14_API void ps14_logger_set_level(Ps14LogLevel level);

/**
 * @brief Get the current log level
 * @return Current log level
 */
PS14_API Ps14LogLevel ps14_logger_get_level(void);

/**
 * @brief Set log targets
 * @param targets Bitmask of log targets
 */
PS14_API void ps14_logger_set_targets(u32 targets);

/**
 * @brief Add a log target
 * @param target Log target to add
 */
PS14_API void ps14_logger_add_target(Ps14LogTarget target);

/**
 * @brief Remove a log target
 * @param target Log target to remove
 */
PS14_API void ps14_logger_remove_target(Ps14LogTarget target);

/**
 * @brief Register a log callback
 * @param callback Callback function
 * @param user_data User data to pass to callback
 * @return Handle to the callback (for unregistering)
 */
PS14_API void* ps14_logger_register_callback(Ps14LogCallback callback, void* user_data);

/**
 * @brief Unregister a log callback
 * @param handle Callback handle
 */
PS14_API void ps14_logger_unregister_callback(void* handle);

/**
 * @brief Log a message
 * @param level Log level
 * @param file Source file
 * @param line Line number
 * @param function Function name
 * @param format Printf-style format string
 * @param ... Format arguments
 */
PS14_API void ps14_log(
    Ps14LogLevel level,
    const char* file,
    u32 line,
    const char* function,
    const char* format,
    ...
);

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

#define PS14_LOG_FATAL(format, ...) \
    ps14_log(PS14_LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define PS14_LOG_ERROR(format, ...) \
    ps14_log(PS14_LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define PS14_LOG_WARNING(format, ...) \
    ps14_log(PS14_LOG_LEVEL_WARNING, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define PS14_LOG_INFO(format, ...) \
    ps14_log(PS14_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define PS14_LOG_DEBUG(format, ...) \
    ps14_log(PS14_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define PS14_LOG_TRACE(format, ...) \
    ps14_log(PS14_LOG_LEVEL_TRACE, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

// ============================================================================
// NETWORK LOGGING (Optional)
// ============================================================================

/**
 * @brief Initialize network logging
 * @param server_address Server address
 * @param server_port Server port
 * @return PS14_SUCCESS on success, error code on failure
 */
PS14_API i32 ps14_logger_init_network(const char* server_address, u16 server_port);

/**
 * @brief Shutdown network logging
 */
PS14_API void ps14_logger_shutdown_network(void);

/**
 * @brief Send a log message over network
 * @param level Log level
 * @param message Message to send
 * @return PS14_SUCCESS on success, error code on failure
 */
PS14_API i32 ps14_logger_send_network(Ps14LogLevel level, const char* message);

// ============================================================================
// FILE LOGGING
// ============================================================================

/**
 * @brief Rotate log files
 * @param max_size Maximum file size in bytes before rotation
 * @param max_files Maximum number of rotated files to keep
 * @return PS14_SUCCESS on success, error code on failure
 */
PS14_API i32 ps14_logger_rotate_files(u64 max_size, u32 max_files);

// ============================================================================
// END OF HEADER
// ============================================================================

#ifdef __cplusplus
}
#endif

#endif // PS14_LOGGER_H
