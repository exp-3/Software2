/**
 * 今回の自主課題では、三次元への拡張と、四次のルンゲクッタ法の実装を行った。
 * ルンゲクッタ法を実装したコードについては、ファイル名gravity3_RK4.cとして別に添付する。
 * また、精度については、静止した二体を静かに離した時のシミュレーションによって確認した。
 * 本来、このソースコードだと重力によって引き合う二体は中点で融合しなければならない。
 * しかし、なんの工夫も施していないこのソースコードでは、dt = 0.09としてプログラムを動かした時に、
 * お互いが突き抜けて逆方向に飛んでいってしまった。
 * 一方で、ルンゲクッタ法を実装した方のプログラムでは、同じ条件で動かしても、しっかりと中点で
 * 二体が融合した。
 * シミュレーション結果については、このファイルの末尾に記した。
 * このことから、近似解の精度が上がっていることがわかる。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define WIDTH 75
#define HEIGHT 50
#define ZRANGE 1

#define VECTORSIZE 3 //n次元の系

const double R = 1.0; // 融合する際の閾値半径
const double G = 1.0; // gravity constant

typedef struct {
    double x[VECTORSIZE];
} vector;

struct star
{
    double m; // mass
    vector r; // position
    vector v; // velocity
};

struct star stars[] = {
    { 800.0, {{10.0, 0.0, 0.0}}, {{0.0, 0.0, 0.0}} },
    { 800.0, {{-10.0, 0.0, 0.0}}, {{0.0, 0.0, 0.0}}}
};

const int nstars = sizeof(stars) / sizeof(struct star);

/***************************************************************************/
void plot_stars(FILE *fp, const double t);
void update_velocities(const double dt);
void update_positions(const double dt);
vector add(const vector v1, const vector v2);
vector sub(const vector v1, const vector v2);
vector multi(const vector v, double k);
double norm(const vector v);
void print_vector(const vector *v);
FILE *gnuplot_open(); //gnuplotを開く
void gnuplot_close(FILE *gp); //gnuplotを閉じる
void gnuplot_stars(FILE *gp); //gnuplotに星をプロットする。


/**************************************************************************/
int main(int argc, char* argv[])
{
    const char *filename = "space.txt";
    FILE *gp, *fp;

    //gp = gnuplot_open();

    if ((fp = fopen(filename, "a")) == NULL) {
        fprintf(stderr, "error: cannot open %s.\n", filename);
        return 1;
    }

    double dt = 0.1;
    if(argc > 1) {
        dt = atof(argv[1]);
    }
    const double stop_time = 400;

    int i;
    double t;
    for (i = 0, t = 0; t <= stop_time; i++, t += dt) {
        update_positions(dt);
        //gnuplot_stars(gp);
        //usleep(10 * 1000);
        if (i % 10 == 0) {
            plot_stars(fp, t);
            usleep(200 * 1000);
        }
    }

    fclose(fp);
    //gnuplot_close(gp);
    return 0;
}

/* star書き出し ************************************************************/
void plot_stars(FILE *fp, const double t)
{
    int i;
    char space[WIDTH][HEIGHT];

    memset(space, ' ', sizeof(space));
    for (i = 0; i < nstars; i++) {
        if(stars[i].m == 0) continue;
        const int x = WIDTH  / 2 + (int) stars[i].r.x[0];
        const int y = HEIGHT / 2 + (int) stars[i].r.x[1];
        if (x < 0 || x >= WIDTH)  continue;
        if (y < 0 || y >= HEIGHT) continue;
        char c = 'o';
        if (stars[i].m >= 1.0) c = 'O';
        space[x][y] = c;
    }

    int x, y;
    fprintf(fp, "----------\n");
    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++)
            fputc(space[x][y], fp);
        fputc('\n', fp);
    }
    fflush(fp);

    printf("----t = %5.1f----\n", t);
    for (i = 0; i < nstars; i++) {
        printf("stars[%d]:\n\tr = ", i);
        print_vector(&stars[i].r);
        printf("\tv = ");
        print_vector(&stars[i].v);
        printf("\n");
    }
    printf("\n");
}

/* 速度更新 ****************************************************************/
void update_velocities(const double dt)
{
    int i, j;
    for (i = 0; i < nstars; i++) {
        if(stars[i].m == 0) continue;
        vector a = {{0.0, 0.0}}; //加速度

        for(j = 0; j < nstars; j++) {
            if(stars[j].m == 0 || i == j) continue;
            vector gap;
            gap = sub(stars[j].r, stars[i].r);
            double distance = norm(gap);
            a = add(a, multi(gap, stars[j].m / pow(distance, 3)));
        }
        a = multi(a, G);

        stars[i].v = add(stars[i].v, multi(a, dt));
    }
}

