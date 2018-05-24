BUILD_HOME=../..
include $(BUILD_HOME)/Options

FNAME1 = zdo_bulb_zc
FNAME2 = zdo_bulb_zr

SRCS1 = $(FNAME1).c led.c
OBJS1 = ${SRCS1:.c=.o}
SRCS2 = $(FNAME2).c led.c
OBJS2 = ${SRCS2:.c=.o}

$(BIN)zdo_bulb_zc$(EXE) : $(OBJS1)  $(LIBS)
	$(LINK_BINARY)
	$(OBJCOPY) -O ihex $(FNAME1) $(FNAME1).hex
	$(OBJCOPY) -I ihex -O binary $(FNAME1).hex $(FNAME1).bin

$(BIN)zdo_bulb_zr$(EXE) : $(OBJS2)  $(LIBS)
	$(LINK_BINARY)
	$(OBJCOPY) -O ihex $(FNAME2) $(FNAME2).hex
	$(OBJCOPY) -I ihex -O binary $(FNAME2).hex $(FNAME2).bin


BINS = $(BIN)zdo_bulb_zc$(EXE) $(BIN)zdo_bulb_zr$(EXE)
all: $(BINS)

#BINS_TEST='run_test.sh'

burn_$(FNAME1): $(FNAME1).bin
	st-flash write $(FNAME1).bin 0x08000000

burn_$(FNAME2): $(FNAME2).bin
	st-flash write $(FNAME2).bin 0x08000000

