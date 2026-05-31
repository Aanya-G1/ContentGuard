# ContentGuard — build from PowerShell (requires MSYS2 UCRT64 libpqxx)
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

$pqxxInclude = "C:/msys64/ucrt64/include/pqxx/pqxx"
if (-not (Test-Path $pqxxInclude)) {
    Write-Host "libpqxx not found. Install in MSYS2 UCRT64:" -ForegroundColor Red
    Write-Host "  pacman -S mingw-w64-ucrt-x86_64-libpqxx"
    exit 1
}

$sources = @(
    "userMain.cpp",
    "submission_manager.cpp",
    "preprocessor/preprocessor.cpp",
    "preprocessor/preprocessor_db.cpp",
    "Fingerprinting/FingerPrint.cpp",
    "structuralSimilarity/StructuralSimilarity.cpp",
    "exact_match/Exact_match.cpp",
    "scoring/scoring.cpp",
    "pipeline/detection_core.cpp",
    "ranking/ranking_module.cpp",
    "reporting/reporting.cpp",
    "pipeline/pipeline.cpp",
    "pipeline/token_bridge.cpp",
    "reporting/report_store.cpp"
)

$gppArgs = @(
    "-std=c++17", "-Wall", "-Wextra", "-O2", "-I.",
    "-IC:/msys64/ucrt64/include",
    "-LC:/msys64/ucrt64/lib"
) + $sources + @(
    "-lpqxx", "-lpq", "-lws2_32", "-lwsock32", "-pthread",
    "-o", "contentguard.exe"
)

Write-Host ("g++ " + ($gppArgs -join " "))
& g++ @gppArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "`nBuild OK: contentguard.exe" -ForegroundColor Green
