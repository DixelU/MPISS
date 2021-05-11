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

//#define IS_SAMPLE_CONSTRUCTION_BUILD

#include "SAFGUIF/SAFGUIF.h"
#include "dotted_plotter.h"
#include "matrix_explorer.h"

#include "MPISS_main.h"

namespace data {
	std::string ITERS = "1000", REPS = "10", INITIALS = "1", BES = std::to_string(params_manipulator_globals::begin_evolution_sizes);
	static pooled_thread th;
}

void Start() {
	static auto func = [](void** ptr) {
		auto MainWindow = (MoveableResizeableWindow*)((*WH)["MAIN"]);
		auto SettingsWindow = (MoveableResizeableWindow*)((*WH)["SETTINGS"]);
		auto DP = (DottedPlotter*)(*MainWindow)["PLOTTER"];
		auto ITERS = (InputField*)(*SettingsWindow)["ITERS"];
		auto REPS = (InputField*)(*SettingsWindow)["REPS"];
		auto INITIALS = (InputField*)(*SettingsWindow)["INITIALS"];
		auto METHOD = (SelectablePropertedList*)(*SettingsWindow)["METHOD"];

		params_manipulator_globals::begin_evolution_sizes = std::stoi(data::BES);
		
		auto method = magic_enum::enum_cast<minimization_method>(METHOD->SelectorsText[METHOD->CurrentTopLineID]);
		if (method) {
			StartSimulationProcess(DP->amd, method.value(),
				std::stoi(REPS->GetCurrentInput(data::REPS)),
				std::stoi(INITIALS->GetCurrentInput(data::INITIALS)),
				std::stoi(ITERS->GetCurrentInput(data::ITERS))
			);
			WH->ThrowAlert("Simulation has ended.\n", "Stop signal", SpecialSigns::DrawExTriangle, false, 0x0F0F0FFF, 0xFFFFFFFF);
		}
		else
			WH->ThrowAlert("Error while parsing selected minimization method", "Stop signal", SpecialSigns::DrawExTriangle, false, 0x0F0F0FFF, 0xFFFFFFFF);
	};
	std::this_thread::sleep_for(std::chrono::milliseconds(33));
	if (data::th.get_state() == pooled_thread::state::idle) {
		data::th.set_new_function(func);
		data::th.sign_awaiting();
	}
	else
		WH->ThrowAlert("Thread is running!", "Escape signal", SpecialSigns::DrawExTriangle, false, 0x0F0F0FFF, 0xFFFFFFFF);
}

void StartFastExample() {
	static auto func = [](void** ptr) {
		auto MainWindow = (MoveableResizeableWindow*)((*WH)["MAIN"]);
		auto SettingsWindow = (MoveableResizeableWindow*)((*WH)["SETTINGS"]);
		auto DP = (DottedPlotter*)(*MainWindow)["PLOTTER"];
		auto METHOD = (SelectablePropertedList*)(*SettingsWindow)["METHOD"];
		auto method = magic_enum::enum_cast<minimization_method>(METHOD->SelectorsText[METHOD->CurrentTopLineID]);

		params_manipulator_globals::begin_evolution_sizes = std::stoi(data::BES);

		if (method) {
			SimpleExample(DP->amd, method.value());
			WH->ThrowAlert("Simulation has ended.\n", "Stop signal", SpecialSigns::DrawExTriangle, false, 0x0F0F0FFF, 0xFFFFFFFF);
		}
		else 
			WH->ThrowAlert("Error while parsing selected minimization method", "Stop signal", SpecialSigns::DrawExTriangle, false, 0x0F0F0FFF, 0xFFFFFFFF);
	};

	if (data::th.get_state() == pooled_thread::state::idle) {
		data::th.set_new_function(func);
		data::th.sign_awaiting();
	}
	else
		WH->ThrowAlert("Thread is running!", "Escape signal", SpecialSigns::DrawExTriangle, false, 0x0F0F0FFF, 0xFFFFFFFF);
}

