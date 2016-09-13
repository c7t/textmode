#define _BSD_SOURCE 1
#define _POSIX_SOURCE 1
#include <stdio.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
/* Need select() call below */
#include <sys/select.h>

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


int main(int argc, char **argv) {
  int i = 0;

  setbuf(stdin, NULL);
  setbuf(stdout, NULL);

  struct termios in_term, out_term;
  term_set_raw(fileno(stdin), &in_term);
  term_set_raw(fileno(stdout), &out_term);

  /* Now we have to initialize ch to something, because we're going
     to go ahead and print something, even if the user never pressed
     a key. (Before we always blocked (waited forever) in getchar()
     waiting for them to press something.) */
  int ch = 0;

  while (true) {
    fd_set rfds;
    struct timeval tv;
    int retval;

    /* Watch stdin (fd 0) to see when it has input. */
    FD_ZERO(&rfds);
    FD_SET(fileno(stdin), &rfds);

    /* 60 FPS is 1,000,000 microseconds (or one second) / 60, this
       isn't quite correct, as we'll have used up some of our frame
       time computing in this loop and writing output, but we can
       correct that later.  Right now we just don't want to wait too
       long for input to come in. */
    tv.tv_sec = 0;
    tv.tv_usec = 1000000 / 60;

    /* Select basically says: wait for a maximum timeout for some data
       to become ready on the file descriptors that I specify.  Here
       we use it only to look for input on stdin. */
    retval = select(1, &rfds, NULL, NULL, &tv);
    /* Don't rely on the value of tv now! */

    if (retval == -1)
      perror("select()");
    else if (retval) {
      /* FD_ISSET(0, &rfds) will be true. This means we can "safely"
	 call getchar() and it will return immediately with the
	 character that was pressed, it won't block. */
      ch = getchar();
    } /* else no key was pressed, we continue to "render" (in this
	 case just print the last character received again */

    printf("%d: %d %c\r\n", i, ch, ch);
    if (ch == 'q')
      break;
  }

  term_restore(fileno(stdout), &out_term);
  term_restore(fileno(stdin), &in_term);
  return 0;
}
