CC = gcc
CFLAGS = -Wall -O2

SRCS = main.c
EXE = fbin

all: $(EXE)

$(EXE): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(EXE)

clean:
	rm -f $(EXE) *.o