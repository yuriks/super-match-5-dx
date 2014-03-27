solution "SuperMatch5DX"
	configurations { "Debug", "Release" }

	flags { "FatalWarnings", "NoRTTI", "Unicode" }
	warnings "Extra"
	floatingpoint "Fast"
	vectorextensions "SSE2"

	targetdir "bin"

	configuration "Debug"
		defines { "DEBUG" }
		flags { "Symbols" }
		optimize "Off"

	configuration "Release"
		defines { "NDEBUG" }
		optimize "On"

	configuration "vs*"
		defines { "_CRT_SECURE_NO_WARNINGS" }

	project "libyuriks"
		kind "StaticLib"
		language "C++"
		files { "libyuriks/**.cpp", "libyuriks/**.hpp", "libyuriks/**.c", "libyuriks/**.h" }
		includedirs { "libyuriks" }

	project "SuperMatch5DX"
		kind "ConsoleApp"
		language "C++"
		files { "src/**.cpp", "src/**.hpp", "src/**.c", "src/**.h" }
		includedirs { "src", "libyuriks" }

		links { "SDL2", "SDL2main", "libyuriks" }

		configuration "Windows"
			linkoptions { "/NODEFAULTLIB:msvcrt" }
			links { "OpenGL32" }
