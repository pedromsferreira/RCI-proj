CC	 = gcc
CFLAGS	 = -g -Wall
LFLAGS =
OBJFILES = validation.o network.o main.o 
OUT = ndn

all: $(OUT)

$(OUT): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(OUT) $(OBJFILES) $(LFLAGS)

clean: 
	rm -f $(OBJFILES) $(OUT) 

valgrind: $(OUT)
	valgrind $(OUT)

valgrind_leakcheck: $(OUT)
	valgrind --leak-check=full $(OUT)

valgrind_extreme: $(OUT)
	valgrind --leak-check=full --show-leak-kinds=all --leak-resolution=high --track-origins=yes --vgdb=yes $(OUT)

