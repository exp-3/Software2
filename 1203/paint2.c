#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>

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

Node *history = NULL;
const char *default_history_file = "history.txt";

/************************************************************************/
typedef struct {
    uint8_t a, b, g, r;
} pixel;

pixel canvas[WIDTH][HEIGHT];
pixel alpha_buffer[WIDTH][HEIGHT]; //alphaブレンドするための一時バッファ

pixel _color = {255, 255, 255, 255};
uint32_t parse_color_code();
pixel color_code_to_pixel(uint32_t code);
void set_color(pixel p);

/************************************************************************/
void print_canvas(FILE *fp);
void write_canvas(const int x, const int y, const pixel p); //canvasに直接書き込むための関数
void write_alpha_buffer(const int x, const int y, const pixel p); //長方形や円を描くときは、alpha_bufferへの書き込み→canvasにそれをブレンドという手順を踏む
void brend_alpha_buffer_to_canvas(); //alpha_bufferをcanvasにブレンド
void init_canvas();
void clear_canvas();
void clear_alpha_buffer();
int max(const int a, const int b);
int check_within_canvas(const int x, const int y);
int is_wall_at_alpha_buffer(const int x, const int y);
int is_wall_at_canvas(const int x, const int y);
int is_wall(const int x, const int y);
uint32_t parse_color_code();

/*************************************************************************/
int interpret_command(const char *command);

/**********************************************************************/
// 図形の描画
void draw_point(const int x, const int y);
void draw_line(const int x0, const int y0, const int x1, const int y1);
void draw_rect(const int x0, const int y0, const int x1, const int y1);
void draw_circle(const int x, const int y, const int radius);

// エフェクト
void fill(const int x, const int y);
void grayscale();
void gradient(const int degree, const uint32_t code1, const uint32_t code2);

// ファイル入出力
int load_data(const char *filename);
int save_history(const char *filename);
int inport_data(const char *filename);
int export_data(const char *filename);

// それ以外
void info();

/*************************************************************************/
int main()
{
    const char *filename = "canvas.txt";
    FILE *fp;
    char buf[BUFSIZE];

    if ((fp = fopen(filename, "ab")) == NULL) {
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

    if(history != NULL) { //もし履歴が残っていたら
        history = remove_all(history);
    }
    return 0;
}

/***********************************************************************/
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
    if(begin == NULL) {
        fprintf(stderr, "Don't call pop_front() when the list is empty.\n");
        return begin;
    }

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
    if(begin == NULL) {
        fprintf(stderr, "Don't call pop_back() when the list is empty.\n");
        return begin;
    }

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
    while((begin = pop_front(begin))) {
        ; // Repeat pop_front() until the list becomes empty
    }
    return begin;  // Now, begin is NULL
}

/***********************************************************************/
void print_canvas(FILE *fp)
{
    int x, y;

    brend_alpha_buffer_to_canvas(); //今回の更新によるalphaバッファへの書き込みをcanvasにブレンド
    clear_alpha_buffer(); //次の周のためにクリア

    fprintf(fp, "----------\n");

    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
            /*char c = ' ';
            if(grayscale > 0) {
                c = '#';
            }
            fputc(c, fp);*/
            const char c = ' ';
            fprintf(fp, "\033[48;2;%d;%d;%dm%c\033[0m",
                    canvas[x][y].r, canvas[x][y].g, canvas[x][y].b, c);
        }
        fputc('\n', fp);
    }
    fflush(fp);
}

void write_canvas(const int x, const int y, const pixel p) {
    canvas[x][y] = p;
}

void write_alpha_buffer(const int x, const int y, const pixel p) {
    alpha_buffer[x][y] = p;
}

