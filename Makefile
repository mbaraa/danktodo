.PHONY: setup build

CC=gcc

CFLAGS+=-Wall -Wmissing-declarations -Wshadow
CFLAGS+=-Wstrict-prototypes -Wmissing-prototypes
CFLAGS+=-Wpointer-arith -Wcast-qual -Wsign-compare
CFLAGS+=-lsqlite3 -lulfius -lctemplate -lcrypt -ljwt -lm
LDFLAGS+=-lsqlite3 -lulfius -lctemplate -lcrypt -ljwt -lm

BINARY_NAMR=todooo

all: setup build

objects:
	$(CC) $(CFLAGS) -c ./src/*.c $(LDFLAGS)

build: objects
	$(CC) $(CFLAGS) *.o ./main.c -o $(BINARY_NAMR) $(LDFLAGS)

dev: clean build tailwindcss-build
	./todooo

prod: setup build tailwindcss-build
	./todooo

setup: tailwindcss-init
	mkdir -p assets/js/htmx &&\
	mkdir -p assets/css &&\
	wget https://unpkg.com/htmx-ext-loading-states@2.0.0/loading-states.js -O assets/js/htmx/loading-states.js &&\
	wget https://unpkg.com/htmx.org@2.0.3/dist/htmx.min.js -O assets/js/htmx/htmx.min.js

tailwindcss-init:
	npm i &&\
	npx tailwindcss build -i assets/css/style.css -o assets/css/tailwind.css -m

tailwindcss-build:
	npx tailwindcss build -i assets/css/style.css -o assets/css/tailwind.css -m

clean:
	rm -rf *.o *.so $(BINARY_NAMR)
