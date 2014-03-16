
#include <chs_debug.h>

inline void verbose(const char * format, ...) {
    // fprintf(stderr, "Verbose=%d\n", verbose_flag);
    if (verbose_flag == 1) {
        va_list args;
        va_start(args, format);

        fprintf(stderr, format, args);

        va_end(args);
    }
}
