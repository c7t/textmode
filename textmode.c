/* We need stdio.h for printf and getchar and setbuf */
#include <stdio.h>
/* We need stdbool.h for true (used in the 'while' below) */
#include <stdbool.h>

int main(int argc, char **argv) {
  int i = 0;

  /* Tell the C library that we don't want buffering on our input or
     output (this is not enough, a later update will tell the kernel
     that we don't want buffering on our tty) */
  setbuf(stdin, NULL);
  setbuf(stdout, NULL);

  /* Keep getting characters forever.  We could use a do {} while ()
     since we only have a single exit condition (the 'if (ch == 'q')'
     line below, but we'll want other ways of quitting later, so while
     (true) is perfectly OK. */
  while (true) {
    /* Read a character from "standard input" but really probably just
       the keyboard */
    int ch = getchar();
    /* Print out the iteration number, the character code as a decimal
       number, and as a character.  You can see the relationship
       between character codes and the characters if you "man ascii"
       Everybody uses Unicode and UTF-8 now, but this will make things
       a little simpler. */
    printf("%d: %d %c\n", i, ch, ch);

    /* Hey, if you pressed a 'q' (followed by "Enter", because we
       didn't finish turning off buffering), then we can quit.  Break
       out of this while loop. */
    if (ch == 'q')
      break;
  }

  /* Main always returns an int, it's in the C standard.  0 means all
     good, no errors */
  return 0;
}
