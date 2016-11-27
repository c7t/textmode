#define _BSD_SOURCE 1
#define _POSIX_SOURCE 1
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>

const char *clear = "\x1b[H\x1b[2J";
const char *civis = "\033[?25l";
const char *cnorm = "\033[?12l\033[?25h";

const char *level =
  "+---------------------------+"
  "|  *       H                |"
  "|         OH===             |"
  "| .        H                |"
  "|        + H   ===       +--+"
  "|    ?     H             | ?|"
  "+------------------------+--+";
int level_width = 29;
int level_height = 7;

// Speed constants
int max_speed;
int gravity;
int walk_sx;
int walk_sy;
int climb_s;
int jump_s;
int animate_s;

static const int fp_shift = 4;

static inline int tofp(int a) {
  return a << fp_shift;
}

static inline int fp(int a) {
  return a >> fp_shift;
}

static inline int min(int a, int b) {
  return a < b ? a : b;
}

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
bool get_char(int *ch, struct timeval tv) {
  int character = -1;
  fd_set rfds;
  int retval;

  FD_ZERO(&rfds);
  FD_SET(fileno(stdin), &rfds);

  // first wait for the full specified time, ignoring input
  retval = select(0, NULL, NULL, NULL, &tv);

  for (;;) {
    // now poll for any input, without waiting
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    retval = select(1, &rfds, NULL, NULL, &tv);

    if (retval == -1)
      perror("select()");
    else if (retval) {
      character = getchar();
    } else
      break;
  }
  if (character != -1)
    *ch = character;
  return character != -1;
}


int main(int argc, char **argv) {
  max_speed = tofp(1) / 2;
  gravity = tofp(1) / 4;
  walk_sx = tofp(1) / 2;
  walk_sy = tofp(1) / 4;
  climb_s = tofp(1) / 2;
  jump_s = tofp(1);
  animate_s = tofp(1) / 2;

  int i = 0;

  setbuf(stdin, NULL);
  setbuf(stdout, NULL);

  struct termios in_term, out_term;
  term_set_raw(fileno(stdin), &in_term);
  term_set_raw(fileno(stdout), &out_term);

  int ch = 0;

  print(clear);
  print(civis);

  for (int row = 0; row < level_height; row++) {
    moveto(row, 0);
    printn(level + row * level_width, level_width);
  }

  int row = tofp(level_height / 2), col = tofp(level_width / 2 - 1);
  int sx = walk_sx, sy = walk_sy;

  int manimation = tofp(0);

  while (true) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000000 / 30; /* 30 FPS */

    bool key_pressed = get_char(&ch, tv);

    // Erase protagonist by drawing level
    moveto(fp(row), fp(col));
    fputc(level[fp(row) * level_width + fp(col)], stdout);

    char on = level[fp(row) * level_width + fp(col)];

    // Gravity!
    // standing is the character under the character (what the
    // character is standing on)
    char standing = level[(fp(row) + 1) * level_width + fp(col)];

    moveto(0, level_width + 1);
    print("S: ");
    fputc(standing, stdout);

    moveto(1, level_width + 1);
    print("O: ");
    fputc(on, stdout);

    switch (standing) {
    case ' ': // unlike Wile E. Coyote, character cannot stand on air
    case '?': // can fall into a teleporter
    case '.': // go right through '.' even falling
      if (on != 'H') {
	// apply acceleration by incrementing velocity
	sy = min(sy + gravity, max_speed);
      }
      break;
    default: // everything else is solid, no gravity applied
      if (standing != 'H') { // down is allowed on a ladder
	if (sy > tofp(0))
	  sy = tofp(0);
      }
      break;
    }

    if (key_pressed) {
      switch (ch) {
      case 'w': if (on == 'H') { sx = tofp(0); sy = -climb_s; } break;
      case 'a': sx = -walk_sx; sy = tofp(0); break;
      case 's': if (on == 'H') { sx = tofp(0); sy = climb_s; } break;
      case 'd': sx = walk_sx; sy = tofp(0); break;
      case ' ':
	// only jump from ground, prevents double jumps
	if (strchr(" .?", standing) == NULL) sy = -jump_s; break;
      }
    }

    row += sy;
    col += sx;

    /* Collision detection! */
    char bumped = level[fp(row) * level_width + fp(col)];
    if (bumped == '?') {
      char spawned_on;
      do {
	row = random() % level_height;
	col = random() % level_width;
	spawned_on = level[row * level_width + col];
	row = tofp(row);
	col = tofp(col);
      } while (spawned_on != ' ');
    } else if (bumped == '.') {
      /* ignore '.' for now */
    } else if (bumped == 'H') {
      /* you can walk through a ladder */
    } else if (bumped != ' ') {
      row -= sy;
      sy = tofp(0);
      col -= sx;
      sx = tofp(0);
    } /* else ' ', just keep walking */

    manimation += animate_s;

    moveto(fp(row), fp(col));
    print(fp(manimation) & 1 ? "C" : "c");

    if (ch == 'q')
      break;
  }

  print(clear);
  print(cnorm);
  term_restore(fileno(stdout), &out_term);
  term_restore(fileno(stdin), &in_term);
  return 0;
}

/*

 xX
--x---

*/
