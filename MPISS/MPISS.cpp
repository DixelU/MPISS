#define NOMINMAX 1
#include <Windows.h>
#include <algorithm>

#include <optional>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>

#include <thread>
#include <locale>

#include "SAFGUIF/SAFGUIF.h"
#include "dotted_plotter.h"

#include "MPISS_main.h"

void Init() {
	auto DP = new DottedPlotter(0, 0, 300, 300, 5, 5, System_White, 0x00FFFFDF, 0x7F7F7F1F, 0x7F7F7F7F, true);

	std::vector<matrix> *mxs = new std::vector<matrix>{ matrix({std::vector<double>{1.1, 1.4}}), matrix({std::vector<double>{1.2, 1.9}}), matrix({std::vector<double>{1.4, 7.7}}) };
	std::vector<double>* vals = new std::vector<double>{ 0.5, 0.6, 0.4 };;

	DP->amd->get_param_callback = [mxs](int i) -> matrix& {return (*mxs)[i]; };
	DP->amd->get_value_callback = [vals](int i) -> double {return (*vals)[i]; };
	DP->amd->size_callback = [mxs, vals]() -> int {return std::min((*mxs).size(), (*vals).size()); };
	DP->amd->is_alive = true;

	MoveableWindow* T = new MoveableResizeableWindow("Main window", System_White, -200, 200, 400, 400, 0, 0x3F3F3F7F, 0, [DP](float dH, float dW, float NewHeight, float NewWidth) {
		DP->SafeMove(0.5 * dW, -0.5 * dH);
		DP->SafeResize(NewHeight - 100, NewWidth - 100);
		});
	((MoveableResizeableWindow*)T)->AssignMinDimentions(200, 200);

	(*T)["PLOTTER"] = DP;	

	(*WH)["MAIN"] = T;

	//__main();
}

///////////////////////////////////////
/////////////END OF USE////////////////
///////////////////////////////////////

#define _WH(Window,Element) ((*(*WH)[Window])[Element])//...uh
#define _WH_t(Window,Element,Type) ((Type)_WH(Window,Element))

void onTimer(int v);
void mDisplay() {
	glClear(GL_COLOR_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	if (FIRSTBOOT) {
		FIRSTBOOT = 0;

		WH = new WindowsHandler();
		Init();
		
		ANIMATION_IS_ACTIVE = !ANIMATION_IS_ACTIVE;
		onTimer(0);
	}

	if (WH)WH->Draw();

	glutSwapBuffers();
}

void mInit() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D((0 - RANGE) * (WindX / WINDXSIZE), RANGE * (WindX / WINDXSIZE), (0 - RANGE) * (WindY / WINDYSIZE), RANGE * (WindY / WINDYSIZE));
}

void onTimer(int v) {
	glutTimerFunc(33, onTimer, 0);
	if (ANIMATION_IS_ACTIVE) {
		mDisplay();
		++TimerV;
	}
}

void OnResize(int x, int y) {
	WindX = x;
	WindY = y;
	mInit();
	glViewport(0, 0, x, y);
}
void inline rotate(float& x, float& y) {
	float t = x * cos(ROT_RAD) + y * sin(ROT_RAD);
	y = 0.f - x * sin(ROT_RAD) + y * cos(ROT_RAD);
	x = t;
}
void inline absoluteToActualCoords(int ix, int iy, float& x, float& y) {
	float wx = WindX, wy = WindY;
	x = ((float)(ix - wx * 0.5f)) / (0.5f * (wx / (RANGE * (WindX / WINDXSIZE))));
	y = ((float)(0 - iy + wy * 0.5f)) / (0.5f * (wy / (RANGE * (WindY / WINDYSIZE))));
	rotate(x, y);
}
void mMotion(int ix, int iy) {
	float fx, fy;
	absoluteToActualCoords(ix, iy, fx, fy);
	MXPOS = fx;
	MYPOS = fy;
	if (WH)WH->MouseHandler(fx, fy, 0, 0);
}
void mKey(BYTE k, int x, int y) {
	if (WH)WH->KeyboardHandler(k);

	if (k == 27)
		exit(1);
}
void mClick(int butt, int state, int x, int y) {
	float fx, fy;
	CHAR Button, State = state;
	absoluteToActualCoords(x, y, fx, fy);
	Button = butt - 1;
	if (state == GLUT_DOWN)State = -1;
	else if (state == GLUT_UP)State = 1;
	if (WH)WH->MouseHandler(fx, fy, Button, State);
}
void mDrag(int x, int y) {
	mMotion(x, y);
}
void mSpecialKey(int Key, int x, int y) {
	auto modif = glutGetModifiers();
	if (!(modif & GLUT_ACTIVE_ALT)) {
		switch (Key) {
		case GLUT_KEY_DOWN:		if (WH)WH->KeyboardHandler(1);
			break;
		case GLUT_KEY_UP:		if (WH)WH->KeyboardHandler(2);
			break;
		case GLUT_KEY_LEFT:		if (WH)WH->KeyboardHandler(3);
			break;
		case GLUT_KEY_RIGHT:	if (WH)WH->KeyboardHandler(4);
			break;
		}
	}
	if (modif == GLUT_ACTIVE_ALT && Key == GLUT_KEY_DOWN) {
		RANGE *= 1.1;
		OnResize(WindX, WindY);
	}
	else if (modif == GLUT_ACTIVE_ALT && Key == GLUT_KEY_UP) {
		RANGE /= 1.1;
		OnResize(WindX, WindY);
	}
}
void mExit(int a) {

}

int main(int argc, char** argv) {
	std::ios_base::sync_with_stdio(false);//why not
#ifdef _DEBUG 
	ShowWindow(GetConsoleWindow(), SW_SHOW);
#else // _DEBUG 
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
	ShowWindow(GetConsoleWindow(), SW_SHOW);

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	//srand(1);
	//srand(clock());
	InitASCIIMap();
	//cout << to_string((WORD)0) << endl;

	srand(TIMESEED());
	__glutInitWithExit(&argc, argv, mExit);
	//cout << argv[0] << endl;
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA | GLUT_MULTISAMPLE);
	glutInitWindowSize(WINDXSIZE, WINDYSIZE);
	//glutInitWindowPosition(50, 0);
	glutCreateWindow(WINDOWTITLE);

	hWnd = FindWindowA(NULL, WINDOWTITLE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//_MINUS_SRC_ALPHA
	glEnable(GL_BLEND);

	//glEnable(GL_POLYGON_SMOOTH);//laggy af
	glEnable(GL_LINE_SMOOTH);//GL_POLYGON_SMOOTH
	glEnable(GL_POINT_SMOOTH);

	glShadeModel(GL_SMOOTH);
	//glEnable(GLUT_MULTISAMPLE);

	glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);//GL_FASTEST//GL_NICEST
	glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);

	glutMouseFunc(mClick);
	glutReshapeFunc(OnResize);
	glutSpecialFunc(mSpecialKey);
	glutMotionFunc(mDrag);
	glutPassiveMotionFunc(mMotion);
	glutKeyboardFunc(mKey);
	glutDisplayFunc(mDisplay);
	mInit();
	glutMainLoop();
	return 0;
}