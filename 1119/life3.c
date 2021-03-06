#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int HEIGHT = 0;
int WIDTH = 0;

int** cell;

void init_cells(FILE* src)
{
    while(fgetc(src) != '\n') { //列数を調べる
        WIDTH++;
    }
    fseek(src, 0L, SEEK_SET);

    int c;
    while((c = fgetc(src)) != EOF) { //行数を調べる
        if(c == '\n') {
            HEIGHT++;
        }
    }
    fseek(src, 0L, SEEK_SET);

    cell = (int**) malloc(sizeof(int*) * (size_t) HEIGHT);

    int i, j;

    for (i = 0; i < HEIGHT; i++) {
        cell[i] = (int*) malloc(sizeof(int) * (size_t) WIDTH);
        j = 0;
        while((c = fgetc(src)) != EOF) {
            if(c == '\n') {
                break;
            }
            cell[i][j] = (c == '#') ? 1 : 0;
            j++;
        }
    }
}

void delete_cells() {
    for(int i = 0; i < HEIGHT; i++) {
        free(cell[i]);
    }
    free(cell);
}

void print_cells(FILE *fp)
{
    int i, j;

    fprintf(fp, "----------\n");

    for (i = 0; i < HEIGHT; i++) {
        for (j = 0; j < WIDTH; j++) {
            const char c = (cell[i][j] == 1) ? '#' : ' ';
            fputc(c, fp);
        }
        fputc('\n', fp);
    }
    fflush(fp);

    sleep(1);
}

int count_adjacent_cells(int i, int j)
{
    int n = 0;
    int k, l;
    for (k = i - 1; k <= i + 1; k++) {
        if (k < 0 || k >= HEIGHT) continue;
        for (l = j - 1; l <= j + 1; l++) {
            if (k == i && l == j) continue;
            if (l < 0 || l >= WIDTH) continue;
            n += cell[k][l];
        }
    }
    return n;
}

void update_cells()
{
    int i, j;
    int cell_next[HEIGHT][WIDTH];

    for (i = 0; i < HEIGHT; i++) {
        for (j = 0; j < WIDTH; j++) {
            cell_next[i][j] = 0;
            const int n = count_adjacent_cells(i, j);
            cell_next[i][j] = cell[i][j];
            if(!(n ==2 || n == 3)) {
                cell_next[i][j] = 0;
            }
            if(n == 3) {
                cell_next[i][j] = 1;
            }
        }
    }

    for (i = 0; i < HEIGHT; i++) {
        for (j = 0; j < WIDTH; j++) {
            cell[i][j] = cell_next[i][j];
        }
    }
}

int main()
{
    int gen;
    FILE *src, *fp;

    if((src = fopen("input.txt", "r")) == NULL) {
        fprintf(stderr, "error: cannot open a file.\n");
        return 1;
    }
    init_cells(src);
    fclose(src);

    if ((fp = fopen("cells.txt", "w")) == NULL) {

        fprintf(stderr, "error: cannot open a file.\n");
        return 1;
    }

    print_cells(fp);

    for (gen = 1;; gen++) {
        printf("generation = %d\n", gen);
        update_cells();
        print_cells(fp);
    }

    delete_cells();
    fclose(fp);
}
