#include "core/App.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <iostream>
#include <fstream>
#include <exception>

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

#ifdef _WIN32
static LONG WINAPI customCrashFilter(EXCEPTION_POINTERS* ep) {
    if (!ep || !ep->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;
    std::ofstream f("crash_error.log", std::ios::app);
    f << "CRASH: SEH code 0x" << std::hex << ep->ExceptionRecord->ExceptionCode 
      << " at address 0x" << ep->ExceptionRecord->ExceptionAddress << std::dec << "\n";
    f.close();
    char buf[256];
    snprintf(buf, sizeof(buf), "Ung dung gap loi he thong (SEH Code: 0x%lX).\nChi tiet da duoc ghi vao file crash_error.log", 
             (unsigned long)ep->ExceptionRecord->ExceptionCode);
    MessageBoxA(NULL, buf, "Loi ung dung", MB_OK | MB_ICONERROR);
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

int main() {
#ifdef _WIN32
    SetUnhandledExceptionFilter(customCrashFilter);
#endif
    try {
        enableDpiAwareness();
        App app;
        app.run();
    } catch (const std::exception& e) {
        std::ofstream f("crash_error.log", std::ios::app);
        f << "CRASH: std::exception: " << e.what() << "\n";
        f.close();
        MessageBoxA(NULL, e.what(), "Loi Ngoai Le (C++ Exception)", MB_OK | MB_ICONERROR);
        return 1;
    } catch (...) {
        std::ofstream f("crash_error.log", std::ios::app);
        f << "CRASH: Unknown exception occurred.\n";
        f.close();
        MessageBoxA(NULL, "Loi khong xac dinh xay ra khi chay ung dung.", "Loi Ngoai Le", MB_OK | MB_ICONERROR);
        return 2;
    }
    return 0;
}
