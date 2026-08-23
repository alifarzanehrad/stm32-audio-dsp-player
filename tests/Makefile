CC ?= cc

CPPFLAGS := -I./support -I../Core/Inc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -pedantic
LDLIBS := -lm

SOURCES := \
	test_dsp.c \
	support/dsp_stubs.c \
	../Core/Src/audio_equalizer.c \
	../Core/Src/audio_pipeline.c

TARGET := build/test_dsp

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) $(LDLIBS) -o $@

test: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build
