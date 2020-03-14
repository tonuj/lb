#COMMONOPT = -g -DTICK_FREQ=1000 -DISR_FREQ=2000 -DTIMER_ISR=SIG_OUTPUT_COMPARE2 \
#						-DTIMER_APP_CALL=timerAppCall
MMCU = -mmcu=atmega128

COMMONOPT = $(MMCU) -DF_CPU=7372800lu -DDEBUG
#COMMONOPT = $(MMCU) -DF_CPU=7372800lu -DBOOTLOADER

#  -DPRODUCT_W4A
# -DTELNET_VERSION
ASFLAGS = $(COMMONOPT) -D__ASM__ -Wall
CFLAGS = $(COMMONOPT) -Os -Wall -Wa,-ahlms=$(<:.c=.lst)
LDFLAGS = $(MMCU) -Wall -Wl,-Map=$(MAIN_SRCFILE:.c=.map),--cref

MAIN_SRCFILE = main.c
C_SRCFILES = $(MAIN_SRCFILE) uart.c ringbuf.c gps.c gsm.c ch.c inputs.c str.c sch.c timer.c cmd.c outputs.c mkx.c log.c event.c spi.c flash.c param.c eeprom.c 
# ASM_SRCFILES = rtk.S timer.S




# INCDIR = include
CC = /opt/local/bin/avr-gcc
OBJCOPY = /opt/local/bin/avr-objcopy
AVRDUDE = /opt/local/bin/avrdude
NM = /opt/local/bin/avr-nm

OBJFILES = $(C_SRCFILES:.c=.o) $(ASM_SRCFILES:.S=.o)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.S
	$(CC) -c $(ASFLAGS) $< -o $@

$(MAIN_SRCFILE:.c=.hex): $(MAIN_SRCFILE:.c=.out)
	$(OBJCOPY) -O ihex $< $@
	$(NM) -n -l -S $< > $(MAIN_SRCFILE:.c=.nm)
	/opt/local/bin/avr-size -A main.out


$(MAIN_SRCFILE:.c=.out): $(OBJFILES)
	$(CC) $(LDFLAGS) $(OBJFILES) -o $@


all: $(MAIN_SRCFILE:.c=.hex)

burn: $(MAIN_SRCFILE:.c=.hex)
#	$(AVRDUDE) -p m128 -P usb -c avrispmkII -B 2 -U flash:w:$(MAIN_SRCFILE:.c=.hex):i
#	/opt/local/bin/avrdude -p m128 -P usb -c avrispmkII -B 2 -U eeprom:w:starman_emt.raw:r
	avrdude -p m128 -P /dev/cu.usbserial -c jtag1 -b 115200 -U flash:w:main.hex:i


	./mem.sh

# avrdude -p m32 -P /dev/cu.usbserial-000043FA -c avr109 -b 115200 -U eeprom:w:navirec_emt.raw:r -U flash:w:main.hex:i


clean:
	rm -f $(OBJFILES)

