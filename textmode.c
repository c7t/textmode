#define _BSD_SOURCE 1
#define _POSIX_SOURCE 1
#include <stdio.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>

const char *clear = "\x1b[H\x1b[2J";
const char *civis = "\033[?25l";
const char *cnorm = "\033[?12l\033[?25h";

const char *level =
  "+-------------+"
  "|  *          |"
  "|         O   |"
  "| .           |"
  "|        +    |"
  "|             |"
  "+-------------+";
int level_width = 15;
int level_height = 7;

void moveto(int row, int col) {
  printf("\033[%d;%dH", row+1, col+1);
}

void print(const char *str) {
  fputs(str, stdout);
}

void printn(const char *str, size_t n) {
  fwrite(str, n, 1, stdout);
}

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

  print(clear);
  print(civis);

#if 0
  for (int row = 0; row < 10; row++) {
    for (int col = 0; col < 10; col++) {
      moveto(row, 10);
      printf("%d", row);
      moveto(10, col);
      printf("%d", col);
    }
  }

  goto cleanup;
#endif

  for (int row = 0; row < level_height; row++) {
    moveto(row, 0);
    printn(level + row * level_width, level_width);
  }

  int row = level_height / 2, col =  level_width / 2;
  int sx = 1, sy = 1;

  while (true) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000000 / 15; /* 15 FPS */

    bool key_pressed = get_char(&ch, &tv);

    if (key_pressed) {
      switch (ch) {
      case 'w': sx = 0; sy = -1; break;
      case 'a': sx = -1; sy = 0; break;
      case 's': sx = 0; sy = 1; break;
      case 'd': sx = 1; sy = 0; break;
      }
    }

    moveto(row, col);
    print(" ");

    row += sy;
    col += sx;

    /* Collision detection! */
    if (level[row * level_width + col] != ' ') {
      row -= sy;
      sy = 0;
      col -= sx;
      sx = 0;
    }

    moveto(row, col);
    print("#");

    if (ch == 'q')
      break;
  }

  print(clear);
  print(cnorm);
  term_restore(fileno(stdout), &out_term);
  term_restore(fileno(stdin), &in_term);
  return 0;
}
