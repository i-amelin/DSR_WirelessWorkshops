BUILD_HOME=../..
include $(BUILD_HOME)/Options

SRCS1 = zdo_bulb_zc.c
OBJS1 = ${SRCS1:.c=.o}
SRCS2 = zdo_bulb_zr.c
OBJS2 = ${SRCS2:.c=.o}

$(BIN)zdo_bulb_zc$(EXE) : $(OBJS1)  $(LIBS)
	$(LINK_BINARY)

$(BIN)zdo_bulb_zr$(EXE) : $(OBJS2)  $(LIBS)
	$(LINK_BINARY)

#$(BIN)zdo_start_zc_apss$(EXE) : $(SRCS1)  $(LIBS)
#	$(CC) $(CFLAGS) -DAPS_SECUR  $(DEFINES) $(INCPATH) -o $@ $< $(LIBS) $(LIBS)


BINS = $(BIN)zdo_bulb_zc$(EXE) $(BIN)zdo_bulb_zr$(EXE)
all: $(BINS)

#BINS_TEST='run_test.sh'
