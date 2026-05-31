cmake_minimum_required(VERSION 3.15)
project(ContentGuard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── libpqxx ───────────────────────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# Project root — headers use paths like "pipeline/pipeline.hpp"
set(CONTENTGUARD_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}
    ${PQXX_INCLUDE_DIRS}
)

# ── Root (API + submission layer) ─────────────────────────────
set(ROOT_SOURCES
    userMain.cpp
    submission_manager.cpp
)

# ── preprocessor/ ─────────────────────────────────────────────
set(PREPROCESSOR_SOURCES
    preprocessor/preprocessor.cpp
    preprocessor/preprocessor_db.cpp
)

# ── Fingerprinting/ ───────────────────────────────────────────
set(FINGERPRINTING_SOURCES
    Fingerprinting/FingerPrint.cpp
)

# ── structuralSimilarity/ ─────────────────────────────────────
set(STRUCTURAL_SOURCES
    structuralSimilarity/StructuralSimilarity.cpp
)

# ── exact_match/ ──────────────────────────────────────────────
set(EXACT_MATCH_SOURCES
    exact_match/Exact_match.cpp
)

# ── scoring/ ──────────────────────────────────────────────────
set(SCORING_SOURCES
    scoring/scoring.cpp
)

# ── pipeline/ ─────────────────────────────────────────────────
set(PIPELINE_SOURCES
    pipeline/detection_core.cpp
    pipeline/pipeline.cpp
    pipeline/token_bridge.cpp
)

# ── ranking/ ──────────────────────────────────────────────────
set(RANKING_SOURCES
    ranking/ranking_module.cpp
)

# ── reporting/ ────────────────────────────────────────────────
set(REPORTING_SOURCES
    reporting/reporting.cpp
    reporting/report_store.cpp
)

set(CONTENTGUARD_SOURCES
    ${ROOT_SOURCES}
    ${PREPROCESSOR_SOURCES}
    ${FINGERPRINTING_SOURCES}
    ${STRUCTURAL_SOURCES}
    ${EXACT_MATCH_SOURCES}
    ${SCORING_SOURCES}
    ${PIPELINE_SOURCES}
    ${RANKING_SOURCES}
    ${REPORTING_SOURCES}
)

# ── contentguard executable ───────────────────────────────────
add_executable(contentguard ${CONTENTGUARD_SOURCES})

target_include_directories(contentguard PRIVATE ${CONTENTGUARD_INCLUDE_DIRS})

target_link_libraries(contentguard PRIVATE
    ${PQXX_LIBRARIES}
    pq
    pthread
)

if(WIN32)
    target_link_libraries(contentguard PRIVATE ws2_32 wsock32)
endif()
