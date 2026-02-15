# wav_modulator: 比特流 -> FSK 调制 -> WAV 文件
# 依赖上级目录的 include/ 和 src/modem.c

CC = gcc
CFLAGS = -Wall -Wextra -I.. -I../include
LDFLAGS = -lm

BIN = bits_to_wav
OBJS = bits_to_wav.o wav_writer.o modem.o

all: $(BIN)

modem.o: ../src/modem.c ../include/modem.h ../include/common.h
	$(CC) $(CFLAGS) -c -o $@ ../src/modem.c

bits_to_wav.o: bits_to_wav.c ../include/modem.h ../include/common.h wav_writer.h
	$(CC) $(CFLAGS) -c -o $@ bits_to_wav.c

wav_writer.o: wav_writer.c wav_writer.h ../include/common.h
	$(CC) $(CFLAGS) -c -o $@ wav_writer.c

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(BIN)

.PHONY: all clean test

test: $(BIN)
	./$(BIN) --test
	@echo "Generated output/test.wav — play with: aplay output/test.wav (or any player)"
