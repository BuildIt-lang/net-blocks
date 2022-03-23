-include Makefile.inc
BASE_DIR=$(shell pwd)
SRC_DIR=$(BASE_DIR)/src
BUILD_DIR?=$(BASE_DIR)/build
RUNTIME_DIR?=$(BASE_DIR)/runtime
SCRATCH_DIR?=$(BASE_DIR)/scratch
TEST_DIR?=$(BASE_DIR)/test
INCLUDE_DIR=$(BASE_DIR)/include
BUILDIT_DIR?=$(BASE_DIR)/buildit

INCLUDES=$(wildcard $(INCLUDE_DIR)/*.h) $(wildcard $(INCLUDE_DIR)/*/*.h) $(wildcard $(BUILDIT_DIR)/include/*.h) $(wildcard $(BUILDIT_DIR)/include/*/*.h)

INCLUDE_FLAG=-I$(INCLUDE_DIR) -I$(BUILDIT_DIR)/include

SRCS=$(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp)
OBJS=$(subst $(SRC_DIR),$(BUILD_DIR),$(SRCS:.cpp=.o))


$(shell mkdir -p $(SCRATCH_DIR))
$(shell mkdir -p $(BUILD_DIR))
$(shell mkdir -p $(BUILD_DIR)/core)
$(shell mkdir -p $(BUILD_DIR)/impls)
$(shell mkdir -p $(BUILD_DIR)/modules)
$(shell mkdir -p $(BUILD_DIR)/runtime)
$(shell mkdir -p $(BUILD_DIR)/test)
$(shell mkdir -p $(BUILD_DIR)/test/simple_network_test)
$(shell mkdir -p $(BUILD_DIR)/test/simple_test)
$(shell mkdir -p $(BUILD_DIR)/runtime/mlx5_impl)

BUILDIT_LIBRARY_NAME=buildit
BUILDIT_LIBRARY_PATH=$(BUILDIT_DIR)/build

LIBRARY_NAME=net_blocks
DEBUG ?= 0
ifeq ($(DEBUG),1)
CFLAGS=-g -std=c++11 -O0
LINKER_FLAGS=-rdynamic  -g -L$(BUILDIT_LIBRARY_PATH) -L$(BUILD_DIR) -l$(LIBRARY_NAME) -l$(BUILDIT_LIBRARY_NAME)
else
CFLAGS=-std=c++11 -O3
LINKER_FLAGS=-rdynamic  -L$(BUILDIT_LIBRARY_PATH) -L$(BUILD_DIR) -l$(LIBRARY_NAME) -l$(BUILDIT_LIBRARY_NAME)
endif



LIBRARY=$(BUILD_DIR)/lib$(LIBRARY_NAME).a

CFLAGS+=-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wmissing-declarations -Woverloaded-virtual -pedantic-errors -Wno-deprecated -Wdelete-non-virtual-dtor -Werror

all: executables $(LIBRARY)

.PHONY: dep
dep:
	make -C $(BUILDIT_DIR)

.PRECIOUS: $(BUILD_DIR)/core/%.o
.PRECIOUS: $(BUILD_DIR)/modules/%.o
.PRECIOUS: $(BUILD_DIR)/impls/%.o


.PHONY: $(BUILDIT_LIBRARY_PATH)/lib$(BUILDIT_LIBRARY_NAME).a


.PHONY: executables
executables: $(BUILD_DIR)/impls/simple


$(LIBRARY): $(OBJS)
	ar rv $(LIBRARY) $(OBJS)	
	
$(BUILD_DIR)/core/%.o: $(SRC_DIR)/core/%.cpp $(INCLUDES)
	$(CXX) $(CFLAGS) $< -o $@ $(INCLUDE_FLAG) -c
$(BUILD_DIR)/modules/%.o: $(SRC_DIR)/modules/%.cpp $(INCLUDES)
	$(CXX) $(CFLAGS) $< -o $@ $(INCLUDE_FLAG) -c
$(BUILD_DIR)/impls/%.o: $(SRC_DIR)/impls/%.cpp $(INCLUDES)
	$(CXX) $(CFLAGS) $< -o $@ $(INCLUDE_FLAG) -c

$(BUILD_DIR)/impls/simple: $(BUILD_DIR)/impls/simple.o $(LIBRARY) dep
	$(CXX) -o $@ $< $(LINKER_FLAGS)


