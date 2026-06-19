CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -Isrc
LDFLAGS = -lsqlite3

SRC     = main.c mongoose.c src/db.c src/agent.c src/call.c src/dispatcher.c src/report.c src/routes.c
TARGET  = dispatcher

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
