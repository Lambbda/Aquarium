#include "stdafx.h"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <ctime>		//Для рандома
#include <math.h>		//Для геометрических формул
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

#define PI 3.14159265	//Для перевода, т.к. math.h предпочитает радианы

const int tick = 30;	//Скорость анимации

HBITMAP bmScreenBuffer;

void Init(HDC hdc){		//Рисует начальное
	Graphics graphics(hdc);
	Pen      pen(Color(255,100,200,200),25);
	graphics.DrawRectangle(&pen, 400,100,600,600);	//Стенки аквариума
}

struct TFish {		//Объект рыбы
public:

		HDC hdc;
		Color TColor;	//ARGB
		int Coord[2];	//Координаты носа (Если у рыб есть нос?). 0 - х, 1 - у
		int Speed;		//Скорость в пикселях, >1. При <=1 двигается назад(?)
		int Size;		//Размер в пикселях
		int Dir;		//Направление в градусах

		void Draw(HDC hdc)
		{
			Graphics graphics(hdc);
			Pen      pen(TColor, Size / 5);

			graphics.DrawLine(&pen, Coord[0], Coord[1], Coord[0] + Size * cos((Dir + 30)*PI / 180), Coord[1] + Size * sin((Dir + 30)*PI / 180));		//Туша из двух линий (уголка) и глаза по формуле точки
			graphics.DrawLine(&pen, Coord[0], Coord[1], Coord[0] + Size * cos((Dir - 30)*PI / 180), Coord[1] + Size * sin((Dir - 30)*PI / 180));		//на окружности x=rcosf, y=rsinf; угол между линиями 60. 
			graphics.DrawEllipse(&pen, Coord[0] + Size / 2 * cos((Dir)*PI / 180), Coord[1] + Size / 2 * sin((Dir)*PI / 180), Size / 8, Size / 8);		//Глаз выглядит так, будто рыба готова тебе втащить. Фича.
		}

		void Look(HDC hdc)
		{
			for (int i = Size; i < Speed*3 + Size; i++)
			{
				COLORREF CRF = GetPixel(hdc, Coord[0] + i*cos(-Dir*PI / 180), Coord[1] + i*sin(-Dir*PI / 180));
				if (CRF != (0x00ffffff)) {
					Dir += 180;
					break;
				}
			}
		}

		void Run(HDC hdc)
		{
			Graphics graphics(hdc);

			SolidBrush	 eraser(Color(255, 255, 255, 255));					//Белый ластик для стирания устаревших кадров
			PointF point1(Coord[0] - Size * 0.5 * cos((Dir)*PI / 180), Coord[1] - Size * 0.5 * sin((Dir)*PI / 180));			//Получаем полигон (треугольник) с рыбой. Координаты отличаются от Draw,
			PointF point2(Coord[0] + Size * 1.3 * cos((Dir + 40)*PI / 180), Coord[1] + Size * 1.3 * sin((Dir + 40)*PI / 180));	//т.к. линии утолщены. Мне было лень искать точную формулу, поэтому
			PointF point3(Coord[0] + Size * 1.3 * cos((Dir - 40)*PI / 180), Coord[1] + Size * 1.3 * sin((Dir - 40)*PI / 180));	//размеры полигона подогнаны на глаз (но вполне точно). 
			PointF points[3] = { point1, point2, point3 };	
			graphics.FillPolygon(&eraser, points, 3);						//Закрашиваем полученный полигон, т.к. кадр устарел.

			Coord[0] -= Speed*cos(Dir*PI / 180);		//Двигаем Карпа по направлению Dir со скоростью Speed.
			Coord[1] -= Speed*sin(Dir*PI / 180);
			if (rand() % 4 < 3) {
				Dir += rand() % 31 - 15;								//Двойной рандом: с вероятностью 50% поворачивает от 0 до 15 влево или вправо 
			}

			Look(hdc);
			Draw(hdc);												//Рисуем новый кадр
		}
};
			//С этого момента тупая копипаста образца проги от микрософта

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, INT iCmdShow)
{
	HWND                hWnd;
	MSG                 msg;
	WNDCLASS            wndClass;
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;

	// Initialize GDI+.
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = TEXT("Aquarium");

	RegisterClass(&wndClass);

	hWnd = CreateWindow(
		TEXT("Aquarium"),   // window class name
		TEXT("Аквариум"),  // window caption
		WS_OVERLAPPEDWINDOW,      // window style
		CW_USEDEFAULT,            // initial x position
		CW_USEDEFAULT,            // initial y position
		CW_USEDEFAULT,            // initial x size
		CW_USEDEFAULT,            // initial y size
		NULL,                     // parent window handle
		NULL,                     // window menu handle
		hInstance,                // program instance handle
		NULL);                    // creation parameters

	ShowWindow(hWnd, iCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	GdiplusShutdown(gdiplusToken);
	return msg.wParam;
}  // WinMain

LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
	WPARAM wParam, LPARAM lParam)
{ 
	HDC          hdc;
	HDC          hdcBuffer;
	PAINTSTRUCT  ps;	//Конец копипасты
	TFish Carp;			//Стартовые рыбы
	TFish BigBoy;		//Его настоящее имя - Дра'акуек, крушитель аквариумов
	TFish MONSTER;
	srand(time(NULL));	//Делаем рандом рандомным
	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		hdcBuffer = CreateCompatibleDC(hdc);

		//HBITMAP hbm_Buffer = CreateCompatibleBitmap(hdc, bmScreenBuffer->GetWidth(), bmScreenBuffer->GetHeight());
		//HBITMAP hbm_oldBuffer = (HBITMAP)SelectObject(hdcBuffer, hbm_Buffer);

		Init(hdc);

		Carp.TColor = { 255,200,100,1 };
		Carp.Coord[0] = 500;
		Carp.Coord[1] = 250;
		Carp.Size = 20; 
		Carp.Speed = 5;
		Carp.Dir = 210;

		BigBoy.TColor = { 255,50,60,255 };
		BigBoy.Coord[0] = 800;
		BigBoy.Coord[1] = 150;
		BigBoy.Size = 30;
		BigBoy.Speed = 3;
		BigBoy.Dir = 45;

		MONSTER.TColor = { 255,50,255,2 };
		MONSTER.Coord[0] = 800;
		MONSTER.Coord[1] = 350;
		MONSTER.Size = 300;
		MONSTER.Speed = 3;
		MONSTER.Dir = 90;

		for (int i=1;i<300;i++) {	//Цикл анимации
			Carp.Run(hdc);
			BigBoy.Run(hdc);
			//MONSTER.Run(hdc);
			Sleep(1000 / tick);	
		}

		EndPaint(hWnd, &ps);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
} // WndProc