CXX = clang++
CXXFLAGS = -std=c++23 -Wall -Wextra -Werror -I./include

SRCDIR = src
OBJDIR = obj

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
TARGET = grammar_parser

.PHONY: all clean

build: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

format:
	find include src \( -name "*.hpp" -o -name "*.cpp" \) ! -path "include/nlohmann/*" -exec clang-format -i {} \;

view:
	bunx serve .

cleanall:
	rm -rf $(OBJDIR) $(TARGET)
	rm -f ./*.json
	rm -f ./a.txt

clean:
	rm -f ./*.json
	rm -f ./a.txt

parse:
	./grammar_parser

translate:
	cd trans && bun run index.ts

assemble:
	cd trans && wat2wasm output.wat -o output.wasm

run:
	cd trans && bun run run_wasm.ts

compile: parse translate assemble