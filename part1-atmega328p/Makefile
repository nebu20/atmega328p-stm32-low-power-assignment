MCU = atmega328p
F_CPU = 16000000UL
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size

CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -O2 -Wall -I. -Isrc
LDFLAGS = -mmcu=$(MCU)

SRC = src/main.c src/atmega328p_lowpower.c
OBJ = $(SRC:.c=.o)
TARGET = build/main

all: $(TARGET).hex

build:
	mkdir -p build

$(TARGET).elf: build $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@
	$(SIZE) --format=avr --mcu=$(MCU) $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

clean:
	rm -rf build src/*.o

.PHONY: all clean build
