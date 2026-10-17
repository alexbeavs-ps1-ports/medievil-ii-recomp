#pragma once
/* Fallback application-directory lookup. The recorder normally uses the mod
 * runtime root's parent, which is exactly where the engine anchors
 * settings.toml and saves on every platform; this is only for a root that is
 * not yet known. */
#include <array>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

inline std::filesystem::path medievil2_diagnostic_application_directory() {
#ifdef _WIN32
    std::array<wchar_t,32768> path{};
    DWORD n=GetModuleFileNameW(nullptr,path.data(),(DWORD)path.size());
    if(!n || n>=path.size())throw std::runtime_error("cannot locate executable");
    return std::filesystem::path(path.data()).parent_path();
#else
    /* packaging/linux/AppRun keeps persistent data in a writable directory and
     * starts the runtime through a launcher symlink there; the real executable
     * is inside the read-only AppImage mount. Mirror AppRun's data directory. */
    const char *appimage=std::getenv("MEDIEVIL_II_RECOMP_APPIMAGE_PATH");
    if(!appimage || !appimage[0])appimage=std::getenv("RECOMP_APPIMAGE_PATH");
    if(!appimage || !appimage[0])appimage=std::getenv("APPIMAGE");
    if(appimage && appimage[0]) {
        const char *data=std::getenv("MEDIEVIL_II_RECOMP_DATA_DIR");
        if(data && data[0])return std::filesystem::u8path(data);
        const char *xdg=std::getenv("XDG_DATA_HOME");
        const char *home=std::getenv("HOME");
        if(xdg && xdg[0])return std::filesystem::u8path(xdg)/"MediEvilIIRecomp";
        if(home && home[0])return std::filesystem::u8path(home)/".local/share/MediEvilIIRecomp";
    }
    return std::filesystem::read_symlink("/proc/self/exe").parent_path();
#endif
}
