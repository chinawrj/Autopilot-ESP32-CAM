#include "path_utils.h"

#include <string.h>
#include <stdio.h>

#ifndef SD_MOUNT_POINT
#define SD_MOUNT_POINT "/sdcard"
#endif

bool path_is_safe(const char *path)
{
    if (!path || path[0] == '\0') return false;

    /* Reject absolute paths */
    if (path[0] == '/') return false;

    /* Reject ".." anywhere in the path */
    const char *p = path;
    while (*p) {
        if (p[0] == '.' && p[1] == '.') {
            /* ".." at start, "/.."/end, or "/../" */
            if (p == path || *(p - 1) == '/')
                if (p[2] == '\0' || p[2] == '/')
                    return false;
        }
        p++;
    }

    /* Reject backslashes (Windows path traversal) */
    if (strchr(path, '\\')) return false;

    return true;
}

bool path_sanitize_sd(const char *user_path, char *out, size_t out_size)
{
    if (!out || out_size == 0) return false;
    out[0] = '\0';

    /* NULL or empty → mount point root */
    if (!user_path || user_path[0] == '\0') {
        if (strlen(SD_MOUNT_POINT) + 1 > out_size) return false;
        snprintf(out, out_size, "%s", SD_MOUNT_POINT);
        return true;
    }

    /* Skip leading slashes — treat as relative */
    while (*user_path == '/') user_path++;

    if (user_path[0] == '\0') {
        snprintf(out, out_size, "%s", SD_MOUNT_POINT);
        return true;
    }

    if (!path_is_safe(user_path)) return false;

    /* Check output buffer size */
    size_t needed = strlen(SD_MOUNT_POINT) + 1 + strlen(user_path) + 1;
    if (needed > out_size) return false;

    snprintf(out, out_size, "%s/%s", SD_MOUNT_POINT, user_path);
    return true;
}
