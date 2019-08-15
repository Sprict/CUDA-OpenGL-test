#include <GLUT/glut.h>
// #include <GL/freeglut.h>
#include <math.h>
//#include "stdafx.h" Visual Studio 独自の定義

#define X 0
#define Y 1
#define Z 2

unsigned int num_points = 8;
double point[][3] = {
		{1.0, 1.0, -1.0},
		{-1.0, 1.0, -1.0},
		{-1.0, -1.0, -1.0},
		{1.0, -1.0, -1.0},
		{1.0, 1.0, 1.0},
		{-1.0, 1.0, 1.0},
		{-1.0, -1.0, 1.0},
		{1.0, -1.0, 1.0}};

// ウィンドウサイズを保持する大域変数
unsigned int window_width,
		window_height;

//アスペクト非の変更に伴う画像を必ず表示する範囲
double init_left = -2.0;
double init_right = 2.0;
double init_bottom = -2.0;
double init_top = 2.0;

double left, right, bottom, top;

double eye[3];
double center[3] = {0.0, 0.0, 0.0};
double up[3];

// 円周率
#define PI 3.141592653589793

double phi = 30.0;
double theta = 30.0;

//マウス処理
int mouse_old_x, mouse_old_y;
bool motion_p;

// 2本のベクトルvec0とvec1の内積
double dot(double vec0[], double vec1[])
{
	return (vec0[X] * vec1[X] + vec0[Y] * vec1[Y] + vec0[Z] * vec1[Z]);
}

// ２本のベクトルvec0とvec1の外戚
void cross(double vec0[], double vec1[], double vec2[])
{
	vec2[X] = vec0[Y] * vec1[Z] - vec0[Z] * vec1[Y];
	vec2[Y] = vec0[Z] * vec1[X] - vec0[X] * vec1[Z];
	vec2[Z] = vec0[X] * vec1[Y] - vec0[Y] * vec1[X];
}

// ベクトルの正規化(注意！このベクトルは破壊的に変更される)
void normalVec(double vec[])
{
	double norm;
	norm = sqrt(vec[X] * vec[X] + vec[Y] * vec[Y] + vec[Z] * vec[Z]);
	vec[X] /= norm;
	vec[Y] /= norm;
	vec[Z] /= norm;
}

// マウスのボタン処理
void mouse_button(int button, int state, int x, int y)
{
	if ((state == GLUT_DOWN) && (button == GLUT_LEFT_BUTTON))
		motion_p = true;
	else if (state == GLUT_UP)
		motion_p = false;
	mouse_old_x = x;
	mouse_old_y = y;
}

// マウスの移動処理
void mouse_motion(int x, int y)
{
	int dx, dy;
	dx = x - mouse_old_x;
	dy = y - mouse_old_y;
	if (motion_p)
	{
		phi -= dx * 0.2;
		theta += dy * 0.2;
	}
	mouse_old_x = x;
	mouse_old_y = y;
	glutPostRedisplay();
}
// 投影撮影用の行列定義，モデルを画面いっぱいに表示する
void defineViewMatrix(double phi, double theta)
{
	unsigned int i, j;
	double c, s, xy_dist;
	double x_axis[3], y_axis[3], z_axis[3], vec[3];
	double left, right, bottom, top, farVal, nearVal, margin;
	double dx, dy, d_aspect, w_aspect, d;

	// 視点の設定
	eye[Z] = sin(theta * PI / 180);
	xy_dist = cos(theta * PI / 180);
	c = cos(phi * PI / 180);
	s = sin(phi * PI / 180);
	eye[X] = xy_dist * c;
	eye[Y] = xy_dist * s;
	up[X] = -c * eye[Z];
	up[Y] = -s * eye[Z];
	up[Z] = s * eye[Y] + c * eye[X];
	normalVec(up);

	// 視点を原点とする座標系の定義
	for (i = 0; i < 3; i++)
		z_axis[i] = eye[i] - center[i];
	normalVec(z_axis);
	cross(up, z_axis, x_axis);
	normalVec(x_axis); //　これいる？
	cross(z_axis, x_axis, y_axis);

	// left, right, bottom, top, nearVal, farValの決定
	left = bottom = farVal = 10000.0;
	right = top = nearVal = -10000.0;
	for (i = 0; i < num_points; i++)
	{
		for (j = 0; j < 3; j++)
			vec[j] = point[i][j] - eye[j];
		if (dot(x_axis, vec) < left)
			left = dot(x_axis, vec);
		if (dot(x_axis, vec) > right)
			right = dot(x_axis, vec);
		if (dot(y_axis, vec) < bottom)
			bottom = dot(y_axis, vec);
		if (dot(y_axis, vec) > top)
			top = dot(y_axis, vec);
		if (dot(z_axis, vec) < farVal)
			farVal = dot(z_axis, vec);
		if (dot(z_axis, vec) > nearVal)
			nearVal = dot(z_axis, vec);
	}

	//　図形の周辺に5％ほど余裕を与える
	margin = (right - left) * 0.05;
	left -= margin;
	right += margin;
	margin = (top - bottom) * 0.05;
	bottom -= margin;
	top += margin;
	margin = (nearVal - farVal) * 0.05;
	farVal -= margin;
	nearVal += margin;

	// 表示範囲のアスペクト比とウィンドウのアスペクト比の比較
	dx = right - left;
	dy = top - bottom;
	d_aspect = dy / dx;
	w_aspect = (double)window_height / (double)window_width;

	//　ウィンドウが表示範囲よりも縦長，表示範囲を広げる
	if (w_aspect > d_aspect)
	{
		d = (dy * (w_aspect / d_aspect - 1.0)) * 0.5;
		bottom -= d;
		top += d;

		// ウィンドウが表示範囲よりも横長，表示班にを横に広げる．
	}
	else
	{
		d = (dx * (d_aspect / w_aspect - 1.0)) * 0.5;
		left -= d;
		right += d;
	}
	// OpenGL内部の行列データ記録用のメモリを投影のために用いる宣言
	glMatrixMode(GL_PROJECTION);
	// 処理前にメモリを単位行列に初期化
	glLoadIdentity();
	// ビューポートのサイズ指定
	glOrtho(left, right, bottom, top, -nearVal, -farVal);
}

