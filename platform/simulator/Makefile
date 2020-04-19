TARGET = simulator
LIBS = -lmonome
CC = clang
CFLAGS = -g -Wall -Wextra -Wshadow -Wdouble-promotion -Wundef -Wconversion -fno-common -I../../app

.PHONY: default all clean

include ../../app/app.mk

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(addprefix ../../app/,$(APP_CSRCS))) simulator.o
HEADERS = $(addprefix ../../app/,$(APP_CSRCS))

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f *.d
	-rm -f ../../app/*.o
	-rm -f ../../app/*.d
	-rm -f $(TARGET)
