CXX ?= g++
PKG_CONFIG ?= pkg-config

TARGET ?= build/gb_internovel_sdl

APP_SRCS := \
  src/main.cpp \
  src/app_context.cpp \
  src/app_config.cpp

CORE_SRCS := \
  src/layout_metrics.cpp \
  src/input_manager.cpp \
  src/scene_manager.cpp \
  src/ui_draw.cpp

SCENE_SRCS := \
  src/scenes/boot_scene.cpp \
  src/scenes/home_scene.cpp \
  src/scenes/settings_scene.cpp \
  src/scenes/session_scene.cpp

BUSINESS_SRCS := \
  src/business/ai_client.cpp \
  src/business/story_session.cpp

SRCS := \
  $(APP_SRCS) \
  $(CORE_SRCS) \
  $(SCENE_SRCS) \
  $(BUSINESS_SRCS)

OBJS := $(SRCS:.cpp=.o)

SDL_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags sdl2 2>/dev/null)
SDL_LIBS ?= $(shell $(PKG_CONFIG) --libs sdl2 2>/dev/null)
ifeq ($(strip $(SDL_LIBS)),)
SDL_LIBS := -lSDL2
endif

CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic
CXXFLAGS += $(SDL_CFLAGS) -I./src
CXXFLAGS += -pthread
LDFLAGS += $(SDL_LIBS)
LDFLAGS += -pthread

TTF_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags SDL2_ttf 2>/dev/null)
TTF_LIBS ?= $(shell $(PKG_CONFIG) --libs SDL2_ttf 2>/dev/null)
ifneq ($(strip $(TTF_LIBS)),)
CXXFLAGS += -DHAVE_SDL2_TTF $(TTF_CFLAGS)
LDFLAGS += $(TTF_LIBS)
endif

.PHONY: all clean run print-config

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

print-config:
	@echo "CXX=$(CXX)"
	@echo "PKG_CONFIG=$(PKG_CONFIG)"
	@echo "TARGET=$(TARGET)"
	@echo "SDL_CFLAGS=$(SDL_CFLAGS)"
	@echo "SDL_LIBS=$(SDL_LIBS)"
	@echo "TTF_CFLAGS=$(TTF_CFLAGS)"
	@echo "TTF_LIBS=$(TTF_LIBS)"
	@echo "CXXFLAGS=$(CXXFLAGS)"
	@echo "LDFLAGS=$(LDFLAGS)"

clean:
	rm -f $(OBJS) $(TARGET)