void resize(int width, int height)
{
	unsigned int i;
	double dx, dy, d_aspect, w_aspect, d;
	double margin;

	// print("Size %d x %d\n", width, height);
	window_width = width;
	window_height = height;

	// 図形中心の表示
	{
		// 座標の範囲の読み取り
		init_left = init_bottom = 10000.0;
		init_right = init_top = -10000.0;
		for (i = 0; i < num_points; i++)
		{
			if (point[i][X] < init_left)
				init_left = point[i][X];
			if (point[i][X] > init_right)
				init_right = point[i][X];
			if (point[i][Y] < init_bottom)
				init_bottom = point[i][Y];
			if (point[i][Y] > init_top)
				init_top = point[i][Y];
		}

		// 周囲に5%だけ広がる
		margin = (init_right - init_left) * 0.05;
		init_left -= margin;
		init_right += margin;
		margin = (init_top - init_bottom) * 0.05;
		init_bottom -= margin;
		init_top += margin;
	}
	// 表示範囲のアスペクト非とウィンドウのアスペクト比の比較
	dx = init_right - init_left;
	dy = init_top - init_bottom;
	d_aspect = dy / dx;
	w_aspect = (double)window_height / (double)window_width;

	// ウィンドウが表示範囲よりも縦長，表示範囲を縦に広げる
	if (w_aspect > d_aspect)
	{
		d = (dy * (w_aspect / d_aspect - 1.0)) * 0.5;
		left = init_left;
		right = init_right;
		bottom = init_bottom - d;
		top = init_top + d;
	}
	else
	{
		d = (dx * (d_aspect / w_aspect - 1.0)) * 0.5;
		left = init_left - d;
		right = init_right + d;
		bottom = init_bottom;
		top = init_top;
	}
	////////////////////////////////////////
}

// ウィンドウへの描画関数
void display(void)
{
	// 正投影の定義
	defineViewMatrix(phi, theta);

	// 視点移動
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity(); //　この単位行列化は必要なのか？
		// 視点の移動
		gluLookAt(eye[X], eye[Y], eye[Z], center[X], center[Y], center[Z], up[X], up[Y], up[Z]);
	}

	// 投影によって得られた２次元の画像情報をウィンドウにはめ込む処理（左下基準）
	glViewport(0, 0, window_width, window_height);
	// ウィンドウを背景色で染める
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_LINES);

	glVertex3dv(point[0]);
	glVertex3dv(point[1]);

	glVertex3dv(point[1]);
	glVertex3dv(point[2]);

	glVertex3dv(point[2]);
	glVertex3dv(point[3]);

	glVertex3dv(point[3]);
	glVertex3dv(point[0]);

	glVertex3dv(point[4]);
	glVertex3dv(point[5]);

	glVertex3dv(point[5]);
	glVertex3dv(point[6]);

	glVertex3dv(point[6]);
	glVertex3dv(point[7]);

	glVertex3dv(point[7]);
	glVertex3dv(point[4]);

	glVertex3dv(point[0]);
	glVertex3dv(point[4]);

	glVertex3dv(point[1]);
	glVertex3dv(point[5]);

	glVertex3dv(point[2]);
	glVertex3dv(point[6]);

	glVertex3dv(point[3]);
	glVertex3dv(point[7]);

	// glClear関数を確実に実行させるための「おまじない」
	glEnd();
	glFlush();
}

void initGL(void)
{
	// 画面の背景色を定義
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

int main(int argc, char *argv[])
{
	// 左上基準
	glutInitWindowPosition(128, 128);
	glutInitWindowSize(768, 768);
	// OpenGL/GLUT 環境の初期化
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA);
	// ウィンドウを開く
	glutCreateWindow(argv[0]);
	// ウィンドウへの描画関数の登録(displayにはコールバック関数へのポインタが与えられる)
	glutDisplayFunc(display);
	// ウィンドウの生成時やサイズの変更のイベントのたびにresize関数が呼ばれる
	glutReshapeFunc(resize);

	glutMouseFunc(mouse_button);
	glutMotionFunc(mouse_motion);

	// 自分で定義した関数の呼び出し
	initGL();
	// メインループ開始
	glutMainLoop();

	return 0;
}