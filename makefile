CFLAGS = -Wextra -Wall -Werror -pedantic -lX11 -lXfixes

install:
	gcc ./src/main.c -o rogdrop $(CFLAGS)
