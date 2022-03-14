-include Makefile.inc
BASE_DIR=$(shell pwd)
SRC_DIR=$(BASE_DIR)/src
BUILD_DIR?=$(BASE_DIR)/build
INCLUDE_DIR=$(BASE_DIR)/include
BUILDIT_DIR?=$(BASE_DIR)/buildit


INCLUDES=$(wildcard $(INCLUDE_DIR)/*.h) $(wildcard $(INCLUDE_DIR)/*/*.h) $(wildcard $(BUILDIT_DIR)/include/*.h) $(wildcard $(BUILDIT_DIR)/include/*/*.h)

INCLUDE_FLAG=-I$(INCLUDE_DIR) -I$(BUILDIT_DIR)/include

SRCS=$(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp)
OBJS=$(subst $(SRC_DIR),$(BUILD_DIR),$(SRCS:.cpp=.o))


$(shell mkdir -p $(BUILD_DIR))
$(shell mkdir -p $(BUILD_DIR)/core)
$(shell mkdir -p $(BUILD_DIR)/impls)
$(shell mkdir -p $(BUILD_DIR)/modules)

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

clean:
	rm -rf $(BUILD_DIR)
