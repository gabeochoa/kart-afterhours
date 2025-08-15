CXX ?= c++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic
INCLUDES := -I../vendor -I../

OUT := dependency_graph
SRC := dependency_graph.cpp

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $<

run: $(OUT)
	./$(OUT) --src ../src --main ../src/main.cpp --outdir ../output

clean:
	rm -f $(OUT)

.PHONY: all run clean

