SNIPER_BASE_DIR ?= /home/nick/arch-ntua/ex03/sniper-7.3
SNIPER_INCLUDE_DIR = $(SNIPER_BASE_DIR)/include

CC = gcc

TARGET = locks

IMPLFLAG ?= -DREAL ## -DSNIPER or -DREAL
LFLAG ?= -DTTAS_TS ## -DMUTEX or -DTAS_CAS or -DTAS_TS or -DTTAS_CAS or DTTAS_TS

CFLAGS ?= -Wall -O0 -lpthread $(IMPLFLAG) $(LFLAG) #-DDEBUG
CFLAGS += -I$(SNIPER_INCLUDE_DIR)
CFLAGS += -Wno-unused-variable

$(TARGET): locks_scalability.c
	$(CC) $^ $(HOOKS_LDFLAGS) $(CFLAGS) -o $@

clean :
	rm -f $(TARGET)
