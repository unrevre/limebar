CC	= clang
CFLAGS	= -O2
FRAMES	= -F/System/Library/PrivateFrameworks \
	  -framework SkyLight \
	  -framework Carbon \
	  -framework IOKit

BINDIR	= ./bin

all:	mkdir
	$(CC) ./src/limebar.c $(CFLAGS) -o $(BINDIR)/limebar $(FRAMES)

mkdir:
	mkdir -p $(BINDIR)

clean:
	rm -rf $(BINDIR)
