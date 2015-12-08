#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define WIDTH 70
#define HEIGHT 40

#define MAX_HISTORY 100
#define BUFSIZE 1000

char canvas[WIDTH][HEIGHT];
char history[MAX_HISTORY][BUFSIZE];

const char *default_history_file = "history.txt";

void print_canvas(FILE *fp)
{
  int x, y;

  fprintf(fp, "----------\n");

  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {
      const char c = canvas[x][y];
      fputc(c, fp);
    }
    fputc('\n', fp);
  }
  fflush(fp);
}

void init_canvas()
{
  memset(canvas, ' ', sizeof(canvas));
}

int max(const int a, const int b)
{
  return (a > b) ? a : b;
}

void draw_line(const int x0, const int y0, const int x1, const int y1)
{
  int i;
  const int n = max(abs(x1 - x0), abs(y1 - y0));
  for (i = 0; i <= n; i++) {
    const int x = x0 + i * (x1 - x0) / n;
    const int y = y0 + i * (y1 - y0) / n;
    canvas[x][y] = '#';
  }
}

void save_history(const char *filename, const int history_size)
{
  if (filename == NULL)
    filename = default_history_file;
  
  FILE *fp;
  if ((fp = fopen(filename, "w")) == NULL) {
    fprintf(stderr, "error: cannot open %s.\n", filename);
    return;
  }
  
  int i;
  for (i = 0; i < history_size; i++) {
    fprintf(fp, "%s", history[i]);
  }

  printf("saved as \"%s\"\n", filename);

  fclose(fp);
}

// Interpret and execute a command
//   return value:
//     0, normal commands such as "line"
//     1, unknown or special commands (not recorded in history[])
//     2, quit
int interpret_command(const char *command, int *hsize)
{
  int i;
  char buf[BUFSIZE];
  strcpy(buf, command);
  buf[strlen(buf) - 1] = 0; // remove the newline character at the end

  const char *s = strtok(buf, " ");

  if (strcmp(s, "line") == 0) {
    int x0, y0, x1, y1;
    x0 = atoi(strtok(NULL, " "));
    y0 = atoi(strtok(NULL, " "));
    x1 = atoi(strtok(NULL, " "));
    y1 = atoi(strtok(NULL, " "));
    draw_line(x0, y0, x1, y1);
    return 0;
  }

  if (strcmp(s, "save") == 0) {
    s = strtok(NULL, " ");
    save_history(s, *hsize);
    return 1;
  }

  if (strcmp(s, "undo") == 0) {
    init_canvas();
    for (i = 0; i < *hsize - 1; i++) {
      interpret_command(history[i], NULL);
    }
    (*hsize)--;
    return 1;
  }

  if (strcmp(s, "quit") == 0) {
    return 2;
  }

  printf("error: unknown command.\n");

  return 1;
}

int main()
{
  const char *filename = "canvas.txt";
  FILE *fp;
  char buf[BUFSIZE];

  if ((fp = fopen(filename, "a")) == NULL) {
    fprintf(stderr, "error: cannot open %s.\n", filename);
    return 1;
  }

  init_canvas();
  print_canvas(fp);

  int hsize = 0;  // history size
  while (hsize < MAX_HISTORY) {

    printf("%d > ", hsize);
    fgets(buf, BUFSIZE, stdin);

    const int r = interpret_command(buf, &hsize);
    if (r == 2) break;
    if (r == 0) {
      strcpy(history[hsize], buf);
      hsize++;
    }

    print_canvas(fp);
  }

  fclose(fp);

  return 0;
}