# Runtime objs
$(BUILD_DIR)/runtime/nb_simple.o: $(BUILD_DIR)/impls/simple $(RUNTIME_DIR)/nb_runtime.h
	$(BUILD_DIR)/impls/simple > $(SCRATCH_DIR)/nb_simple.c
	$(CC) $(SCRATCH_DIR)/nb_simple.c -o $(BUILD_DIR)/runtime/nb_simple.o -c -I $(RUNTIME_DIR)
	

$(BUILD_DIR)/runtime/nb_runtime.o: $(RUNTIME_DIR)/nb_runtime.c $(RUNTIME_DIR)/nb_runtime.h
	$(CC) -o $@ $< -c	


$(BUILD_DIR)/runtime/nb_mlx5_transport.o: $(RUNTIME_DIR)/nb_mlx5_transport.cc $(RUNTIME_DIR)/nb_runtime.h
	$(CXX) -c $(RUNTIME_DIR)/nb_mlx5_transport.cc -o $(BUILD_DIR)/runtime/nb_mlx5_transport.o -I $(RUNTIME_DIR)/mlx5_impl/

.PRECIOUS: $(BUILD_DIR)/runtime/mlx5_impl/%.o
$(BUILD_DIR)/runtime/mlx5_impl/%.o: $(RUNTIME_DIR)/mlx5_impl/%.cc
	$(CXX) $< -o $@ -c -I $(RUNTIME_DIR)/mlx5_impl/

mlx5_runtime: $(BUILD_DIR)/runtime/nb_mlx5_transport.o $(BUILD_DIR)/runtime/mlx5_impl/transport.o $(BUILD_DIR)/runtime/mlx5_impl/halloc.o
		
.PHONY: simple_network_test
simple_network_test: mlx5_runtime $(BUILD_DIR)/runtime/nb_simple.o $(BUILD_DIR)/runtime/nb_runtime.o
	$(CC) -c $(TEST_DIR)/test_mlx5_simple/server.c -o $(BUILD_DIR)/test/simple_network_test/server.o -I $(RUNTIME_DIR)
	$(CC) -c $(TEST_DIR)/test_mlx5_simple/client.c -o $(BUILD_DIR)/test/simple_network_test/client.o -I $(RUNTIME_DIR)
	$(CXX) $(BUILD_DIR)/runtime/nb_mlx5_transport.o $(BUILD_DIR)/runtime/mlx5_impl/halloc.o $(BUILD_DIR)/runtime/mlx5_impl/transport.o $(BUILD_DIR)/runtime/nb_runtime.o $(BUILD_DIR)/runtime/nb_simple.o $(BUILD_DIR)/test/simple_network_test/server.o -o $(BUILD_DIR)/test/network_simple_server -libverbs
	$(CXX) $(BUILD_DIR)/runtime/nb_mlx5_transport.o $(BUILD_DIR)/runtime/mlx5_impl/halloc.o $(BUILD_DIR)/runtime/mlx5_impl/transport.o $(BUILD_DIR)/runtime/nb_runtime.o $(BUILD_DIR)/runtime/nb_simple.o $(BUILD_DIR)/test/simple_network_test/client.o -o $(BUILD_DIR)/test/network_simple_client -libverbs

	
.PHONY: simple_test
simple_test: executables $(BUILD_DIR)/runtime/nb_simple.o $(BUILD_DIR)/runtime/nb_runtime.o
	$(CC) -c $(TEST_DIR)/test_simple/server.c -o $(BUILD_DIR)/test/simple_test/server.o -I $(RUNTIME_DIR)
	$(CC) -c $(TEST_DIR)/test_simple/client.c -o $(BUILD_DIR)/test/simple_test/client.o -I $(RUNTIME_DIR)
	$(CC) -c $(RUNTIME_DIR)/nb_ipc_transport.c -o $(BUILD_DIR)/runtime/nb_ipc_transport.o -I $(RUNTIME_DIR)
	$(CC) $(BUILD_DIR)/runtime/nb_runtime.o $(BUILD_DIR)/runtime/nb_simple.o $(BUILD_DIR)/test/simple_test/server.o $(BUILD_DIR)/runtime/nb_ipc_transport.o -o $(BUILD_DIR)/test/simple_server
	$(CC) $(BUILD_DIR)/runtime/nb_runtime.o $(BUILD_DIR)/runtime/nb_simple.o $(BUILD_DIR)/test/simple_test/client.o $(BUILD_DIR)/runtime/nb_ipc_transport.o -o $(BUILD_DIR)/test/simple_client


clean:
	rm -rf $(BUILD_DIR)
