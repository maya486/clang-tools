# --- CONFIGURATION ---
TOOL ?= UnionConversionTransformer
TOOL_DIR := $(TOOL)
TOOL_BUILD_DIR := $(TOOL_DIR)/build
TOOL_BIN := $(TOOL_BUILD_DIR)/$(TOOL)

TEST_DIR := tests/conversion
TEST_IN_DIR := $(TEST_DIR)/in
TEST_OUT_DIR := $(TEST_DIR)/out
TEST_BUILD_DIR := $(TEST_DIR)/build
INJECTED_BIN_DIR := $(TEST_BUILD_DIR)/bin
RESULTS_DIR := $(TEST_BUILD_DIR)/results
COMPILE_DB := $(TEST_BUILD_DIR)/compile_commands.json
BASELINE_BIN_DIR := $(TEST_BUILD_DIR)/baseline_bin
BASELINE_RESULTS_DIR := $(TEST_BUILD_DIR)/baseline_results
DIFF_RESULTS_DIR := $(TEST_BUILD_DIR)/diffs

TEST_INPUTS := 0.0 1.0 -2.5 42.0

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
	@rm -f $(TEST_BUILD_DIR)/inject_summary.txt
	@echo "=== Injecting all tests ==="
	@for file in $(TEST_IN_DIR)/*.cpp; do \
		out_file=$(TEST_OUT_DIR)/$$(basename $$file); \
		echo "Processing $$file -> $$out_file"; \
		if $(TOOL_BIN) -p $(TEST_BUILD_DIR) $$file > $$out_file 2>>$(TEST_BUILD_DIR)/inject_summary.txt; then \
			echo "[OK] injected $$file"; \
		else \
			echo "[INVALID] $$file could not be injected" | tee -a $(TEST_BUILD_DIR)/invalid_tests.txt; \
			rm -f $$out_file; \
		fi \
	done
	@echo "=== Transformation summary (from tool) ==="
	@cat $(TEST_BUILD_DIR)/inject_summary.txt || true

.PHONY: run-baseline
run-baseline:
	@mkdir -p $(BASELINE_BIN_DIR)
	@mkdir -p $(BASELINE_RESULTS_DIR)
	@echo "=== Compiling and running baseline (original) tests ==="
	@for cpp in $(TEST_IN_DIR)/*.cpp; do \
		bin=$(BASELINE_BIN_DIR)/$$(basename $$cpp .cpp); \
		echo "Compiling $$cpp -> $$bin"; \
		$(CXX) $(CXXFLAGS) $$cpp -o $$bin; \
		if [ $$? -ne 0 ]; then \
			echo "[FAIL] Baseline compilation failed for $$cpp"; \
			continue; \
		fi; \
		echo "Running $$bin with inputs: $(TEST_INPUTS)"; \
		rm -f $(BASELINE_RESULTS_DIR)/$$(basename $$cpp .cpp).txt; \
		for val in $(TEST_INPUTS); do \
			./$$bin $$val >> $(BASELINE_RESULTS_DIR)/$$(basename $$cpp .cpp).txt; \
		done; \
	done

.PHONY: run-injected
run-injected:
	@mkdir -p $(INJECTED_BIN_DIR)
	@mkdir -p $(RESULTS_DIR)
	@echo "=== Compiling and running injected tests ==="
	@for cpp in $(TEST_OUT_DIR)/*.cpp; do \
		bin=$(INJECTED_BIN_DIR)/$$(basename $$cpp .cpp); \
		echo "Compiling $$cpp -> $$bin"; \
		$(CXX) $(CXXFLAGS) $$cpp -o $$bin; \
		if [ $$? -ne 0 ]; then \
			echo "[FAIL] Compilation failed for $$cpp"; \
			continue; \
		fi; \
		echo "Running $$bin with inputs: $(TEST_INPUTS)"; \
		rm -f $(RESULTS_DIR)/$$(basename $$cpp .cpp).txt; \
		for val in $(TEST_INPUTS); do \
			./$$bin $$val >> $(RESULTS_DIR)/$$(basename $$cpp .cpp).txt; \
		done; \
	done

.PHONY: compare-results
compare-results:
	@mkdir -p $(DIFF_RESULTS_DIR)
	@echo "=== Comparing baseline vs injected outputs ==="
	@for cpp in $(TEST_IN_DIR)/*.cpp; do \
		name=$$(basename $$cpp .cpp); \
		base_file=$(BASELINE_RESULTS_DIR)/$$name.txt; \
		conv_file=$(RESULTS_DIR)/$$name.txt; \
		if [ ! -f $$base_file ] || [ ! -f $$conv_file ]; then \
			echo "[SKIP] Missing result for $$name"; \
			continue; \
		fi; \
		if diff -u $$base_file $$conv_file > $(DIFF_RESULTS_DIR)/$$name.diff; then \
			echo "[OK] $$name outputs match"; \
			rm -f $(DIFF_RESULTS_DIR)/$$name.diff; \
		else \
			echo "[DIFF] $$name outputs differ! See $(DIFF_RESULTS_DIR)/$$name.diff"; \
		fi; \
	done

.PHONY: full-test
full-test: inject-all run-baseline compare-results


# --- CLEAN EVERYTHING ---
.PHONY: clean
clean:
	rm -rf $(TOOL_BUILD_DIR)
	rm -rf $(TEST_BUILD_DIR)
	rm -rf $(TEST_OUT_DIR)
