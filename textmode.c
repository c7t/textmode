#define _BSD_SOURCE 1
#define _POSIX_SOURCE 1
#include <stdio.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>

/* The magic of terminfo, tput and xxd:

terminfo is a database of all the terminals that UNIX machines have
ever connected to in the history of man.  terminfo knows what magic
strings to send to make the terminal do crazy stuff like hide the
cursor, or move the cursor or make bold writing.

tput is a program that looks up the current terminal the terminfo
database and then sends the strings it finds to the screen.

and xxd is a sweet hexdump utility.

So putting it all together you can do this:

ctalbott@colossus:~/p/textmode$ tput clear

[the screen is cleared!]

ctalbott@colossus:~/p/textmode$ tput clear | xxd
0000000: 1b5b 481b 5b32 4a                        .[H.[2J

Now, instead of clearing the screen, the command gets piped to xxd
which shows us the sequence of characters that would've been sent to
the screen (as hexidecimal pairs of bytes).  So here '1b' is hex for
27, or "Escape" (you can "man ascii" to see what all these codes are,
or you can press Escape while running textmode to see that it prints
27).

To turn off the cursor, send this:

ctalbott@colossus:~/p/textmode$ tput civis | xxd
0000000: 1b5b 3f32 356c                           .[?25l

To turn it back on, send this:

ctalbott@colossus:~/p/textmode$ tput cnorm | xxd
0000000: 1b5b 3f31 326c 1b5b 3f32 3568            .[?12l.[?25h

Try it out with:
ctalbott@colossus:~/p/textmode$ tput cnorm

And, lo!  Your cursor is gone.

Here's one that takes parameters:

ctalbott@colossus:~/p/textmode$ tput cup 5 20 | xxd
0000000: 1b5b 363b 3231 48                        .[6;21H

...to go to screen location row, column, send Escape, '[', row, ';', column, 'H'

Note that we're going to be gross, and only support vt100 terminals,
because that's what xterm (and every other graphical terminal emulator
emulates), because this is 2016, and not 1976, which was 40 years ago,
which makes all this pretty archaic, but hey, so's your brother.

*/

int term_set_raw(int fd, struct termios *term) {
  struct termios raw;
  memset(&raw, 0, sizeof(raw));

  if (tcgetattr(fd, term) != 0)
    return -1;
  
  memcpy(&raw, term, sizeof(raw));
  cfmakeraw(&raw);

  if (tcsetattr(fd, TCSANOW, &raw) != 0)
    return -1;

  return 0;
}

int term_restore(int fd, struct termios *term) {
  return tcsetattr(fd, TCSANOW, term);
}


/* get_char() waits until timeout given by 'tv' for a key press.  If a
   key is pressed in time, the key is returned in 'ch' and get_char()
   returns true.  Otherwise get_char() returns false. */
bool get_char(int *ch, struct timeval *tv) {
    fd_set rfds;
    int retval;

    FD_ZERO(&rfds);
    FD_SET(fileno(stdin), &rfds);

    retval = select(1, &rfds, NULL, NULL, tv);

    if (retval == -1)
      perror("select()");
    else if (retval) {
      *ch = getchar();
    }

    return retval > 0;
}


int main(int argc, char **argv) {
  int i = 0;

  setbuf(stdin, NULL);
  setbuf(stdout, NULL);

  struct termios in_term, out_term;
  term_set_raw(fileno(stdin), &in_term);
  term_set_raw(fileno(stdout), &out_term);

  int ch = 0;

  /* Clear the screen.  Notice that you can print Escape by saying
     \x1b: the \x says, "here comes some hexidecimal:".  Then 1b is a
     single byte of hex.  (You could also use octal, like \033, but
     there is no nice way to use decimal.  I guess you could do this:
     printf("%c[H", 27), but that's sort of clunky, especially when
     you have multiple sequences which need escapes in there. */
  printf("\x1b[H\x1b[2J");

  /* Turn off the cursor: it's distracting to see it bouncing all
     around while we're trying to draw stuff.  Just remember to turn
     it back on.  You can always type "reset" or "tput cnorm" after
     textmode runs, but we'll add code to do it at the bottom of
     main. */
  printf("\033[?25l");

  while (true) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000000 / 60; /* 60 FPS */

    bool key_pressed = get_char(&ch, &tv);

    if (key_pressed) {
      /* Move cursor to 9, 20 */
      printf("\033[%d;%dH", 9, 20);
      printf("pressed!");
    } else {
      /* Move cursor to 9, 20 */
      printf("\033[%d;%dH", 9, 20);
      /* No key pressed, print some spaces to overwrite last
	 "pressed!" */
      printf("        ");
    }

    /* Print status at 10, 20 */
    printf("\033[%d;%dH", 10, 20);
    /* Notice that we don't print the "\r\n" anymore, we're directly
       driving the cursor around, no need to ask the typewriter to go
       to the next line, or back to the left margin */
    printf("%d: %d %c", i, ch, ch);
    if (ch == 'q')
      break;
  }

  /* Turn that cursor back on */
  printf("\033[?12l\033[?25h");

  term_restore(fileno(stdout), &out_term);
  term_restore(fileno(stdin), &in_term);
  return 0;
}
