CXX      = g++
CXXFLAGS = -Wall -std=c++23 -Iinclude

# ─── CUDA ─────────────────────────────────────────────────────────────────────
CUDA_DIR  = C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.3
CUDA_LIB_DIR = $(CUDA_DIR)/lib/x64
NVCC      = "$(CUDA_DIR)/bin/nvcc.exe"
# sm_89 = RTX 4080 Laptop (Ada Lovelace, compute capability 8.9)
# -ccbin points to MSVC compiler (cl.exe) which is strictly required by nvcc on Windows
NVCCFLAGS = -arch=sm_89 --std=c++17 -ccbin "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64" \
            -I"$(CUDA_DIR)/include" -Ithird_party/cutlass/include --expt-relaxed-constexpr \
            -Xcompiler "/Zc:preprocessor,/Zc:__cplusplus"

CUDA_SRC_DIR = src/CUDA
CUDA_SRCS    = $(wildcard $(CUDA_SRC_DIR)/gpu/*.cu) \
               $(wildcard $(CUDA_SRC_DIR)/kernel/*.cu) \
               $(wildcard $(CUDA_SRC_DIR)/util/*.cu)

# ─── CPU sources ─────────────────────────────────────────────────────────────
SRC_DIR  = src
TEST_DIR = test
UTIL_DIR = util

UTIL_SRCS = $(wildcard $(UTIL_DIR)/*.cpp) $(wildcard $(UTIL_DIR)/decompressor/*.cpp) $(wildcard $(UTIL_DIR)/compressor/*.cpp)
ALL_SRCS  = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(TEST_DIR)/*.cpp) $(UTIL_SRCS)

# ─── Object files for incremental compilation ─────────────────────────────────
ALL_OBJS  = $(ALL_SRCS:.cpp=.o)
APP_OBJS  = $(ALL_OBJS)
TEST_OBJS = $(ALL_OBJS)

# ─── Output targets ───────────────────────────────────────────────────────────
APP_TARGET       = app.exe
TEST_TARGET      = test.exe
TEST_LOSS_TARGET = test_loss.exe
TEST_LOSS_SRC    = test/CUDA/test_loss.cu
DLL_TARGET       = build/axiom_core.dll
DLL_SRCS         = src/axiom_c_api.cpp src/ai.cpp src/transfer.cpp $(UTIL_SRCS)
DLL_OBJS         = $(DLL_SRCS:.cpp=.o)

# ─── Default: compile everything ──────────────────────────────────────────────
all: $(APP_TARGET) $(TEST_TARGET) $(TEST_LOSS_TARGET)

CUDA_OBJS    = $(CUDA_SRCS:.cu=.cu.obj)

# ─── Object compilation pattern rule ──────────────────────────────────────────
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.cu.obj: %.cu
	$(NVCC) $(NVCCFLAGS) -DBUILDING_CUDA_DLL -c $< -o $@

# ─── Compile CUDA sources into a single DLL ───────────────────────────────────
# Build a shared library so MSVC's linker handles all MSVC-specific symbols.
CUDA_DLL = cuda_transfer.dll
$(CUDA_DLL): $(CUDA_OBJS)
	-$(NVCC) $(NVCCFLAGS) -shared $(CUDA_OBJS) -o $(CUDA_DLL)

# Helper: return the CUDA DLL only if it was successfully built
CUDA_LINK = $(if $(wildcard $(CUDA_DLL)),$(CUDA_DLL),)

# ─── Link CPU + GPU into the app ──────────────────────────────────────────────
$(APP_TARGET): $(APP_OBJS) $(CUDA_DLL)
	$(CXX) $(CXXFLAGS) $(APP_OBJS) $(CUDA_LINK) -o $(APP_TARGET) -lpsapi -lws2_32

# ─── Link CPU + GPU into the test binary ──────────────────────────────────────
$(TEST_TARGET): $(TEST_OBJS) $(CUDA_DLL)
	$(CXX) $(CXXFLAGS) $(TEST_OBJS) $(CUDA_LINK) -o $(TEST_TARGET) -lpsapi -lws2_32

# ─── Shared library DLL target for Python bindings ───────────────────────────
dll: $(DLL_OBJS) $(CUDA_DLL)
	@mkdir -p build ../build
	$(CXX) $(CXXFLAGS) -shared -o $(DLL_TARGET) $(DLL_OBJS) $(CUDA_LINK) -lpsapi -lws2_32
	@cp -f $(DLL_TARGET) ../build/axiom_core.dll

# ─── CUDA Kernel Loss Test Binary ─────────────────────────────────────────────
test_loss: $(TEST_LOSS_TARGET)

$(TEST_LOSS_TARGET): $(TEST_LOSS_SRC) $(CUDA_DLL)
	$(NVCC) $(NVCCFLAGS) -I$(SRC_DIR) $< $(if $(wildcard cuda_transfer.lib),cuda_transfer.lib,) -o $@

# ─── Clean ────────────────────────────────────────────────────────────────────
clean:
	-rm -f $(APP_TARGET) $(TEST_TARGET) $(CUDA_DLL) cuda_transfer.lib cuda_transfer.exp *.o *.obj *.exp *.lib *.tmp test_*.exe test_*.axiom corrupt_*.axiom weights.axiom vocab.axiom database/*.bin database/*.axiom database/*.tmp database/models/*.axiom database/models/*.bin database/models/*.tmp ../database/*.bin ../database/*.axiom ../database/*.tmp ../database/models/*.axiom ../database/models/*.bin ../database/models/*.tmp build/*.o build/*.dll $(ALL_OBJS) $(CUDA_OBJS) src/CUDA/*/*.cu.obj

# ─── Run tests ────────────────────────────────────────────────────────────────
test: $(TEST_TARGET)
	./$(TEST_TARGET) --test
