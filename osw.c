#if defined(_WIN32)
#include "impl/osw_win.c"
#elif defined(__linux__)
#include "impl/osw_unix.c"
#else
#error "The target platform is not supported by the OSW!"
#endif

