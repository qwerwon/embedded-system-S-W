OBJECTS = main.o input.o output.o
SRCS = main.c input.c output.c

CC = arm-none-linux-gnueabi-gcc -static
CFLAGS = -c

TARGET = 20121600

$(TARGET) : $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) -Wall

clean:
	rm -rf $(OBJECTS) $(TARGET)
