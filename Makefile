CC = gcc
CFLAGS = -Wall -Wextra -ggdb -I./include/ -MMD -MP -fsanitize=address
LDFLAGS = -lraylib -lm
TARGET = build/main.out

SRC = $(wildcard sigmidi/*.c renderer/*.c)
OBJS = $(patsubst %.c, build/%.o, $(SRC))
DEPS = $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

build/%.o: %.c
	@mkdir -p build/sigmidi build/renderer
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build $(TARGET)
