OUTCAT=./out/
TARGET=$(OUTCAT)a
MMCU?=atmega8
OPTIMIZE=-O2
AVRDUDE_FLAGS?=-v -p m8 -P /dev/ttyACM0 -c avr910



CROSS   ?=avr-
CC      :=$(CROSS)gcc
CXX     :=$(CROSS)g++
LD      :=$(CROSS)g++
SIZE    :=$(CROSS)size
OBJCOPY :=$(CROSS)objcopy
OBJDUMP :=$(CROSS)objdump


SOURCES= main.c ./Userlib/uart.c ./Userlib/MBSlave.c ./Userlib/eeprom.c
INCLUDES= . ./Userlib



OBJECTS=$(SOURCES:.cpp=.o)
OBJECTS:=$(OBJECTS:.c=.o)

CSTD?=gnu99
COPT=$(OPTIMIZE) -fdata-sections -ffunction-sections
CFLAGS=-mmcu=$(MMCU) -std=$(CSTD) $(COPT) -Wall
CFLAGS+=$(addprefix -I,$(INCLUDES))


CXXSTD?=c++98
CXXOPT=$(COPT) -fno-exceptions -fno-rtti
CXXFLAGS=-mmcu=$(MMCU) -std=$(CXXSTD) $(CXXOPT) -Wall
CXXFLAGS+=$(addprefix -I,$(INCLUDES))


LDFLAGS=-mmcu=$(MMCU) -Wl,--gc-sections -Wl,-Map=$(TARGET).map,--cref

.PHONY: all avrdude clean
all: $(TARGET).hex $(TARGET).lst clean_o

$(TARGET).elf: $(OBJECTS)
	$(LD) $(LDFLAGS) $(INCLUDES_FLAFS) $^ -lm -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock -R .signature $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary -R .eeprom -R .fuse -R .lock -R .signature $< $@

%.o: %.cpp
	$(CXX) -o $@ $(CXXFLAGS) $(INCLUDES_FLAFS) -MMD -MP -MF $(@:%.o=%.d) $< -c

%.o: %.c
	$(CC) -o $@ $(CFLAGS) $(INCLUDES_FLAFS) -MMD -MP -MF $(@:%.o=%.d) $< -c

$(TARGET).lst: $(TARGET).elf
	$(OBJDUMP) -h -S $< > $@

avrdude:
	avrdude $(AVRDUDE_FLAGS) -e -U flash:w:$(TARGET).hex:i  

clean:
	-rm -f $(addprefix $(TARGET), .elf .hex .bin .lst .map)
	-rm -f $(OBJECTS) $(OBJECTS:.o=.d)

clean_o:
	-rm -f $(OBJECTS) $(OBJECTS:.o=.d)

eepromread:
	avrdude $(AVRDUDE_FLAGS) -Ueeprom:r:-:i

write_fuse:
	avrdude $(AVRDUDE_FLAGS) -U lfuse:w:0xFF:m
	avrdude $(AVRDUDE_FLAGS) -U hfuse:w:0xC9:m

