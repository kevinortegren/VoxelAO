solution "VoxelAO"
	configurations { "Debug", "Release" }
	platforms { "x32" }
	
	location "build/"
	debugdir "bin/"
	
	configuration { "x32", "Debug" }
		flags { "Symbols" }
		targetsuffix "d"
		targetdir "bin/x86/"
	configuration { "x32", "Release" }
		flags { "Optimize"}
		targetdir "bin/x86/"
		
	
	
	project "VoxelAO"
		kind "ConsoleApp"
		language "C++"
		files { "VoxelAO/**.h", "VoxelAO/**.cpp" }
		includedirs { "SDL/include/", "DirectXTK/include/" }
		configuration { "x32", "Debug" }
			links { "SDL/lib/x86/SDL2", "SDL/lib/x86/SDL2_image", "DirectXTK/lib/Debug/DirectXTK" }
			objdir "build/VoxelAO/obj/"
		configuration { "x32", "Release" }
			links { "SDL/lib/x86/SDL2", "SDL/lib/x86/SDL2_image", "DirectXTK/lib/Release/DirectXTK" }
			objdir "build/VoxelAO/obj/"
