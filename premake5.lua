-- premake5.lua
workspace "VoxelAO"
    configurations { "Debug", "Release" }
    platforms { "x64" }
	startproject "VoxelAO"
	
	location "build/VisualStudio/"

project "VoxelAO"
    kind        "ConsoleApp"
    language    "C++"
    targetdir   "bin/%{cfg.buildcfg}"
    files       { "source/VoxelAO/**.*" }
    includedirs { "external/SDL2/include/", "external/DirectXTK/include/", "external/AntTweakBar/include/" }
    libdirs     { "external/SDL2/lib/", "external/AntTweakBar/lib/" }
     
    filter "configurations:Debug"
        links       { "SDL2", "DirectXTK", "AntTweakBar64" }
        objdir      "%{wks.location}/%{prj.name}/debug/obj/"
        flags       { "Symbols", "FatalCompileWarnings", "FatalLinkWarnings", "MultiProcessorCompile" }
        libdirs     { "external/DirectXTK/lib/debug/" }

    filter "configurations:Release"
        links       { "SDL2", "DirectXTK", "AntTweakBar64" }
        objdir      "%{wks.location}/%{prj.name}/release/obj/"
        optimize    "On"
        flags       { "FatalCompileWarnings", "FatalLinkWarnings",  "MultiProcessorCompile" }
        libdirs     { "external/DirectXTK/lib/release/" }