#COMMONOPT = -g -DTICK_FREQ=1000 -DISR_FREQ=2000 -DTIMER_ISR=SIG_OUTPUT_COMPARE2 \
#						-DTIMER_APP_CALL=timerAppCall
MMCU = -mmcu=atmega16

COMMONOPT = $(MMCU) -DF_CPU=3579545lu -DDEBUG

ASFLAGS = $(COMMONOPT) -D__ASM__ -Wall
CFLAGS = $(COMMONOPT) -Os -Wall -Wa,-ahlms=$(<:.c=.lst)
LDFLAGS = $(MMCU) -Wall -Wl,-Map=$(MAIN_SRCFILE:.c=.map),--cref

MAIN_SRCFILE = main.c
C_SRCFILES = $(MAIN_SRCFILE) uart.c str.c test.c display.c io.c
# ASM_SRCFILES = rtk.S timer.S




INCDIR = include
CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

OBJFILES = $(C_SRCFILES:.c=.o) $(ASM_SRCFILES:.S=.o)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.S
	$(CC) -c $(ASFLAGS) $< -o $@

$(MAIN_SRCFILE:.c=.hex): $(MAIN_SRCFILE:.c=.out)
	$(OBJCOPY) -O ihex $< $@
	avr-nm -n -l -S $< > $(MAIN_SRCFILE:.c=.nm)

$(MAIN_SRCFILE:.c=.out): $(OBJFILES)
	$(CC) $(LDFLAGS) $(OBJFILES) -o $@


all: $(MAIN_SRCFILE:.c=.hex)

burn: $(MAIN_SRCFILE:.c=.hex)
	$(AVRDUDE) -p m16 -P usb -c avrispmkII -B 4 -U flash:w:$(MAIN_SRCFILE:.c=.hex):i
	../mem.sh
# avrdude -p m32 -P /dev/cu.usbserial-000043FA -U flash:w:main.hex:i -b 115200 -c avr109
#	$(AVRDUDE) -p m128 -P /dev/cu.usbserial-FTC8C8IK -c stk500v2 -U flash:w:$(MAIN_SRCFILE:.c=.hex):i


clean:
	rm -f $(OBJFILES)

