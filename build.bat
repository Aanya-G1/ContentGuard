@echo off
REM ContentGuard — build with MSYS2 UCRT64 (libpqxx)
set PATH=C:\msys64\ucrt64\bin;%PATH%
cd /d "%~dp0"

g++ -std=c++17 -Wall -Wextra -O2 -I. ^
  userMain.cpp submission_manager.cpp ^
  preprocessor/preprocessor.cpp preprocessor/preprocessor_db.cpp ^
  Fingerprinting/FingerPrint.cpp structuralSimilarity/StructuralSimilarity.cpp ^
  exact_match/Exact_match.cpp scoring/scoring.cpp ^
  pipeline/detection_core.cpp ranking/ranking_module.cpp reporting/reporting.cpp ^
  pipeline/pipeline.cpp pipeline/token_bridge.cpp reporting/report_store.cpp ^
  -IC:/msys64/ucrt64/include -LC:/msys64/ucrt64/lib -lpqxx -lpq -lws2_32 -lwsock32 -pthread -o contentguard.exe

if %ERRORLEVEL% EQU 0 (
  echo.
  echo Build OK: contentguard.exe
  dir contentguard.exe
) else (
  echo.
  echo Build FAILED.
  exit /b 1
)
