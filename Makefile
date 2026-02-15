# IP over Sound - 自动化编译
# 依赖: libportaudio-dev (Ubuntu: sudo apt install libportaudio2 libportaudiocpp0 libportaudio-dev)
# 若已安装，可用 pkg-config 获取 PortAudio 编译/链接选项

CC     = gcc
CFLAGS = -Wall -Wextra -g -I include $(shell pkg-config --cflags portaudio-2.0 2>/dev/null || echo "")
LDFLAGS = -lpthread -lm $(shell pkg-config --libs portaudio-2.0 2>/dev/null || echo "-lportaudio")

SRC = src/main.c src/tun_dev.c src/audio_dev.c src/modem.c src/protocol.c src/utils.c
OBJ = $(SRC:.c=.o)
TARGET = ipo_sound

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)
