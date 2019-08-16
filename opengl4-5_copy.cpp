/*
画像の取得と利用
現時点で得られているのは単なる「絵」であって，このままでは利用することができない．そこでこのボロノイ図の画像データを取得して，これを別な用途に利用することを考える．ここではマウスで指定した点に最も近い母点を検索する処理を考えよう．ボロノイ領域を母点ごとに色分けした画像が得られているので，マウスで画像をクリックした時のマウス位置を取得し，その位置に対応するピクセルの色を得ることができれば，色データをcolorToID()関数に与えることで，そのボロノイ領域の母点のID番号を得ることができる．そこでまず画像の色データを取得することを考える．
　OpenGLにはglReadPixels()関数が用意されており，これを用いるとウィンドウ内の任意の短距離領域の色データやデプスバッファをメモリに取り込むことができる．
　引数x,yには，取得したい短形領域の左下角の位置を与えるl．ウィンドウの，短形領域の左下すみの位置をピクセル数で表した座標値をxとyにセットする．次のwidthとheightには，取得したい短形領域の幅と高さをやはりピクセル数で与える．画像全体の色データなどを取得したい場合には，x = y = 0, width = window_width, height = window_heightとなる．formatには色々なデータを支持できるが，画像の色データを取得する婆には，GL_RGBAに与えることが一般的である．もし画像のデプスデータを得たい場合には，，ここがGL_DEPTH_COMPONENTに変わるにはGL_FLOATを支持する．最後のdataには，取得したデータを格納するメモリ領域（例えば配列）のポインタを与える．
*/
#include <GLUT/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


// 円周率
#define PI 3.141592653589793

// ウィンドウの初期位置と初期サイズ
#define INIT_X_POS 128
#define INIT_Y_POS 128
#define INIT_WIDTH 512
#define INIT_HEIGHT 512

// ウィンドウの縦横サイズ
unsigned int window_width, window_height;

// 画像の表示範囲
double left, right, bottom, top;

// 座標値参照の際のインデックス
#define X 0
#define Y 1

// 点
#define MAX_NUM_POINTS 10000
double point[MAX_NUM_POINTS][2];
unsigned int num_points;

// 画像の最大サイズ，画像の最大ピクセル数を2048x1024と家庭
#define MAX_WIDTH 2048
#define MAX_HEIGHT 1024

// 画像データ
GLubyte color_data[MAX_WIDTH * MAX_HEIGHT][4];
//検出された母点
int detected_point_index = -1;

// ボロノイ母点の生成
// [0, window_width]×[0, window_height]の範囲内にランダムにnum個の点を生成し，これ母点とする
void genPoints(unsigned int num)
// unsigned int num; 生成される点の個数
{
	double x, y;

	// numがMAX_NUM_POINTSを超える場合には，それいかに修正
	if(num > MAX_NUM_POINTS) {
		printf("Too many points to generate.");
		num = MAX_NUM_POINTS;
	}

	// 点はウィンドウ全体にばらまかれるように生成
	num_points = 0;
	while(true) {
		if(num_points >= num) {
			left = bottom = 0.0;
			right = (double)window_width;
			top = (double)window_height;
			return;
		}
		x = window_width * ((double)rand() / (double)RAND_MAX);
		y = window_height * ((double)rand() / (double)RAND_MAX);
		point[num_points][X] = x;
		point[num_points][Y] = y;
		num_points++;
	}
}

// ボロノイ母点の表示
void displayPoints(void)
{
	unsigned int i;
	glPointSize(4.0f);
	glBegin(GL_POINTS);
	for(i = 0; i < num_points; i++) {
		if(i == detected_point_index)
			glColor3d(1.0, 1.0, 0.0); //検出点のみ色を変える
		else
			glColor3d(1.0, 0.0, 0.0);
		glVertex3d(point[i][X], point[i][Y], 0.0);
	}
	glEnd();
}

void wavePoints(void)
{
	time_t t = time(NULL);
	struct tm *mytm = localtime(&t);
	for(int i = 0; i < num_points; i++) {
		double mytime = mytm->tm_sec * i * PI / 3600;
		point[i][X] += 3.1 * cos(mytime);
		point[i][Y] += 3.5 * sin(mytime);
	}
}

