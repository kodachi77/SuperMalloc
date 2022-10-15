workspace "SuperMalloc"
    configurations {"Debug", "Release"}
    platforms { "x64" }

filter "configurations:Debug"
    defines { "DEBUG" }
    symbols "On"
    targetsuffix "_d"

filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "On"

filter {}
    language "C++"
    systemversion("latest")
    targetdir "bin/%{cfg.platform}"
    objdir "__build/%{cfg.platform}/%{cfg.buildcfg}"
    defines { "_CRT_SECURE_NO_WARNINGS" }

project "supermalloc"
    kind "ConsoleApp"
    files { "src/*.cc", "src/*.h" }
    excludes { "src/objsizes.cc" }
    defines {"TESTING"}

project "objsizes"
    kind "ConsoleApp"
    files { "src/objsizes.cc" }
