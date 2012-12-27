CFLAGS+=-Wall
LDFLAGS+=
LDLIBS+=

.PHONY: all clean

all: riech-o-mat-backend

riech-o-mat-backend: riech-o-mat-backend.o iowkit.o

riech-o-mat.ico: riech-o-mat.svg
	inkscape riech-o-mat.svg --export-dpi 240 --export-png riech-o-mat-128.png
	inkscape riech-o-mat.svg --export-dpi 120 --export-png riech-o-mat-64.png
	inkscape riech-o-mat.svg --export-dpi 60 --export-png riech-o-mat-32.png
	inkscape riech-o-mat.svg --export-dpi 30 --export-png riech-o-mat-16.png
	convert riech-o-mat-16.png riech-o-mat-32.png riech-o-mat-64.png riech-o-mat-128.png riech-o-mat.ico

clean:
	rm -f riech-o-mat-backend riech-o-mat-backend.o iowkit.o

