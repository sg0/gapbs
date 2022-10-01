# See LICENSE.txt for license details.
CXX = g++
CXX_FLAGS += -std=c++11 -O3 -Wall
PAR_FLAG = -fopenmp

ifneq (,$(findstring icpc,$(CXX)))
	PAR_FLAG = -openmp
endif

ifneq (,$(findstring sunCC,$(CXX)))
	CXX_FLAGS = -std=c++11 -xO3 -m64 -xtarget=native
	PAR_FLAG = -xopenmp
endif

ifneq (,$(findstring FCC,$(CXX)))
	CXX_FLAGS = -std=c++11 -march=armv8.2-a+sve -Kfast -Khpctag #-Kprefetch_sequential=soft -Kzfill 
	PAR_FLAG = -Kopenmp
endif

ifneq (,$(findstring g++,$(CXX)))
	CXX_FLAGS = -std=c++11 -O3 -march=armv8.2-a+sve -mtune=a64fx #-DZFILL_CACHE_LINES 
	PAR_FLAG = -fopenmp
endif

ifneq (,$(findstring armclang++,$(CXX)))
	CXX_FLAGS = -std=c++11 -Ofast -mcpu=a64fx #-DZFILL_CACHE_LINES 
	PAR_FLAG = -fopenmp
endif

ifneq ($(SERIAL), 1)
	CXX_FLAGS += $(PAR_FLAG)
endif

KERNELS = bc bfs cc cc_sv pr pr_spmv sssp tc
SUITE = $(KERNELS) converter

.PHONY: all
all: $(SUITE)

% : src/%.cc src/*.h
	$(CXX) $(CXX_FLAGS) $< -o $@

# Testing
include test/test.mk

# Benchmark Automation
include benchmark/bench.mk


.PHONY: clean
clean:
	rm -f $(SUITE) test/out/*
