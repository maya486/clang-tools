# --- CONFIGURATION ---
TOOL ?= FunctionInjector
TOOL_DIR := $(TOOL)
TOOL_BUILD_DIR := $(TOOL_DIR)/build
TOOL_BIN := $(TOOL_BUILD_DIR)/$(TOOL)

TEST_DIR := tests
TEST_BUILD_DIR := $(TEST_DIR)/build
COMPILE_DB := $(TEST_BUILD_DIR)/compile_commands.json

# --- DEFAULT TARGET ---
.PHONY: all
all: build-tool build-tests

# --- BUILD TOOL ---
.PHONY: build-tool
build-tool:
	cmake -S $(TOOL_DIR) -B $(TOOL_BUILD_DIR) \
		-DLLVM_DIR=~/llvm-project/build/lib/cmake/llvm \
		-DClang_DIR=~/llvm-project/build/lib/cmake/clang
	cmake --build $(TOOL_BUILD_DIR)

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
	find $(TEST_DIR) -name "*.c" | while read file; do \
		out=$$(basename $$file .c); \
		echo "Injecting with $(TOOL): $$file -> injected_$$out.c"; \
		$(TOOL_BIN) -p $(TEST_BUILD_DIR) $$file; \
	done
		#$(TOOL_BIN) -p $(TEST_BUILD_DIR) $$file > injected_$$out.c; \

# --- CLEAN EVERYTHING ---
.PHONY: clean
clean:
	rm -rf */build
	rm -f injected_*.c

