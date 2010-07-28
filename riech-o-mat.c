#include <iowkit.h>
#include <menu.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <form.h>

#define MAX_ENTRIES 5000
#define MAX_LINE_LENGTH 1024
#define version "Riech-O-Mat 0.1a"

typedef struct {
	char* label; /**< label string */
	int valvePositions; /**< bit field of desired valve positions */
	int isComment; /**< skip this line when executing, if not 0 */
} MenuEntry;

ITEM** it;
MENU* me;
FIELD** step_field;
FORM* step_form;

WINDOW* win;
WINDOW* helpwin;
WINDOW* outermenuwin;
WINDOW* menuwin;
WINDOW* valvewin;
WINDOW* stepwin;
WINDOW* errorwin;
IOWKIT_HANDLE devHandle,iowHandle;
int win_max_x,win_max_y;






void DisplayError(char* errormsg) {
timeout(10000);
	errorwin = subwin(win,6,30, win_max_y/2-3, win_max_x/2-10);
	mvwaddstr(errorwin, 1, 2, errormsg);
	wbkgd(errorwin,COLOR_PAIR(2)|A_REVERSE);
	wborder(errorwin,0,0,0,0,0,0,0,0);
	wrefresh(errorwin);
getchar();
timeout(10);	
}


int readMenuEntries(char* fname, MenuEntry entries[]) {

/**
 * read all menu entries from the given file.
 * Entries are stored in entries. We assume that appropriate space has been allocated beforehand.
 * the number of entries read is returned
 */
	int entryCount = 0;
	int lineNo = 0;
	FILE* f;
	if((f = fopen(fname, "r")) == NULL) {
		DisplayError("Fatal Error\n\n  Punchcard file not found!");
		fprintf(stderr, "Error opening punch card file: \"%s\". \n", fname);
	}
	while (!feof(f)) {
		char line[MAX_LINE_LENGTH];
		++lineNo;
		fgets(line, MAX_LINE_LENGTH, f);
		if (strlen(line) >= MAX_LINE_LENGTH-1) continue;
		line[strlen(line)-1] = '\0';

		/* skip empty lines */
		if (strlen(line) <= 1) continue;

		if (line[0] == '#') {
			/* this is a comment */
			char* s = line+1;
			while ((*s == ' ') || (*s == '#')) s++;

			entries[entryCount].label = strdup(s);
			entries[entryCount].valvePositions = 0;
			entries[entryCount].isComment = 1;
			entryCount++;
		}
		else {
			/* this is not a comment */

			/* read valve positions char by char */
			int valvePositions = 0; /**< bit field of valve positions read so far */
			int currentPos = 0; /**< number of valve we're currently reading */
			char* s;
			for (s = line; *s != '\0'; ++s) {
				if (*s == '0') {
					currentPos++;
				}
				else if (*s == '1') {
					/* set bit #currentPos in valvePositions */
					valvePositions |= (1 << currentPos);
					currentPos++;
				}
				else if (*s == ' ') {
				}
				else if (*s == '#') {
					break;
				}
				else {
						DisplayError("Fatal Error\n\n  Problem with punchcard chars!");
						fprintf(stderr, "Error reading punch card \"%s\". Expected 0 or 1, got \"%c\" (l. %d)\n", fname, *s, lineNo);
					exit(1);
				}
			}

			entries[entryCount].label = strdup(line);
			entries[entryCount].valvePositions = valvePositions;
			entries[entryCount].isComment = 0;
			entryCount++;
		}

	}	
	fclose(f);

	return entryCount;
}

/**
 * atexit hook
 */
void quit(void) {
	int i;

	unpost_form(step_form);
	free_form(step_form);

	unpost_menu(me);
	free_menu(me);

	for(i=0; i<=4; i++)
	{
		free_item(it[i]);
	}

	free(it);
	delwin(win);

	endwin();
	/*Close device*/
	IowKitCloseDevice(devHandle);
}

