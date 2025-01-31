BENCHMARK_SOURCES=benchmark.cpp
PLAINBENCH_SOURCES=plainbench.cpp
EXECUTABLE_SOURCES=main.cpp

WARNING_FLAGS=-Wall -Wextra -Wpedantic

CXX=g++
CXX_FLAGS=-std=c++17


clean:
	rm -rf *.out

benchmark: clean
	$(CXX) $(BENCHMARK_SOURCES) $(CXX_FLAGS) -isystem benchmark/include -Lbenchmark/build/src -lbenchmark -lpthread -o benchmark.out

main: clean
	$(CXX) $(EXECUTABLE_SOURCES) $(CXX_FLAGS) -o main.out

plainbench: clean
	$(CXX) $(PLAINBENCH_SOURCES) $(CXX_FLAGS) -lpthread -o plainbench.out