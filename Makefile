CFLAGS+=-I include -L /home/sommeru/Desktop/riech-o-mat/lib $(ncurses5-config --libs) $(ncurses5-config --cflags) -lmenu -lform -liowkit -Wall -Werror  

.PHONY: all clean

all: riech-o-mat

riech-o-mat: riech-o-mat.c

clean:
	rm -f riech-o-mat

