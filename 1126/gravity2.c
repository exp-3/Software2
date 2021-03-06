#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define WIDTH 75
#define HEIGHT 50

#define VECTORSIZE 2 //n次元の系

const double R = 2.0; // 融合する際の閾値半径
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
    { 1.0, {{0.0, 0.0}}, {{0.0, 0.0}} },
    { 1.0, {{-10.0, 0.0}}, {{0.0, 0.0}} },
    { 1.0, {{0.0, -10.0}}, {{0.0, 0.0}} },
    { 1.0, {{10.0, 0.0}}, {{0.0, 0.0}}}
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

/**************************************************************************/
int main(int argc, char *argv[])
{
    const char *filename = "space.txt";
    FILE *fp;
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
        if (i % 10 == 0) {
            plot_stars(fp, t);
            usleep(200 * 1000);
        }
    }

    fclose(fp);

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

    for( i = 0; i < nstars; i++) {
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

/* vectorの表示 **********************************************************/
void print_vector(const vector *v) {
    printf("(%7.2f", v->x[0]);
    for(int i = 1; i < VECTORSIZE; i++) {
        printf(", %7.2f", v->x[i]);
    }
    printf(")");
}

