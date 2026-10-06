#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>


/**
 * Extracts the first integer found inside square brackets '[ ]' in `src`.
 * Example: "TOP.S.stage[2]" -> 2
 * Returns true on success; false if no bracketed number was found or overflow.
 */
bool extract_bracket_number(const char *src, int *out_num) {
    if (!src || !out_num) return false;

    const char *open = strchr(src, '[');
    if (!open) return false;

    const char *p = open + 1;
    if (!isdigit((unsigned char)*p)) return false;  // Require digit after '['

    long value = 0;
    // Read consecutive digits
    while (*p && isdigit((unsigned char)*p)) {
        int digit = *p - '0';
        // Simple overflow guard for int (assuming 32-bit int)
        if (value > (LONG_MAX - digit) / 10) return false;
        value = value * 10 + digit;
        p++;
    }

    // Expect closing bracket after digits
    if (*p != ']') return false;

    // Additional guard to fit in int
    if (value > INT_MAX) return false;

    *out_num = (int)value;
    return true;
}

/**
 * Produces `dest` by inserting "_<n>" before the final '.' in `path`.
 * If no '.', appends "_<n>" at the end.
 * Example: "/sim.s2c" + 2 -> "/sim_2.s2c"
 *          "/sim"     + 2 -> "/sim_2"
 *
 * `dest_size` must be large enough to hold the result.
 * Returns true on success; false on buffer overflow or invalid args.
 */
bool append_number_to_path(char *dest, size_t dest_size, const char *path, int n) {
    if (!dest || !path || dest_size == 0) return false;

    const char *last_dot = strrchr(path, '.');
    char number_buf[32];
    snprintf(number_buf, sizeof(number_buf), "_%d", n);

    if (last_dot) {
        // Split into prefix (before '.') and suffix (including '.')
        size_t prefix_len = (size_t)(last_dot - path);
        size_t suffix_len = strlen(last_dot);

        // Check total length
        size_t needed = prefix_len + strlen(number_buf) + suffix_len + 1; // +1 for '\0'
        if (needed > dest_size) return false;

        memcpy(dest, path, prefix_len);
        memcpy(dest + prefix_len, number_buf, strlen(number_buf));
        memcpy(dest + prefix_len + strlen(number_buf), last_dot, suffix_len);
        dest[needed - 1] = '\0';
    } else {
        // No extension; just append
        size_t base_len = strlen(path);
        size_t needed = base_len + strlen(number_buf) + 1;
        if (needed > dest_size) return false;

        memcpy(dest, path, base_len);
        memcpy(dest + base_len, number_buf, strlen(number_buf));
        dest[needed - 1] = '\0';
    }

    return true;
}
