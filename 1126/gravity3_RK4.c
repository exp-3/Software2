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
void update_positions(const double dt);
vector add(const vector v1, const vector v2);
vector sub(const vector v1, const vector v2);
vector multi(const vector v, double k);
double norm(const vector v);
void print_vector(const vector *v);
void calca(vector *dr, vector *a, double dt); //加速度計算
FILE *gnuplot_open(); //gnuplotを開く
void gnuplot_close(FILE *gp); //gnuplotを閉じる
void gnuplot_stars(FILE *gp); //gnuplotに星をプロットする


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

/* 加速度計算 **************************************************************/
void calca(vector *dr, vector *a, double dt) {
    int i, j;

    vector r[nstars];
    for(i = 0; i < nstars; i++) {
        r[i] = add(stars[i].r, multi(dr[i], dt));
    }

    for (i = 0; i < nstars; i++) {
        if(stars[i].m == 0) continue;
        for(j = 0; j < VECTORSIZE; j++) {
            a[i].x[j] = 0;
        }

        for(j = 0; j < nstars; j++) {
            if(stars[j].m == 0 || i == j) continue;
            vector gap;
            gap = sub(r[j], r[i]);
            double distance = norm(gap);
            a[i] = add(a[i], multi(gap, stars[j].m / pow(distance, 3)));
        }
        a[i] = multi(a[i], G);
    }
}

/* 座標更新 ****************************************************************/
void update_positions(const double dt)
{
    int i, j;

    vector a[nstars];

    //四次のルンゲクッタ法
    vector dr1[nstars], dr2[nstars], dr3[nstars], dr4[nstars];
    vector dv1[nstars], dv2[nstars], dv3[nstars], dv4[nstars];

    //dr1, dv1の計算
    vector dr[nstars];
    for(i = 0; i < nstars; i++) {
        for(j = 0; j < VECTORSIZE; j++) {
            dr[i].x[j] = 0;
        }
    }
    calca(dr, a, 0);
    for(i = 0; i < nstars; i++) {
        dr1[i] = multi(stars[i].v, dt);
        dv1[i] = multi(a[i], dt);
    }

    //dr2, dv2の計算
    calca(dr1, a, dt / 2);
    for(i = 0; i < nstars; i++) {
        dr2[i] = multi(add(stars[i].v, multi(dv1[i], 0.5)), dt);
        dv2[i] = multi(a[i], dt);
    }

    //dr3, dv3の計算
    calca(dr2, a, dt / 2);
    for(i = 0; i < nstars; i++) {
        dr3[i] = multi(add(stars[i].v, multi(dv2[i], 0.5)), dt);
        dv3[i] = multi(a[i], dt);
    }

    //dr4, dv4の計算
    calca(dr3, a, dt);
    for(i = 0; i < nstars; i++) {
        dr4[i] = multi(add(stars[i].v, dv3[i]), dt);
        dv4[i] = multi(a[i], dt);
    }

    //r_(n + 1) = r_n + (dr1 + 2dr2 + 2dr3 + dr4) / 6
    //v_(n + 1) = v_n + (dv1 + 2dv2 + 2dv3 + dv4) / 6
    for(i = 0; i < nstars; i++) {
        stars[i].r = add(stars[i].r, multi(add(dr1[i], add(multi(dr2[i], 2), add(multi(dr3[i], 2), dr4[i]))), 1.0 / 6));
        stars[i].v = add(stars[i].v, multi(add(dv1[i], add(multi(dv2[i], 2), add(multi(dv3[i], 2), dv4[i]))), 1.0 / 6));
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

/* vectorの表示 **********************************************************/
void print_vector(const vector *v) {
    printf("(%7.2f", v->x[0]);
    for(int i = 1; i < VECTORSIZE; i++) {
        printf(", %7.2f", v->x[i]);
    }
    printf(")");
}

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

void gnuplot_close(FILE *gp) {
    pclose(gp);
}

void gnuplot_stars(FILE *gp) {
    fprintf(gp, "splot '-' pt 7 ps 2\n");
    //fprintf(gp, "splot '-'\n");
    for(int i = 0; i < nstars; i++) {
        if(stars[i].m == 0) continue;
        fprintf(gp, "%e, %e, %e\n", stars[i].r.x[0], stars[i].r.x[1], stars[i].r.x[2]);
    }
    fprintf(gp, "e\n");
    fflush(gp);
}