void brend_alpha_buffer_to_canvas() {
    int x, y;
    for(x = 0; x < WIDTH; x++) {
        for(y = 0; y < HEIGHT; y++) {
            int dr, dg, db, da; //destination pixel
            int sr, sg, sb, sa; //source pixel
            int rr, rg, rb, ra; //result pixel

            dr = canvas[x][y].r;       dg = canvas[x][y].g;
            db = canvas[x][y].b;       da = canvas[x][y].a;
            sr = alpha_buffer[x][y].r; sg = alpha_buffer[x][y].g;
            sb = alpha_buffer[x][y].b; sa = alpha_buffer[x][y].a;

            //アルファブレンドに用いる計算式は以下のサイトを参考にした。
            //http://neareal.net/index.php?ComputerGraphics%2FImageProcessing%2FAlphaBlending
            ra = (255 * sa + (256 - sa) * da) / 255;
            rr = rg = rb = 0;
            if(ra != 0) {
                rr = ((sr * sa * 255 + dr * (255 - sa) * da) / ra) / 255;
                rg = ((sg * sa * 255 + dg * (255 - sa) * da) / ra) / 255;
                rb = ((sb * sa * 255 + db * (255 - sa) * da) / ra) / 255;
            }

            pixel p = {(uint8_t) ra, (uint8_t) rb, (uint8_t) rg, (uint8_t) rr};
            write_canvas(x, y, p);
        }
    }
}

void init_canvas()
{
    pixel p = {255, 255, 255, 255};
    set_color(p);
    clear_canvas();
    clear_alpha_buffer();
}

void clear_canvas() {
    pixel zero = {0, 0, 0, 0};
    int x, y;
    for(x = 0; x < WIDTH; x++) {
        for(y = 0; y < HEIGHT; y++) {
            write_canvas(x, y, zero);
        }
    }
}

void clear_alpha_buffer() {
    pixel zero = {0, 0, 0, 0};
    int x, y;
    for(x = 0; x < WIDTH; x++) {
        for(y = 0; y < HEIGHT; y++) {
            write_alpha_buffer(x, y, zero);
        }
    }
}

uint32_t parse_color_code() {
    char *s = strtok(NULL, " ");
    uint32_t code = 0;
    if(s[0] == '#') {
        char str[11] = "0x";
        strcat(str, &s[1]);
        code = (uint32_t) strtoul(str, NULL, 16);
        if(strlen(s) == 7) { //もしアルファ値がなく、#(6桁の値)だったら
            code = (code << 8) | 0xff;
        }
    }

    return code;
}

pixel color_code_to_pixel(uint32_t code) {
    uint8_t color[4];
    for(int i = 0; i < 4; i++) {
        color[i] = (uint8_t) ((code >> 8 * i) & 0xff);
    }
    pixel p;
    p.a = color[0];
    p.b = color[1];
    p.g = color[2];
    p.r = color[3];

    return p;
}

void set_color(pixel p) {
    _color = p;
}

int max(const int a, const int b)
{
    return (a > b) ? a : b;
}

int check_within_canvas(int x, int y) {
    return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT;
}


int is_wall_at_canvas(const int x, const int y) {
    if(canvas[x][y].a != 0) {
        return 1;
    }
    return 0;
}

int is_wall_at_alpha_buffer(const int x, const int y) {
    if(alpha_buffer[x][y].a != 0) {
        return 1;
    }
    return 0;
}

int is_wall(const int x, const int y) {
    return (is_wall_at_canvas(x,y) || is_wall_at_alpha_buffer(x, y));
}

