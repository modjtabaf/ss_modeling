CXX      := -c++
CXXFLAGS := -pedantic-errors -Wall -Wextra -Werror -std=c++17
LDFLAGS  := -L/usr/lib -lstdc++ -lm  -lboost_iostreams -lboost_system -lboost_filesystem
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/apps
TARGET   := mass_spring
INCLUDE  := # -Iinclude/
SRC      :=        \
	mass_spring.cpp \
	blocks.cpp     \
	helper.cpp     \
	solver.cpp
#    $(wildcard src/module1/*.cpp) \
#    $(wildcard src/module2/*.cpp) \
#    $(wildcard src/*.cpp)         \

OBJECTS  := $(SRC:%.cpp=$(OBJ_DIR)/%.o)
DEPENDENCIES \
         := $(OBJECTS:.o=.d)

all: build $(APP_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -MMD -o $@

$(APP_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $(APP_DIR)/$(TARGET) $^ $(LDFLAGS)

-include $(DEPENDENCIES)

.PHONY: all build clean debug release run info mass_spring

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -O2
release: all

# mass_spring: SRC += mass_spring.cpp
# mass_spring: TARGET += mass_spring
# mass_spring: release

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*

run:
	@$(APP_DIR)/$(TARGET)

info:
	@echo "[*] Application dir: ${APP_DIR}     "
	@echo "[*] Object dir:      ${OBJ_DIR}     "
	@echo "[*] Sources:         ${SRC}         "
	@echo "[*] Objects:         ${OBJECTS}     "
	@echo "[*] Dependencies:    ${DEPENDENCIES}"