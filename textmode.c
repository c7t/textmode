/* We have to define _BSD_SOURCE before we include any headers so that
   we get cfmakeraw().  "man termios" tells us about that in the
   "CONFORMING TO" section. */
#define _BSD_SOURCE 1
/* We have to define _POSIX_SOURCE to get fileno(), similar to
   above */
#define _POSIX_SOURCE 1
#include <stdio.h>
#include <stdbool.h>
/* We need termios.h and unistd.h for tcgetattr and tcsetattr and
   cfmakeraw which are all used for UNIX-y unbuffered input, which we
   need so we don't have to press Enter after each keystroke.  */
#include <termios.h>
#include <unistd.h>
/* Need string.h for memset and for memcpy */
#include <string.h>

int term_set_raw(int fd, struct termios *term) {
  struct termios raw;
  memset(&raw, 0, sizeof(raw));

  if (tcgetattr(fd, term) != 0)
    return -1;
  
  /* Take the setting we got as a starting point, we'll use cfmakeraw
     to make them raw */
  memcpy(&raw, term, sizeof(raw));
  cfmakeraw(&raw);

  if (tcsetattr(fd, TCSANOW, &raw) != 0)
    return -1;

  return 0;
}

int term_restore(int fd, struct termios *term) {
  return tcsetattr(fd, TCSANOW, term);
}


int main(int argc, char **argv) {
  int i = 0;

  setbuf(stdin, NULL);
  setbuf(stdout, NULL);

  /* Tell the kernel we want unbuffered input and output */
  struct termios in_term, out_term;
  term_set_raw(fileno(stdin), &in_term);
  term_set_raw(fileno(stdout), &out_term);

  while (true) {
    int ch = getchar();
    /* now that we're printing in raw mode, '\n' (newline) doesn't get
       converted to '\n\r' (newline followed by carriage return).  So
       our virtual typewriter doesn't go back to the left margin if we
       leave out the '\r' */
    printf("%d: %d %c\r\n", i, ch, ch);
    if (ch == 'q')
      break;
  }

  /* Piece of wackiness: stdin and stdout are probably connected to
     the same "terminal" so we have to set these in the reverse order
     that we retrieved them, in case our second retrieval above got
     the settings from after we set raw mode.  If that were the case
     (and it seems to be the case) then we would just be restoring our
     setting of the raw mode.  I'm just putting all this down, but
     it's mostly UNIX trivia that no-one cares about as long as it
     works. */
  term_restore(fileno(stdout), &out_term);
  term_restore(fileno(stdin), &in_term);
  return 0;
}
