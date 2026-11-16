-- Who Was This From Again: one SKSE DLL for SE 1.5.97, AE 1.6.x and 1.7.x
-- (CommonLibSSE-NG, Address Library). Built by build.bat.
-- CommonLibSSE-NG comes from the COMMONLIBSSE_NG folder (build.bat sets it) or lib\commonlibsse-ng.
includes(os.getenv("COMMONLIBSSE_NG") or "lib/commonlibsse-ng")

set_project("WhoWasThisFromAgain")
set_version("1.0.0")
set_license("GPL-3.0")
set_languages("c++23")
set_warnings("allextra")

add_rules("mode.debug", "mode.releasedbg")

target("WhoWasThisFromAgain")
    add_rules("commonlibsse-ng.plugin", {
        name = "WhoWasThisFromAgain",
        author = "sayngwyn",
        description = "Who Was This From Again: named courier letters, bounties and contracts"
    })

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    -- MinHook (BSD-2-Clause, lib\minhook\LICENSE.txt): the Book Menu hook.
    add_files("lib/minhook/src/*.c", "lib/minhook/src/hde/hde64.c")
    add_includedirs("lib/minhook/include")
    set_pcxxheader("src/pch.h")
    -- Record only the .pdb's file name in the DLL, not the build folder.
    add_shflags("/PDBALTPATH:%_PDB%", {force = true})
