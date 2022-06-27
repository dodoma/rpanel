APP = panel

PKGS:= hiredis opencv

C++ = g++

CFLAGS = -g -DMOS_LINUX -D_GNU_SOURCE

INCS  = -I deps/reef/include/ -I deps/reef/deps/mbedtls/include/
LIBS  = -L deps/reef/src -Wl,--whole-archive -lreef -Wl,--no-whole-archive -L deps/reef//deps/mbedtls/library/ -lmbedtls -lmbedx509 -lmbedcrypto -lbsd

INCS += $(shell pkg-config --cflags $(PKGS))
LIBS += $(shell pkg-config --libs $(PKGS))
LIBS += -ldl -lm -levent -lpthread

SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:.c=.o)
DEPEND	= .depend

all: $(APP)

$(DEPEND): $(SOURCES)
	@$(C++) $(CFLAGS) $(INCS) -MM $^ > $@

include $(DEPEND)
%.o: %.c
	$(C++) $(CFLAGS) $(INCS) -o $@ -c $<

panel: panel.o db.o eflat.o
	$(C++) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -f $(OBJECTS) $(APP)
