#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define WIDTH 70
#define HEIGHT 40

#define BUFSIZE 1000

/******************************************************************************/
struct node
{
    char *str;
    struct node *next;
};

typedef struct node Node;

Node *push_front(Node *begin, const char *str);
Node *pop_front(Node *begin);
Node *push_back(Node *begin, const char *str);
Node *pop_back(Node *begin);
Node *remove_all(Node *begin);

/*****************************************************************************/
char canvas[WIDTH][HEIGHT];
Node *history = NULL;

const char *default_history_file = "history.txt";

/*****************************************************************************/
void print_canvas(FILE *fp);
void init_canvas();
int max(const int a, const int b);
void draw_line(const int x0, const int y0, const int x1, const int y1);
void draw_rect(const int x0, const int y0, const int x1, const int y1);
void draw_circle(const int x, const int y, const int radius);
void save_history(const char *filename);
int interpret_command(const char *command);
int check_in_canvas(const int x, const int y);

/****************************************************************************/
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
    while(1) {

        printf("%d > ", hsize);
        fgets(buf, BUFSIZE, stdin);

        const int r = interpret_command(buf);
        if (r == 2) break;
        if (r == 0) {
            history = push_back(history, buf);
            hsize++;
        }

        print_canvas(fp);
    }

    fclose(fp);

    return 0;
}

/***********************************************************************************/
Node *push_front(Node *begin, const char *str)
{
    // Create a new element
    Node *p = malloc(sizeof(Node));
    char *s = malloc(strlen(str) + 1);
    strcpy(s, str);
    p->str = s;
    p->next = begin; 

    return p;  // Now the new element is the first element in the list
}

Node *pop_front(Node *begin)
{
    assert(begin != NULL); // Don't call pop_front() when the list is empty
    Node *p = begin->next;

    free(begin->str);
    free(begin);

    return p;
}

Node *push_back(Node *begin, const char *str)
{
    if (begin == NULL) {   // If the list is empty
        return push_front(begin, str);
    }

    // Find the last element
    Node *p = begin;
    while (p->next != NULL) {
        p = p->next;
    }

    // Create a new element
    Node *q = malloc(sizeof(Node));
    char *s = malloc(strlen(str) + 1);
    strcpy(s, str);
    q->str = s;
    q->next = NULL;

    // The new element should be linked from the previous last element
    p->next = q;

    return begin;
}

Node *pop_back(Node *begin) {
    assert(begin != NULL); // Don't call pop_front() when the list is empty
    Node *p = begin;
    if(begin->next == NULL) {
        return pop_front(begin);
    }

    //finde second last element
    while(p->next->next != NULL) {
        p = p->next;
    }
    free(p->next->str);
    free(p->next);
    p->next = NULL;

    return begin;
}

Node *remove_all(Node *begin)
{
    while (begin = pop_front(begin))
        ; // Repeat pop_front() until the list becomes empty
    return begin;  // Now, begin is NULL
}

/*************************************************************************************/
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
        int x, y;
        if(n == 0) { //もし始点=終点だったら
            x = x0;
            y = y0;
        } else {
            x = x0 + i * (x1 - x0) / n;
            y = y0 + i * (y1 - y0) / n;
        }
        if(check_in_canvas(x, y)) {
            canvas[x][y] = '#';
        }
    }
}

void draw_rect(const int x0, const int y0, const int x1, const int y1) {
    draw_line(x0, y0, x0, y1); //左上→左下
    draw_line(x0, y1, x1, y1); //左下→右下
    draw_line(x1, y1, x1, y0); //右下→右上
    draw_line(x1, y0, x0, y0); //右上→左上
}

void draw_circle(const int ox, const int oy, const int radius) {
    const int n = 1000;
    const double theta = 2 * M_PI / n;
    int i;
    for(i = 0; i < n; i++) {
        const int x0 = ox + (int) (radius * cos(i * theta));
        const int y0 = oy + (int) (radius * sin(i * theta));
        const int x1 = ox + (int) (radius * cos((i + 1) * theta));
        const int y1 = oy + (int) (radius * sin((i + 1) * theta));

        draw_line(x0, y0, x1, y1);
    }
}

void save_history(const char *filename)
{
    if (filename == NULL)
        filename = default_history_file;

    FILE *fp;
    if ((fp = fopen(filename, "w")) == NULL) {
        fprintf(stderr, "error: cannot open %s.\n", filename);
        return;
    }

    Node *current = history;
    while(current != NULL) {
        fprintf(fp, "%s", current->str);
        current = current->next;
    }
    printf("saved as \"%s\"\n", filename);

    fclose(fp);
}

// Interpret and execute a command
//   return value:
//     0, normal commands such as "line", "rect", "circle"
//     1, unknown or special commands (not recorded in history[])
//     2, quit, exit

int interpret_command(const char *command)
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

    if(strcmp(s, "rect") == 0) {
        int x0, y0, x1, y1;
        x0 = atoi(strtok(NULL, " "));
        y0 = atoi(strtok(NULL, " "));
        x1 = atoi(strtok(NULL, " "));
        y1 = atoi(strtok(NULL, " "));
        draw_rect(x0, y0, x1, y1);

        return 0;
    }

    if(strcmp(s, "circle") == 0) {
        int ox, oy, radius;
        ox = atoi(strtok(NULL, " "));
        oy = atoi(strtok(NULL, " "));
        radius = atoi(strtok(NULL, " "));
        draw_circle(ox, oy, radius);

        return 0;
    }

    if (strcmp(s, "save") == 0) {
        s = strtok(NULL, " ");
        save_history(s);

        return 1;
    }

    if (strcmp(s, "undo") == 0) {
        init_canvas();

        history = pop_back(history);
        Node *current = history;
        while(current != NULL) {
            interpret_command(current->str);
            current = current->next;
        }

        return 1;
    }

    if (strcmp(s, "quit") == 0 || strcmp(s, "exit")) {
        return 2;
    }

    printf("error: unknown command.\n");

    return 1;
}

int check_in_canvas(int x, int y) {
    return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT;
}
