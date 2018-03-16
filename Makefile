# A simple Makefile, to build run: make all 
TARGET	= spacerm

CC	= gcc
#compiler flags here
CFLAGS = -std=c99 -pedantic -O3 -Wall

#linker flags here
LFLAGS = -Wall

SRCDIR	= src

SOURCES	:= $(wildcard $(SRCDIR)/*.c)
INCLUDES	:= $(wildcard $(SRCDIR)/*.h))
OBJECTS	:= $(SOURCES:$(SRCDIR)/%.c=$(SRCDIR)/%.o)

.PHONY: all clean remove
all: ${TARGET}

$(TARGET): $(OBJECTS)
	@$(CC) -o $@ $(LFLAGS) $(OBJECTS)

$(OBJECTS): $(SRCDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@$ rm -f $(OBJECTS)

remove: clean
	@$ rm -f $(TARGET)
