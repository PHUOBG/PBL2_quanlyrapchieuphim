#include "core/App.h"
#ifdef _WIN32
#include <windows.h>
#endif

static void enableDpiAwareness() {
#ifdef _WIN32
    HMODULE user32 = LoadLibraryA("user32.dll");
    if (user32) {
        using SetDpiCtxFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
        auto fn = reinterpret_cast<SetDpiCtxFn>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (fn && fn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
            FreeLibrary(user32);
            return;
        }
        FreeLibrary(user32);
    }
    SetProcessDPIAware();
#endif
}

int main() {
    enableDpiAwareness();
    App app;
    app.run();
    return 0;
}
