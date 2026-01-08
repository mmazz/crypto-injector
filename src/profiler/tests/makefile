CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2 -march=native
LDFLAGS = -lm

PROGRAMS = test_program test_filtering

all: test_filtering

test_program: test_program.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)
	@echo "✓ test_program compilado"


test_filtering: test_filtering.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)
	@echo "✓ test_filtering compilado"
