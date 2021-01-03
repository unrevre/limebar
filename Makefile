CC	= clang
CFLAGS	= -O2 -flto
FRAMES	= -F/System/Library/PrivateFrameworks
LDFLAGS	= -framework Carbon -framework SkyLight -framework IOKit

BINDIR	= ./bin
BLDDIR	= ./build
SRCDIR	= ./src

SRCS	= $(filter-out $(SRCDIR)/limebar.c,$(wildcard $(SRCDIR)/*.c))
DEPS	= $(patsubst $(SRCDIR)/%.c,$(BLDDIR)/%.d,$(SRCS))
OBJS	= $(patsubst $(SRCDIR)/%.c,$(BLDDIR)/%.o,$(SRCS))

all:	mkdir $(OBJS) $(BINDIR)/limebar

$(BLDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(FRAMES) -MMD -c $< -o $@

$(BINDIR)/limebar: $(SRCDIR)/limebar.c $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(FRAMES) $^ -o $@

mkdir:
	mkdir -p $(BINDIR) $(BLDDIR)

clean:
	rm -rf $(BINDIR) $(BLDDIR)

.PHONY:	all mkdir clean

-include $(DEPS)
