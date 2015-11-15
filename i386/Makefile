CC = g++
OBJS = bridge.o usart1class.o cdclass.o

all: bridge

bridge: $(OBJS)
	$(CC) $(OBJS) -o bridge -lpthread
%.o: %.cpp
	$(CC) -c -Os -I../inc -I../lpc11 $< -o $@
clean:
	rm -f *.o