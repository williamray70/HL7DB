/**
 * logger.h - Logging framework for HL7DB
 *
 * Provides thread-safe logging with multiple log levels,
 * timestamp formatting, and file/console output.
 */

#ifndef HL7DB_LOGGER_H
#define HL7DB_LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

/* Log levels */
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO  = 1,
    LOG_WARN  = 2,
    LOG_ERROR = 3,
    LOG_FATAL = 4
} log_level_t;

/* Logger configuration */
typedef struct {
    log_level_t min_level;      /* Minimum level to log */
    FILE *file;                  /* Log file handle (NULL = console only) */
    int use_color;               /* Use ANSI colors for console output */
    int include_timestamp;       /* Include timestamp in log messages */
    int include_level;           /* Include level name in log messages */
    int include_location;        /* Include file:line in log messages */
} logger_config_t;

/* Initialize logger with configuration */
int logger_init(const logger_config_t *config);

/* Initialize logger with default settings (console, INFO level) */
int logger_init_default(void);

/* Set log file */
int logger_set_file(const char *filepath);

/* Set minimum log level */
void logger_set_level(log_level_t level);

/* Get current log level */
log_level_t logger_get_level(void);

/* Close logger and cleanup */
void logger_cleanup(void);

/* Core logging functions */
void logger_log(log_level_t level, const char *file, int line, const char *fmt, ...);
void logger_vlog(log_level_t level, const char *file, int line, const char *fmt, va_list args);

/* Convenience macros */
#define LOG_DEBUG(...) logger_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  logger_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  logger_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) logger_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) logger_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

/* Conditional debug logging (compiled out in release builds) */
#ifdef NDEBUG
#define LOG_TRACE(...) ((void)0)
#else
#define LOG_TRACE(...) logger_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#endif

#endif /* HL7DB_LOGGER_H */
