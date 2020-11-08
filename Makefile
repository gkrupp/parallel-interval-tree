BENCHMARK_SOURCES=benchmark.cpp
EXECUTABLE_SOURCES=main.cpp

CXX=g++
CXX_FLAGS=-std=c++17


clean:
	rm -rf *.out

benchmark: clean
	$(CXX) $(BENCHMARK_SOURCES) $(CXX_FLAGS) -isystem benchmark/include -Lbenchmark/build/src -lbenchmark -lpthread -o benchmark.out

main: clean
	$(CXX) $(EXECUTABLE_SOURCES) $(CXX_FLAGS) -o main.out