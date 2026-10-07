.RECIPEPREFIX = >

CC ?= gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra
SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS := $(shell sdl2-config --libs)

TARGET := ray
ifeq ($(OS),Windows_NT)
TARGET := ray.exe
endif

.PHONY: all clean run

all: $(TARGET)

$(TARGET): src/ray.c
> $(CC) $(CFLAGS) $(SDL_CFLAGS) $< -o $@ $(SDL_LIBS) -lm

run: $(TARGET)
> ./$(TARGET)

clean:
> rm -f ray ray.exe
