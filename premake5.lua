require('vstudio')

premake.override(premake.vstudio.vc2010, "buildEvents", function(base, cfg)
	local write = function (event)
		local name = event .. "Event"
		local field = event:lower()
		local steps = cfg[field .. "commands"]
		local msg = cfg[field .. "message"]

		if #steps > 0 then
			steps = os.translateCommandsAndPaths(steps, cfg.project.basedir, cfg.project.location, 'windows')
			for i = 1, #steps do
                steps[i] = path.translate(steps[i])
            end
			premake.push('<%s>', name)
			premake.x('<Command>%s</Command>', table.implode(steps, "", "", "\r\n"))
			if msg then
				premake.x('<Message>%s</Message>', msg)
			end
			premake.pop('</%s>', name)
		end
    end
	write("PreBuild")
	write("PreLink")
	write("PostBuild")
end)

newoption {
    trigger = "enable-log-check",
    description = "Enable log checking"
}

newoption {
    trigger = "enable-stats",
    description = "Enable statistics"
}

newoption {
    trigger = "coverage",
    description = "Create code coverage report"
}

workspace "SuperMalloc"
    configurations { "Debug", "Release" }

filter { "configurations:Debug" }
    defines { "DEBUG" }
    symbols "On"
    targetsuffix "_d"

filter { "configurations:Release" }
    defines { "NDEBUG" }
    optimize "On"

filter {}
    language "C++"
    cppdialect "C++11"
    systemversion "latest"
    targetdir "bin/%{cfg.platform}"
    objdir "__build/%{cfg.platform}/%{cfg.buildcfg}"

filter { "system:windows" }
    platforms { "x64" }

filter { "system:windows", "configurations:Release" }
    flags { "NoIncrementalLink" }

filter { "system:linux" }
    platforms { "linux64" }
    links { "pthread", "dl" }

filter "action:vs*"
    defines { "_CRT_SECURE_NO_DEPRECATE", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_WARNINGS" }

group "SuperMalloc"

project "supermalloc"
    kind "StaticLib"
    dependson { "objsizes" }
    files { "src/*.cc", "src/*.h", "src/generated_constants.cxx", "src/generated_constants.hxx" }
    excludes { "src/objsizes.cc", "src/unit-tests.cc", "src/unit-tests.h" }
    if _OPTIONS["enable-log-check"] then
        defines { "ENABLE_LOG_CHECKING" }
    end
    if _OPTIONS["enable-stats"] then
        defines { "ENABLE_STATS" }
    end

project "objsizes"
    kind "ConsoleApp"
    files { "src/objsizes.cc" }
    postbuildcommands { "%{cfg.buildtarget.abspath:gsub('/', '\\')} src/generated_constants.cxx > src/generated_constants.hxx" }

group "Tests"

project "supermalloc_test"
    kind "ConsoleApp"
    files { "src/*.cc", "src/*.h", "src/generated_constants.cxx", "src/generated_constants.hxx" }
    excludes { "src/objsizes.cc" }
    defines {"TESTING"}
    
    if _OPTIONS["coverage"] then
        filter { "system:linux" }
            buildoptions { "-fprofile-arcs -ftest-coverage" }
            links { "gcov" }
            defines { "COVERAGE" }
    end 


group "Benchmarks"

project "alloc-test"
    kind "ConsoleApp"
    cppdialect "C++17"
    files { "benchmarks/alloc-test/*.cpp",
        "benchmarks/alloc-test/*.h" }
    includedirs { "src" }
    links { "supermalloc" }

project "larson"
    kind "ConsoleApp"
    files { "benchmarks/larson/larson.cpp" }
    includedirs {"src" }
    links { "supermalloc" }

project "xmalloc-test"
    kind "ConsoleApp"
    files { "benchmarks/xmalloc-test/*.c", "benchmarks/xmalloc-test/*.h" }
    includedirs {"src" }
    links { "supermalloc" }

if _ACTION == "clean" then
    os.rmdir("bin")
    os.rmdir("__build")
    os.remove("src/generated_constants.?xx")
end