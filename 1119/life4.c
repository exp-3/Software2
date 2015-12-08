/**
 * 自主課題では、感染症の拡散過程をイメージしてライフゲームの規則を書き換えてみた。
 * ルールとしては、感染者が周囲に病原体を放出し、
 * 閾値以上の病原体に晒された健康な人間もまた感染者となる、というものだ。
 * このルールでライフゲームを動かしてみると、
 * 人口密度が高ければ高いほど、爆発的に感染者は広がった。
 * 逆にそれ以外の要素(感染するのに必要な閾値、病原体の拡散範囲、初期の感染者の割合)などは
 * 人口密度と比べて、それほど感染速度や感染範囲に影響することはなかった。
 * このことから、感染病に対して最も有効なことは、人口密度の多い地域に感染者が
 * 立ち入らないようにすることである、と結論付けられる。
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int HEIGHT = 40;
int WIDTH = 50;
int THRESHOLD = 3; //感染する閾値
int RANGE = 3; //ウィルスの拡散範囲
double NORMAL_RATIO = 0.2; //人口密度
double INFECTED_RATIO = 0.1; //全人口のうち、初期の感染者の割合

typedef struct {
    int isPerson;
    int isInfected;
    int count;
} state;

int** field; //その場所にどれくらいウィルスが存在するか
state** cell; //その場所の人の状態

void init_cells();
void delete_cells();
void print_cells(FILE* fp);
void update_cells();
int count_total_people();
int count_infected_people();
void show_data(int gen);

void init_cells() {
    field = (int**) malloc(sizeof(int*) * (size_t) HEIGHT);
    cell = (state**) malloc(sizeof(state*) * (size_t) HEIGHT);

    int i, j;

    for (i = 0; i < HEIGHT; i++) {
        field[i] = (int*) malloc(sizeof(int) * (size_t) WIDTH);
        cell[i] = (state*) malloc(sizeof(state) * (size_t) WIDTH);
        for(j = 0; j < WIDTH; j++) {
            cell[i][j].isPerson = 0;
            cell[i][j].isInfected = 0;
            cell[i][j].count = 0;
        }
    }

    for(int n = 0; n < WIDTH * HEIGHT * NORMAL_RATIO; n++) {
        i = rand() % HEIGHT;
        j = rand() % WIDTH;
        cell[i][j].isPerson = 1;
        if((double) rand() / RAND_MAX < INFECTED_RATIO) {
            cell[i][j].isInfected = 1;
        }
    }
}

void delete_cells() {
    for(int i = 0; i < HEIGHT; i++) {
        free(field[i]);
        free(cell[i]);
    }
    free(field);
    free(cell);
}

void print_cells(FILE *fp)
{
    int i, j;

    fprintf(fp, "----------\n");

    for (i = 0; i < HEIGHT; i++) {
        for (j = 0; j < WIDTH; j++) {
            char c = ' ';
            if(cell[i][j].isPerson) {
                if(cell[i][j].isInfected) {
                    c = '*';
                } else {
                    c = '#';
                }
            }
            fputc(c, fp);
        }
        fputc('\n', fp);
    }
    fflush(fp);
}

void show_data(int gen) {
    printf("\033[1;1H"); //カーソルを左上に
    printf("\033[2J");
    printf("generation = %d\n", gen);
    printf("%d / %d people infected\n", count_infected_people(), count_total_people());
    printf("\033[32m#\033[39m: healthy person. \033[31m*\033[39m: infected person.\n");
    printf("--------------------\n");
    for(int i = 0; i < HEIGHT; i++) {
        for(int j = 0; j < WIDTH; j++) {
            char c = ' ';
            if(cell[i][j].isPerson) {
                if(cell[i][j].isInfected) {
                    c = '*';
                    printf("\033[31m"); //red
                } else {
                    c = '#';
                    printf("\033[32m"); //green
                }
            }
            printf("%c", c);
            printf("\033[39m"); //neutral color
        }
        printf("\n");
    }
    fflush(stdout);
}

void update_cells()
{
    int i, j;

    for(i = 0; i < HEIGHT; i++) {
        for(j = 0; j < WIDTH; j++) {
            field[i][j] = 0;
        }
    }
    for(i = 0; i < HEIGHT; i++) {
        for(j = 0; j < WIDTH; j++) {
            if(cell[i][j].count > 0) { //だんだん病原菌が抜けていく
                cell[i][j].count--;
            }
            if(cell[i][j].isInfected) {
                for(int k = i - RANGE; k <= i + RANGE; k++) {
                    if(k < 0 || k >= HEIGHT) continue;
                    for(int l = j - RANGE; l <= j + RANGE; l++) {
                        if(l < 0 || l >= WIDTH) continue;
                        field[k][l]++;
                    }
                }
            }
        }
    }

    for(i = 0; i < HEIGHT; i++) {
        for(j = 0; j < WIDTH; j++) {
            if(cell[i][j].isPerson) {
                cell[i][j].count += field[i][j];
                if(cell[i][j].count > THRESHOLD) {
                    cell[i][j].isInfected = 1;
                }
            }
        }
    }
}

int count_total_people() {
    int num = 0;
    int i, j;
    for(i = 0; i < HEIGHT; i++) {
        for(j = 0; j < WIDTH; j++) {
            if(cell[i][j].isPerson) {
                num++;
            }
        }
    }
    return num;
}

int count_infected_people() {
    int num = 0;
    int i, j;
    for(i = 0; i < HEIGHT; i++) {
        for(j = 0; j < WIDTH; j++) {
            if(cell[i][j].isPerson && cell[i][j].isInfected) {
                num++;
            }
        }
    }
    return num;
}

int main()
{
    int gen;
    FILE *fp;

    init_cells();

    if ((fp = fopen("cells.txt", "w")) == NULL) {
        fprintf(stderr, "error: cannot open a file.\n");
        return 1;
    }

    print_cells(fp);

    for (gen = 1;; gen++) {
        update_cells();
        show_data(gen);
        print_cells(fp);
        sleep(1);
    }

    delete_cells();
    fclose(fp);
}
