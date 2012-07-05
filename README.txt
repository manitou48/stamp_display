stamp_display.c

  first cut at getting stamp's DEBUG cursor control options to work with UNIX.


  to build:
         cc -o stamp_display stamp_display.c -lncurses

  to run:
     stamp_display  [/dev/bstamp] [9600]  [echo]
	    defaults are /dev/bstamp and 9600 baud
		for propeller/spin (limited testing)
		  stamp_display /dev/ttyUSB0 115200 echo
  
  build and run your stamp program, then ctrl-c bstamp_run, startup 
    stamp_display

  DEBUGIN should work 

