#include <X11/Xlib.h>
#undef Always
#undef None

namespace videoplayer {

// cppcheck-suppress unusedFunction
void platformInit() {
    XInitThreads();
}

} // namespace videoplayer
