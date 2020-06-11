# vebtest.exe makefile for Linux clang compilation.

# First set MOD_DEFINES to specialized defines for this specific project.
#MOD_DEFINES = -D__NDEBUG_THROW
# -D__DEBUG_THROW_VERBOSE 
#MODTIDYCHECKFLAGS = -modernize-use-using,-performance-unnecessary-copy-initialization,-google-build-using-namespace

# We will use the same compiler for C and CPP files - choose the C and we will choose the CPP and options.
#CC := gcc
CC := clang
#CC := /usr/local/cuda/bin/nvcc

ifneq (1,$(NDEBUG))
# ASAN_OPTIONS=check_initialization_order=1:detect_leaks=1:detect_stack_use_after_return=1
# LSAN_OPTIONS=verbosity=1:log_threads=1
# CLANG_ADDR_SANITIZE = -fsanitize=address -fsanitize-address-use-after-scope

# # MSAN_OPTIONS=poison_in_dtor=1
# # ASAN_OPTIONS=detect_leaks=1
# # ASAN_OPTIONS=detect_stack_use_after_return=1
# #CLANG_MEM_SANITIZE = -fsanitize=memory -fsanitize-memory-track-origins -fsanitize-memory-use-after-dtor
# CLANGSANITIZE = $(CLANG_ADDR_SANITIZE) $(CLANG_MEM_SANITIZE) -fno-omit-frame-pointer
# #-fsanitize-blacklist=blacklist.txt 
endif # !NDEBUG

# I haven't noticed that tcmalloc has performed better - but I have noticed that it uses more memory and spends more time in the system - as opposed to user time.
# However it runs the dgraph unit testing much faster so I am going to use it.
LOCAL_MACHINE_OS = $(shell uname)
ifneq (Darwin,$(LOCAL_MACHINE_OS))
MOD_TCMALLOC := 1
MOD_LIBS := -luuid
else
MOD_CPPVER := -std=c++2a
endif
MOD_INCLUDES := -I./bienutil/

# We needed to install "sudo apt-get install uuid-dev" on linux.
MAKEBASE = ./bienutil/makebase.mk
include $(MAKEBASE)

SRCS = vebtest.cpp dbgthrw.cpp

ifeq (1,$(TIDY))
all: tidy
# The tidy build will invoke default rules to produce the *.tidy files.cd ..

tidy: $(patsubst %,$(BUILD_DIR)/%.tidy,$(basename $(SRCS)))
else
all: $(BUILD_DIR)/vebtest.exe
$(BUILD_DIR)/vebtest.exe: $(patsubst %,$(BUILD_DIR)/%.o,$(basename $(SRCS)))
	$(CXX) $(LINKFLAGS) -o $(BUILD_DIR)/vebtest.exe $(patsubst %,$(BUILD_DIR)/%.o,$(basename $(SRCS)))
include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))
endif