/*********************************************************************/
// Interpret and execute a command
//   return value:
//     0, normal commands such as "line", "rect", "circle", "load"
//     1, unknown or special commands (not recorded in history[])
//     2, quit, exit
int interpret_command(const char *command)
{
    char buf[BUFSIZE];
    strcpy(buf, command);
    buf[strlen(buf) - 1] = 0; // remove the newline character at the end

    const char *s = strtok(buf, " ");

    // 図形の描画
    if(strcmp(s, "point") == 0) {
        int x, y;
        x = atoi(strtok(NULL, " "));
        y = atoi(strtok(NULL, " "));
        draw_point(x, y);

        return 0;
    }

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

    // エフェクト
    if(strcmp(s, "fill") == 0) {
        int x, y;
        x = atoi(strtok(NULL, " "));
        y = atoi(strtok(NULL, " "));
        fill(x, y);

        return 0;
    }

    //グレースケール変換
    if(strcmp(s, "grayscale") == 0) {
        grayscale();

        return 0;
    }

    //グラデーション
    if(strcmp(s, "gradient") == 0) {
        int degree = atoi(strtok(NULL, " "));
        uint32_t code1 = parse_color_code();
        uint32_t code2 = parse_color_code();
        gradient(degree, code1, code2);

        return 0;
    }

    // ファイルの入出力
    if(strcmp(s, "load") == 0) {
        s = strtok(NULL, " ");
        if(load_data(s)) { //もしロードに失敗したら
            return 1;
        }

        return 0;
    }

    if (strcmp(s, "save") == 0) {
        s = strtok(NULL, " ");
        save_history(s);

        return 1;
    }

    if(strcmp(s, "export") == 0) {
        s = strtok(NULL, " ");
        export_data(s);

        return 1;
    }

    // それ以外
    if(strcmp(s, "color") == 0) {
        uint32_t code = parse_color_code();
        set_color(color_code_to_pixel(code));

        return 0;
    }

    if (strcmp(s, "undo") == 0) {
        init_canvas();

        history = pop_back(history);
        Node *current = history;
        while(current != NULL) {
            interpret_command(current->str);
            brend_alpha_buffer_to_canvas();
            clear_alpha_buffer();
            current = current->next;
        }

        return 1;
    }

    if(strcmp(s, "clear") == 0) {
        clear_canvas();

        return 0;
    }

    if (strcmp(s, "quit") == 0 || strcmp(s, "exit") == 0) {
        return 2;
    }

/*    if(strcmp(s, "info") == 0) {
        info();

        int x = atoi(strtok(NULL, " "));
        int y = atoi(strtok(NULL, " "));
        printf("canvas[%d][%d] = %x, %x, %x, %x\n", x, y,
               canvas[x][y].r, canvas[x][y].g, canvas[x][y].b, canvas[x][y].a);

        return 1;
    }
*/
    printf("error: unknown command.\n");

    return 1;
}

