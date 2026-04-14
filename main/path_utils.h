#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <stdbool.h>
#include <stddef.h>

/**
 * Sanitize a user-supplied path for SD card access.
 *
 * Rejects path traversal (..),  absolute paths (/), null bytes,
 * and paths that exceed the output buffer. Returns the sanitized
 * path with SD_MOUNT_POINT prepended.
 *
 * @param user_path  Raw path from HTTP request (may be NULL)
 * @param out        Output buffer for the full sanitized path
 * @param out_size   Size of output buffer
 * @return true on success, false if the path is invalid
 */
bool path_sanitize_sd(const char *user_path, char *out, size_t out_size);

/**
 * Check if a relative path component is safe (no traversal).
 * @return true if safe, false if contains ".." or other dangerous patterns
 */
bool path_is_safe(const char *path);

#endif /* PATH_UTILS_H */
