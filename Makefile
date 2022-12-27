all: gdbus-introspection-to-code.out

gdbus-introspection-to-code.out: gdbus-introspection-to-code.c
	$(CC) -o $@ $^ -Wall -Wextra -ggdb `pkg-config --cflags --libs glib-2.0 gio-2.0` $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)
