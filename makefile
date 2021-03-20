CC	 = gcc
CFLAGS	 = -Wall
LFLAGS =
OBJFILES = validation.o main.o 
TARGET = npn

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LFLAGS)

clean:
	rm -f $(OBJFILES) $(TARGET) *~