bool WriteSimple (IOWKIT_HANDLE devHandle, DWORD value)
{
	IOWKIT_REPORT rep;
	IOWKIT56_IO_REPORT rep56;

	/* Init report */
	switch (IowKitGetProductId(devHandle))
	{
		/* Write simple value to IOW40*/
		case IOWKIT_PRODUCT_ID_IOW40:
			memset(&rep, 0xff, sizeof(rep));
			rep.ReportID = 0;
			rep.Bytes[3] = (UCHAR) value;
			return IowKitWrite(devHandle, IOW_PIPE_IO_PINS,
					(PCHAR) &rep, IOWKIT40_IO_REPORT_SIZE) == IOWKIT40_IO_REPORT_SIZE;
			/* Write simple value to IOW24*/
		case IOWKIT_PRODUCT_ID_IOW24:
			memset(&rep, 0xff, sizeof(rep));
			rep.ReportID = 0;
			rep.Bytes[0] = (UCHAR) value;
			return IowKitWrite(devHandle, IOW_PIPE_IO_PINS,
					(PCHAR) &rep, IOWKIT24_IO_REPORT_SIZE) == IOWKIT24_IO_REPORT_SIZE;
			/* Write simple value to IOW56*/
		case IOWKIT_PRODUCT_ID_IOW56:
			memset(&rep56, 0xff, sizeof(rep56));
			rep56.ReportID = 0;
			rep56.Bytes[6] = (UCHAR) value;
			return IowKitWrite(devHandle, IOW_PIPE_IO_PINS,
					(PCHAR) &rep56, IOWKIT56_IO_REPORT_SIZE) == IOWKIT56_IO_REPORT_SIZE;
		default:
			return FALSE;
	}
}

void performStep(MenuEntry* entry) {

	int bitwise_valve_pos=0;	

	/* Open device*/
	devHandle = IowKitOpenDevice();
	if (devHandle == NULL) {
		DisplayError("Fatal Error\n\n  Riech-O-mat not connected!");
		fprintf(stderr, "Error opening device \"iowarior\". make sure it's connected.\n");
		exit(1);
	}
	iowHandle = IowKitGetDeviceHandle(1);
	IowKitSetWriteTimeout(iowHandle, 10);
	WriteSimple(iowHandle,~entry->valvePositions);
	IowKitCloseDevice(devHandle);

	/* Set valve graphics*/
	bitwise_valve_pos=~entry->valvePositions;

	if ((~bitwise_valve_pos & 1) == 1) {
		mvwaddch(valvewin, 6,win_max_x/4-31/2-2 +15, ACS_VLINE);
	}
	else {
		mvwaddch(valvewin, 6,win_max_x/4-31/2-2 +15, ACS_ULCORNER);	
	}

	if ((~bitwise_valve_pos & 2) == 2) {
		mvwaddch(valvewin, 3,win_max_x/4-31/2-2 +3 , ACS_VLINE);	}
	else {
		mvwaddch(valvewin, 3,win_max_x/4-31/2-2 +3 , ACS_TTEE);
	}

	if ((~bitwise_valve_pos & 4) == 4) {
		mvwaddch(valvewin, 3,win_max_x/4-31/2-2 +11 , ACS_VLINE);	}
	else {
		mvwaddch(valvewin, 3,win_max_x/4-31/2-2 +11 , ACS_TTEE);
	}

	if ((~bitwise_valve_pos & 8) == 8) {
		mvwaddch(valvewin, 3,win_max_x/4-31/2-2 +19 , ACS_VLINE);	}
	else {
		mvwaddch(valvewin, 3,win_max_x/4-31/2-2 +19 , ACS_TTEE);
	}

	if ((~bitwise_valve_pos & 16) == 16) {
		mvwaddch(valvewin, 3,win_max_x/4-31/2-2 +27 , ACS_VLINE);	}
	else {
		mvwaddch(valvewin, 3,win_max_x/4-31/2-2 +27 , ACS_TTEE);
	}

	wrefresh(valvewin);
}



/**
 * main
 */
