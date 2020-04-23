TARGET = simulator
LIBS = -lmonome -lcsound64
CC = clang
CFLAGS = -g -Wall -Wextra -Wshadow -Wdouble-promotion -Wundef -Wconversion -fno-common -I../../app

.PHONY: default all clean simple_trigger_test

include ../../app/app.mk

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(addprefix ../../app/,$(APP_CSRCS))) simulator.o orchestras.o
HEADERS = $(addprefix ../../app/,$(APP_CSRCS)) orchestras.h
XXDS = simple_trigger.xxd

%.o: %.c $(HEADERS) $(XXDS)
	$(CC) $(CFLAGS) -c $< -o $@

%.xxd: %.orc
	xxd -i < $< > $@
	echo ', 0x00' >> $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f *.d
	-rm -f *.xxd
	-rm -f ../../app/*.o
	-rm -f ../../app/*.d
	-rm -f $(TARGET)

simple_trigger_test:
	csound -+rtaudio=pulse -odac simple_trigger.orc simple_trigger.sco
