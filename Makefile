CC = gcc
CFLAGS = -Wall #-Wextra #-Werror
LDFLAGS = -lrt -pthread


all:simulator manager
	echo "Compiled Complete"

simulator.o: resources/generatePlate.h resources/hashTable.h resources/queue.h resources/shared_mem.h
manager.o: resources/generatePlate.h resources/hashTable.h resources/queue.h resources/shared_mem.h
fire_alarm_new.o: resources/generatePlate.h resources/hashTable.h resources/queue.h resources/shared_mem.h

simulator: simulator.o
manager: manager.o
fire_alarm_new: fire_alarm_new.o

clean:
	rm -f simulator
	rm -f manager
	rm -f fire_alarm_new
	rm -f *.o

.PHONY: all clean