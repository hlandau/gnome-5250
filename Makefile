

CFLAGS			= -g `gnome-config --cflags glib gnome gnomeui` -I/usr/local/include
LIBS			= `gnome-config --libs glib gnome gnomeui` -L/usr/local/lib -l5250

gnome_5250_OBJECTS	= gnome-5250.o gtk5250terminal.o

gnome-5250: $(gnome_5250_OBJECTS)
	$(CC) -g -o gnome-5250 $(gnome_5250_OBJECTS) $(LIBS)

clean:
	rm -f *~ gnome-5250 $(gnome_5250_OBJECTS)
