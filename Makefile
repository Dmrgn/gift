CC      := gcc
CFLAGS  := -Wall -Wextra -O2 $(shell pkg-config --cflags sdl3 libcurl)
LDFLAGS := $(shell pkg-config --libs sdl3 libcurl) -lm

SRC := src/main.c src/config.c src/audio.c src/wav.c src/base64.c src/api.c src/vendor/cJSON.c
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))

TARGET := gift

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c | build
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

build:
	mkdir -p build build/vendor

install: $(TARGET)
	mkdir -p ~/.config/gift
	cp config.ini ~/.config/gift/config.ini
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)/usr/local/bin/$(TARGET)

clean:
	rm -rf build $(TARGET)

.PHONY: all clean install uninstall
