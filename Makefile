########################################
##COPYRIGHT: 2020. Stealthy Labs LLC
##DATE: 2020-01-15
##LICENSE: Refer LICENSE file.
########################################
PREFIX?=/usr/local
CC?=$(shell which gcc)
CXX?=$(shell which g++)
INSTALL?=$(shell which install)
CFLAGS_COMMON=-Wall -std=c99 -pedantic -posix -fPIC -D_DEFAULT_SOURCE -D_REENTRANT
CFLAGS_DEBUG=-g -O0
CFLAGS_RELEASE=-g -O
I2C_LIBS=-li2c
INC=-I$(PWD)
LDFLAGS=-L$(PWD)
SOFLAGS=-shared
EXEFLAGS=-Wl,-rpath,$(PWD)

TARGET_LIB=libssd1306_i2c.so
TARGET_EXE=test_ssd1306_i2c

LIB_OBJS=$(patsubst %.c,%.o,$(wildcard ssd1306_i2c*.c))
TARGET_HEADERS=$(wildcard ssd1306_i2c*.h)

ifeq ($(RELEASE),1)
 CFLAGS=$(CFLAGS_COMMON) $(CFLAGS_RELEASE)
else
 CFLAGS=$(CFLAGS_COMMON) $(CFLAGS_DEBUG)
endif

default: debug
	
debug: 
	$(MAKE) RELEASE=0 build

release:
	$(MAKE) RELEASE=1 build

build: $(TARGET_LIB) $(TARGET_EXE)

clean:
	rm -f $(TARGET_LIB) $(TARGET_EXE) *.o

install: build
	@$(INSTALL) -d $(PREFIX)/lib
	@$(INSTALL) -d $(PREFIX)/include
	$(INSTALL) -s -D $(TARGET_LIB) $(PREFIX)/lib
	$(INSTALL) -m 0644 -t $(PREFIX)/include $(TARGET_HEADERS)

$(TARGET_LIB): $(LIB_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOFLAGS) -o $@ $^ $(I2C_LIBS)

$(TARGET_EXE): $(TARGET_EXE).o $(TARGET_LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $(EXEFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $^

.PHONY: default clean debug release build install uninstall

