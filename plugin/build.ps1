# Compiles the Papyrus scripts, scrubs the .pex headers (source path, Windows user and PC name) and writes the ESL.
# Output: plugin\build\Scripts\*.pex and plugin\build\WhoWasThisFromAgain.esp
# Needs Skyrim SE's Papyrus Compiler (Creation Kit), SKSE's script sources and the vanilla script sources (the game's
# Data\Scripts.zip, unpacked). In plugin\deps (git-ignored): MCM Helper's public script sources
# (github.com/Exit-9B/MCM-Helper, scripts/public).
# Usage: build.ps1 -Compiler <PapyrusCompiler.exe> -SkseSource <SKSE's Scripts\Source> -VanillaSource <unpacked Scripts\Source>
param(
    [Parameter(Mandatory)][string]$Compiler,
    [Parameter(Mandatory)][string]$SkseSource,
    [Parameter(Mandatory)][string]$VanillaSource
)
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$out = "$here\build"
New-Item -ItemType Directory -Force "$out\Scripts" | Out-Null
$imports = "$here\Source\Scripts;$here\deps;$SkseSource;$VanillaSource"
& $Compiler "$here\Source\Scripts" -all -optimize -f="TESV_Papyrus_Flags.flg" -i="$imports" -o="$out\Scripts"
if ($LASTEXITCODE -ne 0) { throw "Papyrus compile failed" }
python "$here\scrub_pex.py" "$out\Scripts" sayngwyn
if ($LASTEXITCODE -ne 0) { throw "scrubbing the scripts failed" }
python "$here\make_plugin.py" "$out\WhoWasThisFromAgain.esp"
if ($LASTEXITCODE -ne 0) { throw "plugin build failed" }
Get-ChildItem $out -Recurse -File | ForEach-Object { "$($_.FullName.Substring($out.Length + 1))  $($_.Length) bytes" }
