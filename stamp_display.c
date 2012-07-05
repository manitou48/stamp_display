// stamp_display  [/dev/bstamp] [9600]  [echo]
//   DEBUG output/input for stamp (spin)
//   for spin: stamp_display  /dev/ttyUSB0 115200 echo
// cc -o stamp_display stamp_display.c -lncurses
//  default /dev/bstamp  9600
//   select keyboard or remote input
// send typed char to stamp;  print/interpret character from stamp
// TODO  ctrl-c,  delete?   break?

#include <stdlib.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/select.h>
#include <ncurses.h>

#define CRSRXY 2 // Move to cursor to X, Y (X-byte and Y-byte follow command)
#define CRSRLF 3 // Move cursor position left
#define CRSRRT 4 // Move cursor position right
#define CRSRUP 5 // Move cursor position up one line
#define CRSRDN 6 // Move cursor position down one line
#define LF 10 // Linefeed character
#define CLREOL 11 // Clear all characters to the end of current line
#define CLRDN 12 // Clear all characters from the current position to the end of window
#define CR 13 // carriage return
#define CRSRX 14 // Move to cursor to column X (X-byte follows command)
#define CRSRY 15 // Move to cursor to line Y (Y-byte follows command)
#define CS 16 // clear screen spin
#define CLS 0 // clear screen
#define HOME 1  // home cursor

#define DATABITS CS8
#define STOPBITS 0
#define PARITY 0
#define PARITYON 0

#define DANG(X) {perror("X"); exit(1);}

#define STATE_IDLE 0
#define STATE_WANT_X 1
#define STATE_WANT_Y 2
#define STATE_WANT_X1 3
#define STATE_WANT_Y1 4
static int fd,state,row,col,newx;

static char *baud_names[] = {"2400","9600","115200",""};
static int baud_vals[] = {B2400, B9600, B115200};
static int baud = B9600, baud_index=1;
char *tty_port = "/dev/bstamp";
struct termios oldtio;
static int do_echo=0;

void set_baud(char *name) {
	int i=0;
	while(*baud_names[i]) {
		if (strcmp(baud_names[i],name) == 0) {
			baud_index = i;
			baud = baud_vals[i];
			return;
		}
		i++;
	}
}

void init_port() {
	struct termios newtio;

      fd = open(tty_port, O_RDWR | O_NOCTTY );
      if (fd < 0)
      {
         perror(tty_port);
         exit(-1);
      }
      tcgetattr(fd,&oldtio); // save current port settings
      newtio.c_cflag = baud |  DATABITS | STOPBITS | PARITYON | PARITY | CLOCAL | CREAD;
      newtio.c_iflag = IGNPAR;
      newtio.c_oflag = 0;
      newtio.c_lflag = 0;       //ICANON;
      newtio.c_cc[VMIN]=1;
      newtio.c_cc[VTIME]=0;
      tcflush(fd, TCIFLUSH);
      tcsetattr(fd,TCSANOW,&newtio);
}

int set_state(int c) {
	// in STATE_IDLE set state based on c
	// return indicates whether to add character to screen
  	state=STATE_IDLE;
	switch(c) {
	  case CRSRXY:
	  	state=STATE_WANT_X;
		break;
	  case CRSRX:
	  	state=STATE_WANT_X1;
		break;
	  case CRSRY:
	  	state=STATE_WANT_Y1;
		break;
	  case CRSRLF:
	  	move(row,col-1);
		refresh();
		break;
	  case CRSRRT:
	  	move(row,col+1);
		refresh();
		break;
	  case CRSRUP:
	  	move(row-1,col);
		refresh();
		break;
	  case CRSRDN:
	  	move(row+1,col);
		refresh();
		break;
	  case CLREOL:
	  	clrtoeol();
		refresh();
		break;
	  case CLRDN:
	  	clrtobot();
		refresh();
		break;
	  case CLS:
	  case CS:
	  	clear();
		refresh();
		break;
	  case HOME:
	  	move(0,0);
		refresh();
		break;
	  case CR:
	  	addch('\n');
		break;
	  default:
	  	return 1;    // print char
		break;
	}
	return 0;
}

void read_port() {
	int c,n;
	unsigned char byte;

	getyx(stdscr,row,col);    // current position
	read(fd,&byte,1);
	c = byte;
	switch(state) {
	 case STATE_WANT_X:
		newx = c;
		state=STATE_WANT_Y;
	 	break;
	 case STATE_WANT_Y:
		move(c,newx);
		state=STATE_IDLE;
	 	break;
	 case STATE_WANT_X1:
		move(row,c);
		state=STATE_IDLE;
	 	break;
	 case STATE_WANT_Y1:
		move(c,col);
		state=STATE_IDLE;
	 	break;
	 case STATE_IDLE:
		if (set_state(c)) {
			addch(c);
			refresh();
		}
		break;
	}
}

void read_keys() {
	unsigned char c, cr='\r';
	c=getch();
	if (do_echo && c == '\n') write(fd,&cr,1);
	write(fd,&c,1);
	if (do_echo && isalnum(c)) {   // spin echo
		addch(c);
		refresh();
	}
}

int main(int argc, char *argv[])
{	
	fd_set rdfds, rdfds_master;

	if (argc > 1) tty_port = argv[1];
	if (argc > 2) set_baud(argv[2]);
	if (argc > 3) do_echo=1;
	initscr();			/* Start curses mode 		  */
	noecho();
	scrollok(stdscr,TRUE);
	printw("stamp_display %s at %s",tty_port,baud_names[baud_index]);
	refresh();			/* Print it on to the real screen */
	init_port();
	FD_ZERO(&rdfds_master);
	FD_SET(0,&rdfds_master);
	FD_SET(fd,&rdfds_master);
	while(1){
		rdfds = rdfds_master; /* reset */
		if ( select(FD_SETSIZE, &rdfds, NULL,NULL,NULL) <0)
			DANG(select);
		if (FD_ISSET(0,&rdfds))  read_keys();
		if (FD_ISSET(fd,&rdfds)) read_port();
	}
	
	endwin();			/* End curses mode		  */

	return 0;
}