// 図形の描画
void draw_point(const int x, const int y) {
    if(check_within_canvas(x, y)) {
        write_alpha_buffer(x, y, _color); //現在セットされているカラーをalphaバッファに書き込み
    }
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
        draw_point(x, y);
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

// エフェクト

//塗りつぶし
void fill(const int x, const int y) { //幅優先探索で壁に当たるまで塗りつぶし
    if(!check_within_canvas(x, y) || is_wall(x, y)) { //範囲内ではないor壁
        return;
    }

    draw_point(x, y);
    int dx[] = {0, -1, 0, 1};
    int dy[] = {1, 0, -1, 0};

    for(int i = 0; i < 4; i++) {
        fill(x + dx[i], y + dy[i]);
    }
}

//グレースケール変換
void grayscale() {
    int x, y;
    for(x = 0; x < WIDTH; x++) {
        for(y = 0; y < HEIGHT; y++) {
            //NTSC Coef. method
            uint8_t g = (uint8_t) (0.298912 * canvas[x][y].r
                                   + 0.586611 * canvas[x][y].g
                                   + 0.114478 * canvas[x][y].b);
            pixel p = {canvas[x][y].a, g, g, g};
            write_canvas(x, y, p);
        }
    }
}

//グラデーション
void gradient(int degree, uint32_t code1, uint32_t code2) {
    //グラデーションの方向によって始点、終点、方向を場合分け
    int startx[] = {0, WIDTH - 1, WIDTH - 1, 0};
    int starty[] = {HEIGHT - 1, HEIGHT - 1, 0, 0};
    int endx[] = {WIDTH - 1, 0, 0, WIDTH - 1};
    int endy[] = {0, 0, HEIGHT - 1, HEIGHT - 1};
    int dx[] = {1, -1, -1, 1};
    int dy[] = {-1, -1, 1, 1};

    if(degree < 0) {
        degree = 360 - (-degree % 360);
    } else {
        degree %= 360;
    }

    int quad = degree / 90; //どの象限へ進んでゆくグラデーションなのか
    double theta = (double) degree * 2 * M_PI / 360;
    double ex = cos(theta);
    double ey = sin(theta);

    //隅までの長さ、後で二色の内分を取る際の規格化に必要
    double k = sqrt((endx[quad] - startx[quad]) * ex + -(endy[quad] - starty[quad]) * ey);

    pixel p1, p2;
    p1 = color_code_to_pixel(code1);
    p2 = color_code_to_pixel(code2);
    int x, y;
    for(x = startx[quad]; x != endx[quad] + dx[quad]; x += dx[quad]) {
        for(y = starty[quad]; y != endy[quad] + dy[quad]; y += dy[quad]) {
            double l = sqrt((x - startx[quad]) * ex + -(y - starty[quad]) * ey);
            pixel p;

            //二色の内分を取る
            p.r = (uint8_t) (p1.r + ((double) p2.r - (double) p1.r) * l / k);
            p.g = (uint8_t) (p1.g + ((double) p2.g - (double) p1.g) * l / k);
            p.b = (uint8_t) (p1.b + ((double) p2.b - (double) p1.b) * l / k);
            p.a = (uint8_t) (p1.a + ((double) p2.a - (double) p1.a) * l / k);

            write_alpha_buffer(x, y, p);
        }
    }
}

// ファイルの入出力関連
int load_data(const char *filename) {
    if(filename == NULL) {
        printf("lack of filename!\n");
        return 1;
    }

    FILE *fp;
    if((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "error: cannot open %s.\n", filename);
        return 1;
    }

    char buff[BUFSIZE];
    while(fgets(buff, BUFSIZE, fp) != NULL) {
        interpret_command(buff);
    }

    fclose(fp);

    return 0;
}

int save_history(const char *filename)
{
    if (filename == NULL) {
        filename = default_history_file;
    }

    FILE *fp;
    if ((fp = fopen(filename, "w")) == NULL) {
        fprintf(stderr, "error: cannot open %s.\n", filename);
        return 1;
    }

    Node *current = history;
    while(current != NULL) {
        fprintf(fp, "%s", current->str);
        current = current->next;
    }
    printf("saved as \"%s\"\n", filename);

    fclose(fp);

    return 0;
}

// bmp形式でexport

// bmp形式での書き出しについては、以下のサイトを参考にした。
// http://www.mm2d.net/main/prog/c/image_io-11.html

// 環境に依存せずにリトルエンディアンでバイナリを出力するために、
// バイトストリームを用いる。
typedef struct {
    uint8_t *buffer;
    int offset;
} byte_stream;

void bs_init(uint8_t *s, byte_stream *bs) {
    bs->buffer = s;
    bs->offset = 0;
}

void bs_write32(byte_stream *bs, uint32_t data) {
    //32bitのデータをリトルエンディアンで格納
    int i;
    uint8_t *dst = &(bs->buffer[bs->offset]);
    for(i = 0; i < 4; i++) {
        *(dst++) = (uint8_t) ((data >> 8 * i) & 0xff);
    }
    bs->offset += 4;
}

void bs_write16(byte_stream *bs, uint16_t data) {
    //16bitのデータをリトルエンディアンで格納
    int i;
    uint8_t *dst = &(bs->buffer[bs->offset]);
    for(i = 0; i < 2; i++) {
        *(dst++) = (uint8_t) ((data >> 8 * i) & 0xff);
    }
    bs->offset += 2;
}

void bs_write8(byte_stream *bs, uint8_t data) {
    //8bitのデータをbsに格納
    bs->buffer[bs->offset] = data;
    bs->offset++;
}

/***********************************************************************/
int export_data(const char *filename) {
    byte_stream bs;

    int file_header_size, info_header_size, header_size;
    uint8_t *header;

    int pad;              /* 32bit境界のためのパディング    */
    int imgsize;          /* イメージのサイズ               */
    uint8_t *buf;         /* 画像データとその先頭ポインタ   */

    FILE *fp;
    if((fp = fopen(filename, "wb")) == NULL) {
        fprintf(stderr, "error: cannot open %s.\n", filename);
        return 1;
    }

    /* ------------------------- ヘッダ作成 ---------------------------- */
    file_header_size = 14;
    info_header_size = 40;
    header_size = file_header_size + info_header_size;
    pad = (4 - ((WIDTH * 3) % 4)) % 4;    /* 32bit境界条件によるパディング  */
    imgsize = (WIDTH * 3 + pad) * HEIGHT; /* 出力されるデータサイズ         */

    header = (uint8_t*) malloc(sizeof(uint8_t) * (size_t) (header_size));
    if(header == NULL) { //メモリ確保失敗
        fclose(fp);
        return 1;
    }
    bs_init(header, &bs);

    // BMPFILEHEADER
    bs_write16(&bs, (uint16_t) 0x4d42);                  //bfType
    bs_write32(&bs, (uint32_t) (header_size + imgsize)); //bfSize
    bs_write16(&bs, 0);                                  //bfReserved1
    bs_write16(&bs, 0);                                  //bfReserved2
    bs_write32(&bs, (uint32_t) (header_size));           //bfOffBits

    //BMPINFOHEADER
    bs_write32(&bs, (uint32_t) info_header_size);
    bs_write32(&bs, WIDTH);
    bs_write32(&bs, HEIGHT);
    bs_write16(&bs, 1);
    bs_write16(&bs, 24);
    bs_write32(&bs, 0);
    bs_write32(&bs, (uint32_t) imgsize);
    bs_write32(&bs, 0);
    bs_write32(&bs, 0);
    bs_write32(&bs, 0);
    bs_write32(&bs, 0);

    /* ------------------------- ヘッダ出力 ---------------------------- */
    if(fwrite((void*)header, 1, (size_t) header_size, fp) != (size_t) header_size) {
        //書き込み失敗
        fprintf(stderr, "error: cannot write %s.\n", filename);
        free(header);
        fclose(fp);
        return 1;
    }
    free(header);

    /* ------------------------- 画像データ作成 ------------------------- */
    buf = (uint8_t*) malloc(sizeof(uint8_t) * (size_t) imgsize);
    if(buf == NULL) { //メモリ確保失敗
        fprintf(stderr, "error: cannot allocate memory.\n");
        fclose(fp);
        return 1;
    }

    bs_init(buf, &bs);
    for(int y = HEIGHT - 1; y >= 0; y--){
        for(int x = 0; x < WIDTH; x++){
            bs_write8(&bs, canvas[x][y].b);
            bs_write8(&bs, canvas[x][y].g);
            bs_write8(&bs, canvas[x][y].r);
        }
        for(int i = 0; i < pad; i++) {
            bs_write8(&bs, 0);
        }
    }

    /* ------------------------- 画像データ出力 ------------------------ */
    if(fwrite((void*) buf, 1, (size_t) imgsize, fp) != (size_t) imgsize) {
        //書き込み失敗
        free(buf);
        fclose(fp);
        return 1;
    }

    /* ------------------------- すべて成功 ---------------------------- */
    printf("exported as \"%s\"\n", filename);
    free(buf);
    fclose(fp);

    return 0;
}

// それ以外
void info() {
    printf("color: r = %d, g = %d, b = %d, alpha = %d\n", _color.r, _color.g, _color.b, _color.a);
}
