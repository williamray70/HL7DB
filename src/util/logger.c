/**
 * logger.c - Logging framework implementation
 */

#include "util/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

/* ANSI color codes */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_GRAY    "\033[90m"

/* Global logger state */
static struct {
    logger_config_t config;
    FILE *logfile;
    pthread_mutex_t lock;
    int initialized;
} g_logger = {
    .config = {
        .min_level = LOG_INFO,
        .file = NULL,
        .use_color = 0,
        .include_timestamp = 1,
        .include_level = 1,
        .include_location = 1
    },
    .logfile = NULL,
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .initialized = 0
};

/* Log level names */
static const char *level_names[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

/* Log level colors */
static const char *level_colors[] = {
    COLOR_GRAY,
    COLOR_CYAN,
    COLOR_YELLOW,
    COLOR_RED,
    COLOR_MAGENTA
};

/**
 * Get formatted timestamp string
 */
static void get_timestamp(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/**
 * Get filename from full path
 */
static const char *get_filename(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

/**
 * Initialize logger with configuration
 */
int logger_init(const logger_config_t *config) {
    pthread_mutex_lock(&g_logger.lock);

    if (g_logger.initialized) {
        pthread_mutex_unlock(&g_logger.lock);
        return -1; /* Already initialized */
    }

    /* Copy configuration */
    if (config) {
        g_logger.config = *config;
    }

    /* Check if stdout is a TTY for color support */
    if (isatty(STDOUT_FILENO)) {
        g_logger.config.use_color = 1;
    }

    g_logger.initialized = 1;
    pthread_mutex_unlock(&g_logger.lock);

    return 0;
}

/**
 * Initialize logger with default settings
 */
int logger_init_default(void) {
    return logger_init(NULL);
}

/**
 * Set log file
 */
int logger_set_file(const char *filepath) {
    pthread_mutex_lock(&g_logger.lock);

    /* Close existing log file if open */
    if (g_logger.logfile) {
        fclose(g_logger.logfile);
        g_logger.logfile = NULL;
    }

    /* Open new log file */
    if (filepath) {
        g_logger.logfile = fopen(filepath, "a");
        if (!g_logger.logfile) {
            pthread_mutex_unlock(&g_logger.lock);
            return -1;
        }
        /* Disable buffering for immediate writes */
        setbuf(g_logger.logfile, NULL);
    }

    pthread_mutex_unlock(&g_logger.lock);
    return 0;
}

/**
 * Set minimum log level
 */
void logger_set_level(log_level_t level) {
    pthread_mutex_lock(&g_logger.lock);
    g_logger.config.min_level = level;
    pthread_mutex_unlock(&g_logger.lock);
}

/**
 * Get current log level
 */
log_level_t logger_get_level(void) {
    log_level_t level;
    pthread_mutex_lock(&g_logger.lock);
    level = g_logger.config.min_level;
    pthread_mutex_unlock(&g_logger.lock);
    return level;
}

/**
 * Close logger and cleanup
 */
void logger_cleanup(void) {
    pthread_mutex_lock(&g_logger.lock);

    if (g_logger.logfile) {
        fclose(g_logger.logfile);
        g_logger.logfile = NULL;
    }

    g_logger.initialized = 0;
    pthread_mutex_unlock(&g_logger.lock);
}

/**
 * Core logging function with variadic arguments
 */
void logger_log(log_level_t level, const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logger_vlog(level, file, line, fmt, args);
    va_end(args);
}

/**
 * Core logging function with va_list
 */
void logger_vlog(log_level_t level, const char *file, int line, const char *fmt, va_list args) {
    /* Check if we should log this level */
    if (level < g_logger.config.min_level) {
        return;
    }

    pthread_mutex_lock(&g_logger.lock);

    /* Auto-initialize if not done */
    if (!g_logger.initialized) {
        g_logger.initialized = 1;
        if (isatty(STDOUT_FILENO)) {
            g_logger.config.use_color = 1;
        }
    }

    char timestamp[32];
    if (g_logger.config.include_timestamp) {
        get_timestamp(timestamp, sizeof(timestamp));
    }

    /* Determine output streams */
    FILE *console = (level >= LOG_ERROR) ? stderr : stdout;
    FILE *outputs[2] = { console, g_logger.logfile };

    /* Write to console and file */
    for (int i = 0; i < 2; i++) {
        FILE *out = outputs[i];
        if (!out) continue;

        int use_color = (i == 0) && g_logger.config.use_color;

        /* Timestamp */
        if (g_logger.config.include_timestamp) {
            if (use_color) {
                fprintf(out, "%s%s%s ", COLOR_GRAY, timestamp, COLOR_RESET);
            } else {
                fprintf(out, "%s ", timestamp);
            }
        }

        /* Log level */
        if (g_logger.config.include_level) {
            if (use_color) {
                fprintf(out, "%s[%-5s]%s ",
                    level_colors[level], level_names[level], COLOR_RESET);
            } else {
                fprintf(out, "[%-5s] ", level_names[level]);
            }
        }

        /* File and line */
        if (g_logger.config.include_location && file) {
            if (use_color) {
                fprintf(out, "%s%s:%d%s ",
                    COLOR_GRAY, get_filename(file), line, COLOR_RESET);
            } else {
                fprintf(out, "%s:%d ", get_filename(file), line);
            }
        }

        /* Message */
        va_list args_copy;
        va_copy(args_copy, args);
        vfprintf(out, fmt, args_copy);
        va_end(args_copy);

        fprintf(out, "\n");
        fflush(out);
    }

    pthread_mutex_unlock(&g_logger.lock);

    /* Fatal errors terminate the program */
    if (level == LOG_FATAL) {
        abort();
    }
}
