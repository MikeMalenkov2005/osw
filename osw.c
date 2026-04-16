#include "osw.h"

_OSW_C_BEGIN

#if defined(_WIN32)
#include "impl/osw_win.c"
#else
#include "impl/osw_unix.c"
#endif

#ifdef OSW_NET
#include "impl/osw_net.c"
#endif

_OSW_C_END

