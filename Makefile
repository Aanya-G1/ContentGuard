# ContentGuard — Makefile (modular layout)
# MSYS2 UCRT64: pacman -S mingw-w64-ucrt-x86_64-libpqxx mingw-w64-ucrt-x86_64-gcc make

CXX       = g++
TARGET    = contentguard

CXXFLAGS  = -std=c++17 -Wall -Wextra -O2
INCLUDES  = -I.

# libpqxx (MSYS2 UCRT64 default paths)
PQXX_INC  = -IC:/msys64/ucrt64/include
PQXX_LIB  = -LC:/msys64/ucrt64/lib
LIBS      = $(PQXX_LIB) -lpqxx -lpq -pthread

# Windows socket libraries (no-op on Linux if unused)
ifeq ($(OS),Windows_NT)
  LIBS += -lws2_32 -lwsock32
  TARGET := $(TARGET).exe
endif

# ── Root (API + submission layer) ─────────────────────────────
ROOT_SRCS = \
	userMain.cpp \
	submission_manager.cpp

# ── preprocessor/ ─────────────────────────────────────────────
PREPROCESSOR_SRCS = \
	preprocessor/preprocessor.cpp \
	preprocessor/preprocessor_db.cpp

# ── Fingerprinting/ ───────────────────────────────────────────
FINGERPRINTING_SRCS = \
	Fingerprinting/FingerPrint.cpp

# ── structuralSimilarity/ ─────────────────────────────────────
STRUCTURAL_SRCS = \
	structuralSimilarity/StructuralSimilarity.cpp

# ── exact_match/ ──────────────────────────────────────────────
EXACT_MATCH_SRCS = \
	exact_match/Exact_match.cpp

# ── scoring/ ──────────────────────────────────────────────────
SCORING_SRCS = \
	scoring/scoring.cpp

# ── pipeline/ ─────────────────────────────────────────────────
PIPELINE_SRCS = \
	pipeline/detection_core.cpp \
	pipeline/pipeline.cpp \
	pipeline/token_bridge.cpp

# ── ranking/ ──────────────────────────────────────────────────
RANKING_SRCS = \
	ranking/ranking_module.cpp

# ── reporting/ ────────────────────────────────────────────────
REPORTING_SRCS = \
	reporting/reporting.cpp \
	reporting/report_store.cpp

SRCS = \
	$(ROOT_SRCS) \
	$(PREPROCESSOR_SRCS) \
	$(FINGERPRINTING_SRCS) \
	$(STRUCTURAL_SRCS) \
	$(EXACT_MATCH_SRCS) \
	$(SCORING_SRCS) \
	$(PIPELINE_SRCS) \
	$(RANKING_SRCS) \
	$(REPORTING_SRCS)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(PQXX_INC) $(SRCS) $(LIBS) -o $(TARGET)

clean:
	rm -f contentguard contentguard.exe fingerprint_demo fingerprint_demo.exe