/* 座標更新 ****************************************************************/
void update_positions(const double dt)
{
    int i, j;
    vector vn[nstars];
    for(i = 0; i < nstars; i++) {
        vn[i] = stars[i].v;
    }

    update_velocities(dt);

    for (i = 0; i < nstars; i++) {
        if(stars[i].m == 0) continue;
        stars[i].r = add(stars[i].r, multi(vn[i], dt));
    }

    for(i = 0; i < nstars; i++) {
        if(stars[i].m == 0) continue;
        for(j = i + 1; j < nstars; j++) {
            if(stars[j].m == 0) continue;
            double distance = norm(sub(stars[i].r, stars[j].r));
            if(distance < R) {
                //座標を融合
                stars[i].r = multi(add(stars[i].r, stars[j].r), 0.5);

                //速度を融合 v = (m1 * v1 + m2 * v2) / (m1 + m2)
                stars[i].v = multi(add(multi(stars[i].v, stars[i].m),
                                       multi(stars[j].v, stars[j].m)),
                                   1 / (stars[i].m + stars[j].m));

                //質量を融合
                stars[i].m += stars[j].m;
                stars[j].m = 0;
            }
        }
    }

}

/* vectorの足し算 **********************************************************/
vector add(const vector v1, const vector v2) {
    vector result;
    for(int i = 0; i < VECTORSIZE; i++) {
        result.x[i] = v1.x[i] + v2.x[i];
    }

    return result;
}

/* vectorの引き算 **********************************************************/
vector sub(const vector v1, const vector v2) {
    vector result = add(v1, multi(v2, (-1)));
    return result;
}

/* vectorのk倍 *************************************************************/
vector multi(const vector v, double k) {
    vector result;
    for(int i = 0; i < VECTORSIZE; i++) {
        result.x[i] = v.x[i] * k;
    }

    return result;
}

/* vectorの計量を求める ****************************************************/
double norm(const vector v) {
    double result = 0.0;
    for(int i = 0; i < VECTORSIZE; i++) {
        result += v.x[i] * v.x[i];
    }
    result = sqrt(result);
    return result;
}

/* vectorの表示 ************************************************************/
void print_vector(const vector *v) {
    printf("(%7.2f", v->x[0]);
    for(int i = 1; i < VECTORSIZE; i++) {
        printf(", %7.2f", v->x[i]);
    }
    printf(")");
}

/* gnuplotの入出力を開く ***************************************************/
FILE *gnuplot_open() {
    FILE *gp;
    if((gp = popen("gnuplot", "w")) == NULL) {
        fprintf(stderr, "error: cannot open gnuplot.\n");
    }
    fprintf(gp, "set xrange [%e:%e]\n", (double) -WIDTH, (double) WIDTH);
    fprintf(gp, "set yrange [%e:%e]\n", (double) -HEIGHT, (double) HEIGHT);
    fprintf(gp, "set zrange [%e:%e]\n", (double) -ZRANGE, (double) ZRANGE);

    fprintf(gp, "set size square\n");

    return gp;
}

/* gnuplotの入出力を閉じる *************************************************/
void gnuplot_close(FILE *gp) {
    pclose(gp);
}

/* gnuplotに星の座標を出力 *************************************************/
void gnuplot_stars(FILE *gp) {
    fprintf(gp, "splot '-' pt 7 ps 2\n");
    for(int i = 0; i < nstars; i++) {
        if(stars[i]. m == 0) continue;
        fprintf(gp, "%e, %e, %e\n", stars[i].r.x[0], stars[i].r.x[1], stars[i].r.x[2]);
    }
    fprintf(gp, "e\n");
    fflush(gp);
}

/* 以下にシミュレーション結果を記す*/
/* gravity3.c (dt = 0.09)
----------
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                           O                   O                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
----------
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                            O                 O                            
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
----------
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                               O           O                               
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
----------
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                               O           O                               
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
----------
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           

*/
/* gravity3_RK4.c (dt = 0.09)
----------
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                            O                 O                            
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
----------
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                             O               O                             
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
----------
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                O         O                                
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
----------
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                     O                                     
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           

*/
