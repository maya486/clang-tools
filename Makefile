# --- CONFIGURATION ---
TOOL ?= UnionConversionTransformer
TOOL_DIR := $(TOOL)
TOOL_BUILD_DIR := $(TOOL_DIR)/build
TOOL_BIN := $(TOOL_BUILD_DIR)/$(TOOL)

TEST_DIR := tests/conversion
TEST_IN_DIR := $(TEST_DIR)/in
TEST_OUT_DIR := $(TEST_DIR)/out
TEST_BUILD_DIR := $(TEST_DIR)/build
CONVERTED_BIN_DIR := $(TEST_BUILD_DIR)/bin
RESULTS_DIR := $(TEST_BUILD_DIR)/results
COMPILE_DB := $(TEST_BUILD_DIR)/compile_commands.json

CXX ?= g++
CXXFLAGS ?= -std=c++17 -O0

# --- DEFAULT TARGET ---
.PHONY: all
all: build-tool build-tests inject-all run-tests

# --- BUILD TOOL ---
.PHONY: build-tool
build-tool:
	cmake -S $(TOOL_DIR) -B $(TOOL_BUILD_DIR) \
		-DLLVM_DIR=~/llvm-project/build/lib/cmake/llvm \
		-DClang_DIR=~/llvm-project/build/lib/cmake/clang
	cmake --build $(TOOL_BUILD_DIR) --parallel 10

# --- BUILD TESTS & COMPILATION DB ---
.PHONY: build-tests
build-tests:
	cmake -S $(TEST_DIR) -B $(TEST_BUILD_DIR) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(TEST_BUILD_DIR)

# --- RUN TOOL ON A SINGLE FILE ---
.PHONY: inject
inject:
	$(TOOL_BIN) -p $(TEST_BUILD_DIR) $(FILE) 
#$(TOOL_BIN) -p $(TEST_BUILD_DIR) $(FILE) > injected_$(notdir $(FILE))

# Example: make inject FILE=tests/foo/test1.c

# --- INJECT ALL .c FILES ---
.PHONY: inject-all
inject-all:
	@mkdir -p $(TEST_OUT_DIR)
	@rm -f $(TEST_OUT_DIR)/*.cpp
	@rm -f $(TEST_BUILD_DIR)/invalid_tests.txt
	@echo "=== Injecting all tests ==="
	@for file in $(TEST_IN_DIR)/*.cpp; do \
		out_file=$(TEST_OUT_DIR)/$$(basename $$file); \
		echo "Processing $$file -> $$out_file"; \
		if $(TOOL_BIN) -p $(TEST_BUILD_DIR) $$file > $$out_file; then \
			echo "[OK] Converted $$file"; \
		else \
			echo "[INVALID] $$file could not be converted" | tee -a $(TEST_BUILD_DIR)/invalid_tests.txt; \
			rm -f $$out_file; \
		fi \
	done

.PHONY: run-tests
run-tests:
	@mkdir -p $(CONVERTED_BIN_DIR)
	@mkdir -p $(RESULTS_DIR)
	@echo "=== Compiling and running converted tests ==="
	@for cpp in $(TEST_OUT_DIR)/*.cpp; do \
		bin=$(CONVERTED_BIN_DIR)/$$(basename $$cpp .cpp); \
		echo "Compiling $$cpp -> $$bin"; \
		$(CXX) $(CXXFLAGS) $$cpp -o $$bin; \
		if [ $$? -ne 0 ]; then \
			echo "[FAIL] Compilation failed for $$cpp"; \
			continue; \
		fi; \
		echo "Running $$bin:"; \
		./$$bin > $(RESULTS_DIR)/$$(basename $$cpp .cpp).txt; \
	done

# --- CLEAN EVERYTHING ---
.PHONY: clean
clean:
	rm -rf $(TOOL_BUILD_DIR)
	rm -rf $(TEST_BUILD_DIR)
	rm -rf $(TEST_OUT_DIR)
