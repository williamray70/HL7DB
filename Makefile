# HL7DB Makefile
# Build system for HL7 v2.x database server

# Compiler and tools
CC = gcc
LEX = flex
YACC = bison -d

# Directories
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin
TEST_DIR = tests
TOOL_DIR = tools

# Compiler flags
CFLAGS = -Wall -Wextra -Werror -std=c11 -O2 -g -I$(INC_DIR) -I$(SRC_DIR)
LDFLAGS = -lpthread -lssl -lcrypto

# Source files by component
UTIL_SRC = $(wildcard $(SRC_DIR)/util/*.c)
HL7_SRC = $(wildcard $(SRC_DIR)/hl7/*.c)
STORAGE_SRC = $(wildcard $(SRC_DIR)/storage/*.c)
INDEX_SRC = $(wildcard $(SRC_DIR)/index/*.c)
NETWORK_SRC = $(wildcard $(SRC_DIR)/network/*.c)
SECURITY_SRC = $(wildcard $(SRC_DIR)/security/*.c)

# Query source files (exclude lex/yacc input files)
QUERY_SRC = $(filter-out $(SRC_DIR)/query/hl7sql.l $(SRC_DIR)/query/hl7sql.y, \
              $(wildcard $(SRC_DIR)/query/*.c))

# Generated parser files (Flex/Bison output)
PARSER_GEN_SRC = $(SRC_DIR)/query/hl7sql.tab.c $(SRC_DIR)/query/lex.yy.c
PARSER_GEN_OBJ = $(PARSER_GEN_SRC:.c=.o)

# Object files
UTIL_OBJ = $(UTIL_SRC:.c=.o)
HL7_OBJ = $(HL7_SRC:.c=.o)
STORAGE_OBJ = $(STORAGE_SRC:.c=.o)
INDEX_OBJ = $(INDEX_SRC:.c=.o)
QUERY_OBJ = $(QUERY_SRC:.c=.o)
NETWORK_OBJ = $(NETWORK_SRC:.c=.o)
SECURITY_OBJ = $(SECURITY_SRC:.c=.o)

# All core object files
CORE_OBJ = $(UTIL_OBJ) $(HL7_OBJ) $(STORAGE_OBJ) $(INDEX_OBJ) $(QUERY_OBJ) $(NETWORK_OBJ) $(SECURITY_OBJ)

# Test files
TEST_SRC = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJ = $(TEST_SRC:.c=.o)

# Tool files
CLI_SRC = $(TOOL_DIR)/hl7db-cli.c
IMPORT_SRC = $(TOOL_DIR)/hl7db-import.c

# Executables
SERVER = $(BIN_DIR)/hl7db
CLI = $(BIN_DIR)/hl7db-cli
IMPORT = $(BIN_DIR)/hl7db-import
TEST_RUNNER = $(BIN_DIR)/test_runner
QUEUE_TESTER = $(BIN_DIR)/test_queue_tester
WEB_TESTER = $(BIN_DIR)/web_queue_tester

# Default target
.DEFAULT_GOAL := all

# Build all targets
all: $(BIN_DIR) $(SERVER)
	@echo "Build complete!"

# Build everything including tools
full: all $(CLI) $(IMPORT)
	@echo "Full build complete!"

# Create bin directory if it doesn't exist
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Generate parser from Flex/Bison grammar (Phase 5)
$(SRC_DIR)/query/hl7sql.tab.c $(SRC_DIR)/query/hl7sql.tab.h: $(SRC_DIR)/query/hl7sql.y
	@echo "Generating parser from Bison grammar..."
	$(YACC) -d -o $(SRC_DIR)/query/hl7sql.tab.c $<

$(SRC_DIR)/query/lex.yy.c: $(SRC_DIR)/query/hl7sql.l $(SRC_DIR)/query/hl7sql.tab.h
	@echo "Generating lexer from Flex specification..."
	$(LEX) -o $@ $<

# Main server executable
$(SERVER): $(CORE_OBJ) $(PARSER_GEN_OBJ) $(SRC_DIR)/main.o | $(BIN_DIR)
	@echo "Linking server executable..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# CLI client tool
$(CLI): $(CORE_OBJ) $(CLI_SRC) | $(BIN_DIR)
	@echo "Building CLI client..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Import tool
$(IMPORT): $(CORE_OBJ) $(IMPORT_SRC) | $(BIN_DIR)
	@echo "Building import tool..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Test runner
$(TEST_RUNNER): $(CORE_OBJ) $(QUERY_OBJ) $(TEST_OBJ) | $(BIN_DIR)
	@echo "Building test runner..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Queue tester application
$(QUEUE_TESTER): $(CORE_OBJ) $(PARSER_GEN_OBJ) tests/integration/test_queue_tester.o | $(BIN_DIR)
	@echo "Building queue tester application..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Web queue tester (embed HTML)
web/index.o: web/index.html
	@echo "Embedding web interface..."
	@objcopy -I binary -O elf64-x86-64 -B i386:x86-64 \
		--rename-section .data=.rodata,alloc,load,readonly,data,contents \
		web/index.html web/index.o

$(WEB_TESTER): $(CORE_OBJ) $(PARSER_GEN_OBJ) tools/web_queue_tester.o web/index.o | $(BIN_DIR)
	@echo "Building web queue tester..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Run tests
test: $(TEST_RUNNER)
	@echo "Running tests..."
	@$(TEST_RUNNER)

# Compile generated parser files (relaxed warnings for generated code)
$(SRC_DIR)/query/lex.yy.o: $(SRC_DIR)/query/lex.yy.c
	@echo "Compiling generated lexer..."
	$(CC) -Wall -Wno-unused-function -std=c11 -O2 -g -I$(INC_DIR) -I$(SRC_DIR) -c $< -o $@

$(SRC_DIR)/query/hl7sql.tab.o: $(SRC_DIR)/query/hl7sql.tab.c
	@echo "Compiling generated parser..."
	$(CC) -Wall -Wno-unused-function -std=c11 -O2 -g -I$(INC_DIR) -I$(SRC_DIR) -c $< -o $@

# Compile object files
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(CORE_OBJ) $(PARSER_GEN_OBJ) $(TEST_OBJ)
	rm -f $(SRC_DIR)/main.o tests/integration/test_queue_tester.o tools/web_queue_tester.o web/index.o
	rm -f $(PARSER_GEN_SRC) $(SRC_DIR)/query/hl7sql.tab.h
	rm -f $(SERVER) $(CLI) $(IMPORT) $(TEST_RUNNER) $(QUEUE_TESTER) $(WEB_TESTER)
	find tests/ -name "*.o" -delete 2>/dev/null || true
	@echo "Clean complete!"

# Clean everything including bin directory
distclean: clean
	rm -rf $(BIN_DIR)
	@echo "Distribution clean complete!"

# Install (copy to system directories - placeholder for now)
install: $(SERVER) $(CLI) $(IMPORT)
	@echo "Install target not yet implemented"
	@echo "Binaries are available in $(BIN_DIR)/"

# Display help
help:
	@echo "HL7DB Build System"
	@echo "=================="
	@echo ""
	@echo "Build Targets:"
	@echo "  all          - Build server (default)"
	@echo "  full         - Build server and all tools"
	@echo "  tester       - Build queue tester application"
	@echo "  webtester    - Build web queue tester application"
	@echo "  rebuild      - Quick rebuild without clean"
	@echo "  rebuild-all  - Clean and rebuild everything"
	@echo ""
	@echo "Run Targets:"
	@echo "  run-server   - Start HL7DB server on port 7777"
	@echo "  run-web      - Start web tester on http://localhost:8080"
	@echo "  run-tester   - Start terminal queue tester"
	@echo ""
	@echo "Test Targets:"
	@echo "  test         - Build and run tests"
	@echo "  check-leaks  - Run web tester with valgrind"
	@echo ""
	@echo "Code Quality:"
	@echo "  analyze      - Run clang static analyzer"
	@echo "  lint         - Check code formatting"
	@echo "  format       - Auto-format code with clang-format"
	@echo ""
	@echo "Cleanup Targets:"
	@echo "  clean        - Remove build artifacts"
	@echo "  distclean    - Remove all generated files"
	@echo ""
	@echo "Other:"
	@echo "  install      - Install binaries (not yet implemented)"
	@echo "  help         - Display this help message"
	@echo ""
	@echo "Executables:"
	@echo "  $(SERVER)        - HL7 database server"
	@echo "  $(CLI)           - Interactive CLI client"
	@echo "  $(IMPORT)        - Bulk import tool"
	@echo "  $(QUEUE_TESTER)  - Queue tester application"
	@echo "  $(WEB_TESTER)    - Web queue tester application"

# Build queue tester
tester: $(QUEUE_TESTER)
	@echo "Queue tester build complete!"

# Build web queue tester
webtester: $(WEB_TESTER)
	@echo "Web queue tester build complete!"
	@echo "Run with: $(WEB_TESTER) [port]"
	@echo "Default port: 8080"

# Run web tester on port 8080
run-web: webtester
	@echo "Starting web tester on http://localhost:8080"
	./bin/web_queue_tester 8080

# Run terminal tester
run-tester: tester
	./bin/test_queue_tester

# Run main server
run-server: all
	@echo "Starting HL7DB server on port 7777"
	./bin/hl7db

# Quick rebuild (no clean)
rebuild:
	@echo "Quick rebuild..."
	@$(MAKE) --no-print-directory all

# Full rebuild (clean + build everything)
rebuild-all: clean
	@$(MAKE) --no-print-directory full tester webtester

# Check for memory leaks in web tester (requires valgrind)
check-leaks: webtester
	valgrind --leak-check=full --show-leak-kinds=all ./bin/web_queue_tester 9999

# Static analysis with clang analyzer
analyze:
	@echo "Running static analysis..."
	@echo "Analyzing source files with clang --analyze..."
	@clang --analyze $(CFLAGS) $(UTIL_SRC) $(HL7_SRC) $(STORAGE_SRC) $(NETWORK_SRC) $(SECURITY_SRC) $(QUERY_SRC) $(SRC_DIR)/main.c 2>&1 | grep -v "$(SRC_DIR)/query/hl7sql.tab.c" | grep -v "$(SRC_DIR)/query/lex.yy.c" || true
	@echo "Analysis complete!"
	@rm -f *.plist 2>/dev/null || true

# Code formatting check
lint:
	@echo "Checking code formatting..."
	@which clang-format > /dev/null || (echo "clang-format not found. Install with: sudo apt-get install clang-format" && exit 1)
	@echo "Running clang-format on source files..."
	@clang-format --dry-run --Werror $(UTIL_SRC) $(HL7_SRC) $(STORAGE_SRC) $(NETWORK_SRC) $(SECURITY_SRC) $(QUERY_SRC) $(SRC_DIR)/main.c 2>&1 | head -20 || echo "Format check complete (see errors above if any)"

# Auto-format code
format:
	@echo "Formatting code..."
	@which clang-format > /dev/null || (echo "clang-format not found. Install with: sudo apt-get install clang-format" && exit 1)
	@clang-format -i $(UTIL_SRC) $(HL7_SRC) $(STORAGE_SRC) $(NETWORK_SRC) $(SECURITY_SRC) $(QUERY_SRC) $(SRC_DIR)/main.c
	@echo "Code formatted!"

# Phony targets (not actual files)
.PHONY: all full test tester webtester clean distclean install help run-web run-tester run-server rebuild rebuild-all check-leaks analyze lint format

# Dependency tracking (auto-generated header dependencies)
-include $(CORE_OBJ:.o=.d)
-include $(QUERY_OBJ:.o=.d)
-include $(PARSER_OBJ:.o=.d)
-include $(SRC_DIR)/main.d