int main(int argc, char* argv[]) {

	MenuEntry entries[MAX_ENTRIES];
	int entryCount;
	int i;
	int ch;
	bool shutdown = false;
	bool autoRun = false;
	double nextStepAt;
	double steptime;

	/* init ncurses */
	initscr();
	atexit(quit);
	clear();
	noecho();
	curs_set(0);
	timeout(10); /**< getch times out after 10ms */
	cbreak();
	nl();
	keypad(stdscr, TRUE);
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_YELLOW, COLOR_BLUE);
	init_pair(3, COLOR_WHITE, COLOR_BLUE);
	init_pair(4, COLOR_YELLOW, COLOR_WHITE);




	/* Set main window and sub window */
	getmaxyx(stdscr, win_max_y, win_max_x);
	win = newwin(win_max_y, win_max_x, 0, 0);
	outermenuwin = subwin(win,win_max_y-2,(win_max_x/2)-1, 1, win_max_x/2); 
	set_menu_win(me, win);
	menuwin= derwin(win,win_max_y-4,(win_max_x/2)-3, 2, win_max_x/2+1);
	set_menu_sub(me,menuwin);
	set_menu_format(me, win_max_y-4, 1);

	/*  Set menu mark to string */
	set_menu_mark(me, "--> ");

	/* Print a border around the main window and print a title */
	//box(win, 0, 0);
	//mvwaddch(win, 0, win_max_x/2-1, ACS_TTEE);
	//mvwvline(win, 1, win_max_x/2-1, ACS_VLINE, win_max_y-2);
	//mvwaddch(win, win_max_y-1, win_max_x/2-1, ACS_BTEE);
	mvwaddstr(win, 0, win_max_x/2-strlen(version)/2-1, version);

	/*  set colors        */
	set_menu_fore(me, COLOR_PAIR(2)|A_REVERSE);
	set_menu_back(me, COLOR_PAIR(2));
	set_menu_grey(me, COLOR_PAIR(3));
	bkgd(COLOR_PAIR(1));
	wbkgd(win,COLOR_PAIR(1));
	wbkgd(menuwin,COLOR_PAIR(2));
	wbkgd(outermenuwin,COLOR_PAIR(2));
       

	/* Display Help Window */
	helpwin= subwin(win, win_max_y-18, win_max_x/2-2, 1, 1);
	mvwaddstr(helpwin, 1, 2, "UP/DOWN     Select valve positions");
	mvwaddstr(helpwin, 2, 2, "ENTER       Start/Stop sequencer");
	mvwaddstr(helpwin, 3, 2, "SHIFT + Q   Exit Riech-O-Mat");
	mvwaddstr(helpwin, 4, 2, "SHIFT + S   Set stepping time");

	wbkgd(helpwin,COLOR_PAIR(3));

	/*Display Stepping Time Window*/
	stepwin= subwin(win, 5, win_max_x/2-2, win_max_y-16, 1);
	wbkgd(stepwin,COLOR_PAIR(3));

	step_field = (FIELD **)calloc(1, sizeof(FIELD *));
	step_field[0] = new_field(1, 5, 2, 17, 0, 0);
	set_field_fore(step_field[0], COLOR_PAIR(1));
	set_field_back(step_field[0], COLOR_PAIR(4));
	step_form = new_form(step_field);

	set_field_just(step_field[0], JUSTIFY_RIGHT);
	set_field_type(step_field[0], TYPE_INTEGER,0, 100, 99999);
	set_field_buffer(step_field[0], 0, "1000");
	steptime=1000;

	set_form_win(step_form,stepwin);
	post_form(step_form);
	mvwaddstr(stepwin, 2, 2, "Stepping time:");
	mvwaddstr(stepwin, 2, 23, "ms");




	/* Display valve Positions */
	valvewin= subwin(win, 9, win_max_x/2-2, win_max_y-10,1);
	wbkgd(valvewin,COLOR_PAIR(3));
	mvwaddstr(valvewin,1,win_max_x/4-31/2-2, "CONTROL  ODOR_1  ODOR_2  ODOR_3");
	mvwaddch(valvewin, 2,win_max_x/4-31/2-2 +3 , ACS_VLINE);                     
	mvwaddch(valvewin, 2,win_max_x/4-31/2-2 +11, ACS_VLINE);                                               
	mvwaddch(valvewin, 2,win_max_x/4-31/2-2 +19, ACS_VLINE); 
	mvwaddch(valvewin, 2,win_max_x/4-31/2-2 +27, ACS_VLINE);

	mvwaddch(valvewin, 3,win_max_x/4-31/2-2 +3 , ACS_VLINE);                     
	mvwaddch(valvewin, 3,win_max_x/4-31/2-2 +11, ACS_VLINE);                                               
	mvwaddch(valvewin, 3,win_max_x/4-31/2-2 +19, ACS_VLINE); 
	mvwaddch(valvewin, 3,win_max_x/4-31/2-2 +27, ACS_VLINE);

	mvwhline(valvewin, 4,win_max_x/4-31/2-2 +3 , ACS_HLINE,25);                     
	mvwaddch(valvewin, 4,win_max_x/4-31/2-2 +3 , ACS_LLCORNER);                     
	mvwaddch(valvewin, 4,win_max_x/4-31/2-2 +11, ACS_BTEE);                                               
	mvwaddch(valvewin, 4,win_max_x/4-31/2-2 +19, ACS_BTEE); 
	mvwaddch(valvewin, 4,win_max_x/4-31/2-2 +27, ACS_LRCORNER);
	mvwaddch(valvewin, 4,win_max_x/4-31/2-2 +15, ACS_TTEE);  

	mvwaddch(valvewin, 5,win_max_x/4-31/2-2 +15, ACS_VLINE);

	mvwaddch(valvewin, 6,win_max_x/4-31/2-2 +15, ACS_LTEE);
	mvwhline(valvewin, 6,win_max_x/4-31/2-2 +16 , ACS_HLINE,6);
	mvwaddstr(valvewin,6,win_max_x/4-31/2-2 +22, "> EXHAUST");

	mvwaddch(valvewin, 7,win_max_x/4-31/2-2 +15, ACS_VLINE);

	refresh();
	wrefresh(win);


	/* read entries from punch card */
	entryCount = readMenuEntries("punchcard.txt", entries);

	/* Initialize Items */
	it = (ITEM **)calloc(entryCount+1, sizeof(ITEM *));
	for (i = 0; i < entryCount; ++i) {
		it[i] = new_item(entries[i].label, "");
		set_item_userptr(it[i], &entries[i]);
		if (entries[i].isComment) {
			item_opts_off(it[i], O_SELECTABLE);
		}
	}
	it[entryCount] = 0;

	/* Initialize Menu */
	me = new_menu(it);

	/* Post Menu */
	post_menu(me); 
	refresh();
	wrefresh(win);

	while(!shutdown) {
		struct timeval tv;
		double currentTime;
		gettimeofday(&tv, 0);
		currentTime = tv.tv_sec + 1e-6 * tv.tv_usec;

		if (autoRun && (nextStepAt <= currentTime)) {
			if (item_index(current_item(me)) >= entryCount-1) {
				autoRun = false;
			}
			else {
				MenuEntry* currentEntry = 0;

				menu_driver(me, REQ_DOWN_ITEM);

				currentEntry = &entries[item_index(current_item(me))];
				while (currentEntry && currentEntry->isComment) {
					if (item_index(current_item(me)) >= entryCount-1) {
						currentEntry = 0;
						break;
					}
					menu_driver(me, REQ_DOWN_ITEM);
					currentEntry = &entries[item_index(current_item(me))];
				}
				if (currentEntry) {
					performStep(currentEntry);
					nextStepAt = nextStepAt + (steptime / 1000);
				}
				else {
					/* no more items */
					autoRun = false;
				}
			}
		}

		ch=getch();
		switch(ch) {
			case KEY_DOWN:
				if (!autoRun) menu_driver(me, REQ_DOWN_ITEM);
				break;
			case KEY_UP:
				if (!autoRun) menu_driver(me, REQ_UP_ITEM);
				break;
			case KEY_LEFT:
				if (!autoRun) menu_driver(me, REQ_FIRST_ITEM);
				break;
			case KEY_RIGHT:
				if (!autoRun) menu_driver(me, REQ_LAST_ITEM);
				break;

			case 'Q':
				shutdown = true;
				break;
			case 'S':


				set_field_back(step_field[0], COLOR_PAIR(2)|A_REVERSE);
				set_menu_fore(me, COLOR_PAIR(2));
				set_menu_mark(me, "    ");
				set_field_buffer(step_field[0], 0, "");

				wrefresh(win);
				wrefresh(stepwin);

				while((ch=wgetch(stepwin)) != 0xA) {
					switch(ch) {
						case KEY_RIGHT: {
							form_driver(step_form,	REQ_RIGHT_CHAR);
							}
						case KEY_LEFT: {
								form_driver(step_form,	REQ_LEFT_CHAR);
							}
						default: /* Enter stepping cycle */	
							form_driver(step_form, ch);
							wrefresh(stepwin);
					}
				}   
				set_field_fore(step_field[0], COLOR_PAIR(1));
				set_field_back(step_field[0], COLOR_PAIR(4));
				set_menu_fore(me, COLOR_PAIR(2)|A_REVERSE);
				set_menu_mark(me, "--> ");

				steptime=strtol(field_buffer(step_field[0], 0),NULL,10);
				
				if (steptime < 50) {
					steptime = 1000;
					set_field_buffer(step_field[0], 0, "1000");
				}
				
				if (steptime > 99999) {
					steptime = 1000;
					set_field_buffer(step_field[0], 0, "1000");
				}

				wrefresh(win);
				wrefresh(stepwin);

				break;

			case 0xA:
				autoRun = !autoRun;
				nextStepAt = currentTime + (steptime / 1000);
				if (autoRun) {

					MenuEntry* currentEntry = &entries[item_index(current_item(me))];
					while (currentEntry && currentEntry->isComment) {
						if (item_index(current_item(me)) >= entryCount-1) {
							currentEntry = 0;
							break;
						}
						menu_driver(me, REQ_DOWN_ITEM);
						currentEntry = &entries[item_index(current_item(me))];
					}
					if (currentEntry) {
						performStep(currentEntry);
					}
					else {
						autoRun = false;
					}
				}
				break;
		}

		wrefresh(win);
	}   

	return 0;
}

