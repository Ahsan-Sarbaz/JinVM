-- premake5.lua
workspace "VM"
    configurations { "Debug", "Release" }
    platforms {"X64"}

project "vm"
    kind "ConsoleApp"
    language "C++"
    targetdir "build/%{cfg.buildcfg}"
    links {"dl", "GL", "SDL2"}
    files { "src/*.h", "src/*.cpp" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"