void Init() {
	static ButtonSettings* BS_List_Black_Small = new ButtonSettings(System_White, 0, 0, 100, 5, 1, 0, 0, 0xFFEFDFFF, 0x00003F7F, 0x7F7F7FFF);

	auto DP = new DottedPlotter(0, 0, 275, 275, 5, 5, System_White, 0x00FF7FFF, 0x7F7F7F1F, 0x7F7F7F7F, false);

	DP->max_value = 10000;

	MoveableWindow* T = new MoveableResizeableWindow("Main window", System_White, -200, 200, 400, 400, 0xFF, 0x3F3F3F7F, 0, [DP](float dH, float dW, float NewHeight, float NewWidth) {
		DP->SafeMove(0.5 * dW, -0.5 * dH);
		DP->SafeResize(NewHeight - 125, NewWidth - 125);
	});
	((MoveableResizeableWindow*)T)->AssignMinDimentions(200, 200);

	(*T)["PLOTTER"] = DP;

	(*WH)["MAIN"] = T;


	T = new MoveableWindow("Settings", System_White, -350, 200, 150, 400, 0xFF, 0x3F3F3F7F);

	auto L = new SelectablePropertedList(BS_List_Black_Small, nullptr, nullptr, -275, -175, 140, 10, 50, 1, _Align::center);

	auto minimization_methods = magic_enum::enum_names<minimization_method>();
	for (auto& val : minimization_methods)
		L->SafePushBackNewString(std::string(val));

	L->DefaultScrollStep = 1;

	(*T)["METHOD"] = L;

	(*T)["START"] = new Button("Start", System_White, Start, -325, +190 - WindowHeapSize, 40, 10, 1, 0, 0xFFFFFFFF, 0xFF, 0xFFFFFFFF, 0x7F7F7FFF, nullptr);
	(*T)["START_FAST"] = new Button("Start fast example", System_White, StartFastExample, -300, +150 - WindowHeapSize, 90, 10, 1, 0, 0xFFFFFFFF, 0xFF, 0xFFFFFFFF, 0x7F7F7FFF, nullptr);

	(*T)["ITERS"] = new InputField(data::ITERS, -275, 190 - WindowHeapSize, 10, 40, System_White, &data::ITERS, 0x007FFFFF, System_White, "Iterations", 7, _Align::center, _Align::center, InputField::Type::NaturalNumbers);

	(*T)["REPS"] = new InputField(data::REPS, -325, 170 - WindowHeapSize, 10, 40, System_White, &data::REPS, 0x007FFFFF, System_White, "Repeats", 7, _Align::center, _Align::center, InputField::Type::NaturalNumbers);
	(*T)["INITIALS"] = new InputField(data::INITIALS, -225, 170 - WindowHeapSize, 10, 40, System_White, &data::INITIALS, 0x007FFFFF, System_White, "Inititals", 7, _Align::center, _Align::center, InputField::Type::NaturalNumbers);


	(*T)["BES"] = new InputField(std::to_string(params_manipulator_globals::begin_evolution_sizes), -275, 170 - WindowHeapSize, 10, 40, System_White, &data::BES, 0x007FFFFF, System_White, "Samples count", 7, _Align::center, _Align::center, InputField::Type::NaturalNumbers);
	(*T)["2"] = new InputField("0", -225, 190 - WindowHeapSize, 10, 40, System_White, NULL, 0x007FFFFF, System_White, "___", 7, _Align::center, _Align::center, InputField::Type::NaturalNumbers);


	(*T)["MATRIX_EXPLORER"] = new MatrixExplorer(-345, 140 - WindowHeapSize, 5, 1, 0x00007F7F, 0xFFFFFFFF, 0x1F1F1FFF,
		[DP]()->std::pair<size_t, size_t> {
			if (DP->amd->is_alive) {
				std::lock_guard<std::mutex> locker(DP->amd->locker);
				if (DP->amd->size_callback()) {
					matrix& mx = DP->amd->get_param_callback(0);
					auto pair = mx.size();
					return {pair.second, pair.first};
				}
				else return { 0,0 };
			}
			return { 0,0 };
		}, [DP](size_t x, size_t y) {
			DP->mx_x = x;
			DP->mx_y = y;
		}
		);

	T->IsCloseable = false;

	(*WH)["SETTINGS"] = T;


	T = new MoveableWindow("Matrix printer", System_White, 200, 200, 400, 400, 0xFF, 0xFF);

	auto matrix_output = new TextBox(" ", System_White, 0, 0, 390 - WindowHeapSize, 390, 10, 0, 0, 0, _Align::left, TextBox::VerticalOverflow::display);
	matrix_output->SafeChangePosition_Argumented(GLOBAL_TOP | GLOBAL_LEFT, 205, 195 - WindowHeapSize);
	(*T)["OUTPUT"] = matrix_output;

	DP->on_click =[matrix_output](const matrix& mx) {
		std::stringstream ss;
		ss << mx;
		matrix_output->SafeStringReplace(ss.str());
	};

	T->IsCloseable = false;

	(*WH)["PRINT_DEST"] = T;


	WH->EnableWindow("MAIN");
	WH->EnableWindow("SETTINGS");
	WH->EnableWindow("PRINT_DEST");

	params_manipulator_globals::desired_range = 1e-8;

	//__main();
}

///////////////////////////////////////
/////////////END OF USE////////////////
///////////////////////////////////////

#define _WH(MainWindow,Element) ((*(*WH)[MainWindow])[Element])//...uh
#define _WH_t(MainWindow,Element,Type) ((Type)_WH(MainWindow,Element))

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