BUILD_HOME=../..
include $(BUILD_HOME)/Options

SRCS1 = zdo_explore_zr.c
OBJS1 = ${SRCS1:.c=.o}

$(BIN)zdo_explore_zr$(EXE) : $(OBJS1)  $(LIBS)
	$(LINK_BINARY)

BINS = $(BIN)zdo_explore_zr$(EXE)
all: $(BINS)

#BINS_TEST='run_test.sh'
