CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I.
LIBS     = -lpqxx -lpq

SRCS = main.cpp \
       module1/submission_manager.cpp \
       module2/preprocessor.cpp \
       module2/preprocessor_db.cpp

TARGET = contentguard

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)

# ── Install dependencies ──────────────────────────────────────
# Ubuntu / Debian / WSL:
#   sudo apt install libpqxx-dev libpq-dev
#
# macOS:
#   brew install libpqxx
