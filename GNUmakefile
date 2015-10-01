PROGRAM = MouseWinX.exe
SRCS = MouseWinX.c
RCS = MouseWinX.rc

OBJS = $(SRCS:.c=.o)
COFFS = $(RCS:.rc=.coff)
DEPS = $(OBJS:.o=.d)

.PHONY: clean all

all: $(PROGRAM)

clean:
	rm -f $(PROGRAM) $(OBJS) $(COFFS) $(DEPS)

$(PROGRAM): $(OBJS) $(COFFS)
	$(CC) $(LDFLAGS) $(LOADLIBES) $^ $(LDLIBS) -o $@

WINDRES ?= windres
%.coff %.res : %.rc
	$(WINDRES) -i $< -o $@

CFLAGS = -O2 -Werror -Wall -MMD
LDFLAGS = -mwindows

-include $(DEPS)
