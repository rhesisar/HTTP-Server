MAIN = http-server
SRC_C = $(wildcard src/*.c) \
        ../parser/src/api.c \
        ../parser/src/syntax.c \
        ../parser/src/tree.c
CFLAGS = -Wall -g -O0

# Include paths
IFLAGS = -I api/librequest-0.5/ \
         -I /usr/local/include \
         -I /opt/homebrew/include

# Library paths + libs
LFLAGS = -L /usr/local/lib \
         -L /opt/homebrew/lib \
         -lm -lmagic

$(MAIN): $(SRC_C)
	gcc $^ -o $@ $(CFLAGS) $(IFLAGS) $(LFLAGS)

run:
	./$(MAIN) &

stop:
	@echo "Stopping HTTP server..."
	pkill http-server
	@echo "Done"

re: clean $(MAIN) run

clean:
	rm -rf $(MAIN) src/*~ src/*.swap