// ボロノイ母点下に配置された円錐形の描画
void displayCone(double peak_point[])
// double peak_point[]; xy平面上の点の座標．これを頂点とする円錐をxy平面の下側に描く
{
	int i;
	double x, y, radius;

	// ウィンドウ内に中心を持つ円がウィンドウを覆い尽くすために必要な半径
	radius = sqrt((double)(window_width * window_width + window_height * window_height)) * 1.1;

	// peak_pointを頂点とする円錐を描く
	glBegin(GL_TRIANGLE_FAN);
	glVertex3d(peak_point[X], peak_point[Y], 0.0);
	for(i = 0; i<= 360; i++) {
		x = radius * cos((i % 360) / 180.0 * PI);
		y = radius * sin((i % 360) / 180.0 * PI);
		glVertex3d(peak_point[X] + x, peak_point[Y] + y, (- radius));
	}
	glEnd();
}

// OpenGL関係の初期設定
void initGL(void)
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // 背景色は白
	glEnable(GL_DEPTH_TEST);							// デプスバッファ機能を使用
	glClearDepth(1.0);
	glDepthFunc(GL_LESS);
	glDisable(GL_LIGHTING); //　照明は用いない
}

//　指定したIDに対応した色(r, g, b)を与える処理,８ビットごとに分割
void IDToColor(unsigned int id)
// unsigned int id; ID値
{
	GLubyte r, g, b;
	unsigned int tmp;
	r = (GLubyte)(id / 65536); // 65536 = 256 * 256
	tmp = id % 65536;
	g = (GLubyte)(tmp / 256);
	b = (GLubyte)(tmp % 256);
	glColor3ub(r, g, b);
}

// 色から対応するIDを得る処理
unsigned int colorToID(GLubyte r, GLubyte g, GLubyte b)
// GLubyte r, g, b; 色データ
{
	return(r * 65536 + g * 256 + b);
}

// 円錐形状群の表示
void displayCones(void)
{
	unsigned int i;
	for (i = 0; i < num_points; i++) {
		IDToColor(i);
		displayCone(point[i]);
	}
}

// 表示モード
#define DISPLAY_POINTS 0
#define DISPLAY_CONES 1
#define DISPLAY_AUTO 2
unsigned int display_mode = DISPLAY_POINTS;

// 表示
void display(void)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(left, right, bottom, top, -1.0, 2000.0);
	glViewport(0, 0, window_width, window_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	switch(display_mode) {
		case DISPLAY_POINTS:
			displayPoints();
			break;
		case DISPLAY_CONES:
			displayCones();
			glReadPixels(0, 0, window_width, window_height, GL_RGBA, GL_UNSIGNED_BYTE, color_data);
			// displayPoints();
			break;
		case DISPLAY_AUTO:
			wavePoints();
			displayCones();
			break;
		default:
			break;
	}
	glFlush();
}



// キー処理
void keyboard(unsigned char key, int x, int y)
{
	switch(key){
		case 'q':
		case 'Q':
		case '\033':
			exit(0);
		case 'g':
		case 'G':
			genPoints(1000);
			display_mode = DISPLAY_POINTS;
			glutPostRedisplay();
			break;

		// ボロノイ図の生成
		case 'v':
		case 'V':
			display_mode = DISPLAY_CONES;
			glutPostRedisplay();
			break;
		case 'a':
		case 'A':
			display_mode = DISPLAY_AUTO;
			glutPostRedisplay();
			break;
		default:
			break;
	}
}

//マウスのボタン操作
void mouse_button(int button, int state, int x, int y)
{
	unsigned int shift, index;
	switch(button) {
		//マウスの左ボタンを押すと最寄の点を取得
		case GLUT_LEFT_BUTTON:
			if(state == GLUT_DOWN) {
				shift = window_width * (window_height - y) + x;
				index = colorToID(color_data[shift][0], color_data[shift][1], color_data[shift][2]);
				if(index < num_points){
					detected_point_index = index;
					printf("Nearest point: %d\n", detected_point_index);
				}
				glutPostRedisplay();
				break;
			}
		default:	break;
	}
}
// ウィンドウサイズの変更
void resize(int width, int height)
{
	unsigned int old_width, old_height;
	double d;

	// 現在のウィンドウサイズを確保しておく．
	old_width = window_width;
	old_height = window_height;

	// ウィンドウサイズを大域変数にしまい直す
	window_width  = width;
	window_height = height;

	// ウィンドウサイズの変数ぶんを実サイズに置き換えて，描画範囲を補正．
	d = ((int)window_width - (int)old_width) * 0.5;
	right += d;
	left -= d;
	d = ((int)window_height - (int)old_height) * 0.5;
	top += d;
	bottom -= d;
}

int main(int argc, char *argv[])
{
	glutInitWindowPosition(INIT_X_POS, INIT_Y_POS);
	glutInitWindowSize(INIT_WIDTH, INIT_HEIGHT);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow("Voronoi Diagram");
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(resize);
	glutMouseFunc(mouse_button);
	initGL();
	glutMainLoop();
	return 0;
}