#include "stdafx.h"
#include "Aquarium.h"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <ctime>        //��� �������
#include <math.h>       //��� �������������� ������
#include <list>
#include <iterator>
using namespace Gdiplus;
using namespace std;
#pragma comment (lib,"Gdiplus.lib")

#define PI 3.14159265   //��� ��������, �.�. math.h ������������ �������
						//������������ ID ��� ����
#define ID_Add 1		//�������� ����
#define ID_Remove 2		//������ ����
#define ID_Pause 3		//���������� ��������
#define ID_Exit 4		//������� ����
#define ID_Restart 5	//����������� ���������
#define ID_Debug 6		//���������� �� ���

HANDLE hTickThread;		//��������� �� ������������ ����
HWND hWnd;				//�������� ���� ����������
HWND nWnd;				//������ ���� ��� ����� ������
HDC aquarium;			//�� � ������ ����������
bool pause = 0;			//����� ��������
bool test = 0;			//���� ��
const int fps = 30;		//�������� ��������
Color KarpC = { 255,240,100,50 };
Color PikeC = { 255,50,200,100 };

int distance(int x1, int  y1, int  x2, int  y2) {			//������ ��� ��������: ������� ���������� ����� ����� �������.
	int d;
	d = sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2));		//d = sqrt((x2-x1)^2+(y2-y1)^2)	
	return d;
}

struct TFish {			//����������� ����

private:				//�������: ��������� ���������� - �������� � Init � �� ��������; HDC - ������� ����� �����; ������ Draw, Look � AITest - ������������ ���������� � ������ Draw

	HDC hdc;			//�������� ����������

	virtual void Look(HDC hdc)				//������ ���� - ��� ���� �� ����. ����� 100 ��������, ��������� - ��� ������� ����. ���������� ���� � ��������� (� �������� i) �� �������.
	{
		for (int i = 1; i < Size; i++)	//��������� - ��������� ��������, ����� � ��� ��� ���, ����������� ��� ����������� ����� �� ����.
		{
			COLORREF CRF_Left = GetPixel(hdc, Coord[0] - i * cos((Dir - 50) * PI / 180), Coord[1] - i * sin((Dir - 50) * PI / 180));	//����� ��� ������
			if (CRF_Left != 0x00ffffeb && CRF_Left != TColor.ToCOLORREF()) {															//��� ������ ���������� ���� ���� (�����) � ����������� ����.
				Dir += 45;																												//����� ������������.
				break;																													//��� ����������� ����� �������� �� ������� ����������
			}
			COLORREF CRF_Mid = GetPixel(hdc, Coord[0] - i * cos(Dir * PI / 180), Coord[1] - i * sin(Dir * PI / 180));					//����������� ���
			if (CRF_Mid != 0x00ffffeb && CRF_Mid != TColor.ToCOLORREF()) {
				Dir += 180;
				break;
			}
			COLORREF CRF_Right = GetPixel(hdc, Coord[0] - i * cos((Dir + 50) * PI / 180), Coord[1] - i * sin((Dir + 50)* PI / 180));	//������ ��� (�� �����, �� ��������))))
			if (CRF_Right != 0x00ffffeb && CRF_Right != TColor.ToCOLORREF()) {
				Dir -= 45;
				break;
			}
		}
	}

	void AITest(HDC hdc, Color color) {				//����� - ������ ������� ������ ���� (TFish::Look). 

		Graphics graphics(hdc);
		Pen red(color, 1);
		int x = (Coord[0] - cos((Dir)* PI / 180));	//DrawLine �� �������� ������, �������� ��� � int ��� float, ��-�� ���� ������������� ����������. ������� �������� ������ ���� �� ���������� int.
		graphics.DrawLine(&red,						//������� ������ ����� �� Look
			x,
			Coord[1] - sin((Dir)* PI / 180),
			Coord[0] - Size * cos((Dir)* PI / 180),
			Coord[1] - Size * sin((Dir)* PI / 180));
		graphics.DrawLine(&red,
			x,
			Coord[1] - sin((Dir)* PI / 180),
			Coord[0] - Size * cos((Dir - 50)* PI / 180),
			Coord[1] - Size * sin((Dir - 50)* PI / 180));
		graphics.DrawLine(&red,
			x,
			Coord[1] - sin((Dir)* PI / 180),
			Coord[0] - Size * cos((Dir + 50)* PI / 180),
			Coord[1] - Size * sin((Dir + 50)* PI / 180));
	}

public:	

	bool Test = 0;			//������������� ������ ������������
	int Coord[2];			//���������� ���� (���� � ��� ���� ���?). 0 - �, 1 - �
	int Dir;				//����������� � ��������
	int Size;				//������ � ��������
	int Speed;				//�������� � ��������, >1
	Color TColor;			//ARGB

	void Init(				//������������� �������� ����. 
		Color iColor,		//�� �� ����� ��������� ���� ���� � ���� ����, ������ ��� �� ����������� �������� (�� �����)
		int iSize,
		bool iTest			//�� ��� �� ������� � Test �� ����������� ��������
	) {
		TColor = iColor;
		Size = iSize;
		Test = iTest;
		Speed = 50 / Size+1;//�������� �������� ������������ ��������
	}

	virtual void Draw(HDC hdc, Color color)		//������ ����. ����� ����� ����� ����� ��� �������� (� ������, ������������) ����������� �����.
	{
		Graphics graphics(hdc);
		Pen   pen(color, Size / 5);
		SolidBrush fill(color);

		graphics.DrawLine(&pen, Coord[0], Coord[1], Coord[0] + Size * cos((Dir + 30)*PI / 180), Coord[1] + Size * sin((Dir + 30)*PI / 180));		//���� �� ���� ����� (������) � ����� �� ������� �����
		graphics.DrawLine(&pen, Coord[0], Coord[1], Coord[0] + Size * cos((Dir - 30)*PI / 180), Coord[1] + Size * sin((Dir - 30)*PI / 180));        //�� ���������� x=rcosf, y=rsinf; ���� ����� ������� 60.
		graphics.DrawEllipse(&pen, Coord[0] + Size / 2 * cos((Dir)*PI / 180), Coord[1] + Size / 2 * sin((Dir)*PI / 180), Size / 8, Size / 8);       //���� �������� ���, ����� ���� ������ ���� �������. ����.
	}

	void Run(HDC hdc)							//������� ���� (+ �� � ����.)
	{
		Graphics graphics(hdc);
		Color Water(255, 235, 255, 255);

		Draw(hdc, Water);											//����������� ���� ���������� ����� ������� ������
		if (Test == 1) AITest(hdc, Water);							//� �������� ������ ����� ����������� �� ��������

		Coord[0] -= Speed * cos(Dir*PI / 180);                      //������� ���� �� ����������� Dir �� ��������� Speed.
		Coord[1] -= Speed * sin(Dir*PI / 180);
		if (rand() % 10 < 6) {
			Dir += rand() % 31 - 15;                                //������� ������: � ������������ 60% ������������ �� 0 �� 15 ����� ��� ������
		}

		Look(hdc);
		Draw(hdc, TColor);                                          //������ ����� ����
		if (Test == 1) AITest(hdc, TColor);
	}
};

struct TKarp : public TFish {										//���� ��� ��� ����.

	bool operator ==(const TKarp& K)								//��� �������� ���������� ������ (struct==struct), ��� C++ �� ��������� �� �����. ������� ������ ���� ��������. 
	{
		return Coord[0] == K.Coord[0] && Coord[1] == K.Coord[1];	//"�������" ��������� ����� � ����������� ������������. �� ��������� �������, �� ���� ��� ����� ����� �������� � ����� ����� ����� �����, ��� ��� �����.
	}

	void Draw(HDC hdc, Color color) {								//� ������ ���� ����� ���������, ������� ���� ������� Draw

		Graphics graphics(hdc);
		SolidBrush fill(color);

		Point K1(Coord[0], Coord[1]);
		Point K2(Coord[0] + Size * cos((Dir - 30)*PI / 180), Coord[1] + Size * sin((Dir - 30)*PI / 180));
		Point K3(Coord[0] + Size * .6 * cos((Dir)*PI / 180), Coord[1] + Size * .6 * sin((Dir)*PI / 180));
		Point K4(Coord[0] + Size * cos((Dir + 30)*PI / 180), Coord[1] + Size * sin((Dir + 30)*PI / 180));

		Point KPoints[] = { K1, K2, K3, K4 };

		graphics.FillPolygon(&fill, KPoints, (sizeof(KPoints) / sizeof(*KPoints)));
	}
};

struct TPike : public TFish {								//���� ��� ��� ����.

	bool Kill;												//����������, ��� ���� ����-�� ����� � ������ �����.

/*
��� Kill:
�� ��, ���� ������ ���� ������. ��� ����� (����� �� ��) ����� ��������� ������ ������ � ������� ���������� � ����.
������ ������ ���� ������ ��������� (��). �������������, ��� ������ �� ������� ����� ��������� �� ��������.
����� ��������: �� �� ����� ������� ��������� �� �������� � ������� ���� - ���� ���� ���� ����
�������������� ��������� (��), � ������ �� ����� ������� ������������ �������� ��� ����������� �� �������.
����� ������ ��������, �� ��� ���� ������ "���� ����", � � �������� ������ ���� ���� ���� ������ � ������ ����.
*/

	bool operator ==(const TPike& K)								//��� �������� ���������� ��� (struct==struct), ��� C++ �� ��������� �� �����. ������� ������ ���� ��������. 
	{
		return Coord[0] == K.Coord[0] && Coord[1] == K.Coord[1];	//"�������" ��������� ���� � ����������� ������������.
	}

	void Draw(HDC hdc, Color color) { 						//� ��� ���� ����� ���������, ������� ���� ������� Draw

		Graphics graphics(hdc);
		SolidBrush fill(color);

		Point S1(Coord[0], Coord[1]);
		Point S2(Coord[0] + Size * .8 * cos((Dir - 30)*PI / 180), Coord[1] + Size * .8 * sin((Dir - 30)*PI / 180));
		Point S3(Coord[0] + Size * .7 * cos((Dir - 20)*PI / 180), Coord[1] + Size * .7 * sin((Dir - 20)*PI / 180));
		Point S4(Coord[0] + Size * cos((Dir - 20)*PI / 180), Coord[1] + Size * sin((Dir - 20)*PI / 180));
		Point S5(Coord[0] + Size * .8 * cos((Dir)*PI / 180), Coord[1] + Size * .8 * sin((Dir)*PI / 180));
		Point S6(Coord[0] + Size * cos((Dir + 20)*PI / 180), Coord[1] + Size * sin((Dir + 20)*PI / 180));
		Point S7(Coord[0] + Size * .7 * cos((Dir + 20)*PI / 180), Coord[1] + Size * .7 * sin((Dir + 20)*PI / 180));
		Point S8(Coord[0] + Size * .8 * cos((Dir + 30)*PI / 180), Coord[1] + Size * .8 * sin((Dir + 30)*PI / 180));

		Point SPoints[] = { S1, S2, S3, S4, S5, S6, S7, S8 };

		graphics.FillPolygon(&fill, SPoints, (sizeof(SPoints) / sizeof(*SPoints)));
	}

	void Look(HDC hdc) override				//������ ���� - ��� ���� �� ����. ����� 100 ��������, ��������� - ��� ������� ����. ���������� ���� � ��������� (� �������� i) �� �������.
	{
		Kill = 0;

		for (int i = 1; i < Size; i++)	//��������� - ��������� ��������, ����� � ��� ��� ���, ����������� ��� ����������� ����� �� ����.
		{
			COLORREF CRF_Left = GetPixel(hdc, Coord[0] - i * cos((Dir - 50) * PI / 180), Coord[1] - i * sin((Dir - 50) * PI / 180));	//����� ��� ������

			if (CRF_Left == 0x003264f0) {																								//��� ���� ����� ������������ � ����
				Dir -= 25;
			}
			else if (CRF_Left != 0x00ffffeb && CRF_Left != TColor.ToCOLORREF()) {														//���� �� ����, �� ��� � �� ������ ����, ��������������
				Dir += 45;
				break;																													//��� ����������� ����� �������� �� ������� ����������
			}

			COLORREF CRF_Mid = GetPixel(hdc, Coord[0] - i * cos(Dir * PI / 180), Coord[1] - i * sin(Dir * PI / 180));					//����������� ���

			if (CRF_Mid == 0x003264f0) {																								//������������ �������� ��� ���� �����
				Kill = 1;
			}
			else if (CRF_Mid != 0x00ffffeb && CRF_Mid != TColor.ToCOLORREF()) {
				Dir += 180;
				break;
			}

			COLORREF CRF_Right = GetPixel(hdc, Coord[0] - i * cos((Dir + 50) * PI / 180), Coord[1] - i * sin((Dir + 50)* PI / 180));	//������ ��� (�� �����, �� ��������))))

			if (CRF_Right == 0x003264f0) {																								//��� ���� ����� ������������ � ����
				Dir += 25;
			}
			else if (CRF_Right != 0x00ffffeb && CRF_Right != TColor.ToCOLORREF()) {
				Dir -= 45;
				break;
			}
		}
	}
};

struct TAquarium {	//������ ���������

private:

	int x0 = 90;    //���������� ���������: ������� ����� ���� + ���������.
	int y0 = 60;	//������ ������ ������, ������ ��� ����� ���� �������� ����.
	int aw = 600;   //������� ���������: ������, ������.
	int ah = 600;
	HDC hdc;

public:

	bool done = 0;				//�������������: ����� �� ��������
	std::list<TKarp> Karps;		//���� ������
	std::list<TPike> Pikes;		//���� ���

	void Init(HDC Ahdc) {		//������ �������� � ��� ����������
		hdc = Ahdc;

		Graphics graphics(hdc);
		Pen      Glass(Color(255, 100, 200, 200), 20);		//���������� ������� ������, �� ���� �� ����, 20 ��������.
		SolidBrush Water(Color(255, 235, 255, 255));
		SolidBrush Rock(Color(255, 100, 100, 100));

		graphics.FillRectangle(&Water, x0, y0, aw, ah);		//������ ���������
		graphics.DrawRectangle(&Glass, x0, y0, aw, ah);		//������ ���������

		for (int i = 0; i < 10; i++) {						//���������� ��������������� ���������!! ����� ��� � ��.
			int x1 = x0 + rand() % (aw - 70) + 10;			//����� �� ������ ����� � ������, ����� ��� ��� �������. ����� �������� �� �������� ������ ����.
			int y1 = y0 + rand() % (ah - 70) + 10;			//������� ���������: �� �������� ������ ���� + ������, �� ������� ������� - (������ + ������ �����).
			graphics.FillEllipse(&Rock, x1, y1, rand() % 20 + 30, rand() % 20 + 30);
		}

		aquarium = CreateCompatibleDC(hdc);
		HBITMAP initbmp = CreateCompatibleBitmap(hdc, 800, 800);
		SelectObject(aquarium, initbmp);
		BitBlt(aquarium, 0, 0, 800, 800, hdc, 0, 0, SRCCOPY);				//��������� ������ ��������, ����� �������� �� ������� ��� �������.
		done = 1;
	}

	void Run() {	//�������� � ������� �������: �������� (�������) DC; �������� DC, ��� ���������� �� ���������; DC � ����������, ����� �� �������� ������ ����.
		HDC		mdc = CreateCompatibleDC(hdc);					//�������� DC (Memory DC) ��� �������� �� ������
		HBITMAP mbmp = CreateCompatibleBitmap(hdc, 800, 800);	//�������� bmp ��� MDC
		SelectObject(mdc, mbmp);
		BitBlt(mdc, 0, 0, 800, 800, hdc, 0, 0, SRCCOPY);		//������ ����� �������� ����� � ���������� � � MDC ��� ���������

		BitBlt(mdc, 0, 0, 800, 800, aquarium, 0, 0, SRCCOPY);	//��������������� �������� �� ������, ���� ���������� �������� ��� ���������.

		for (auto& i : Karps) {									//������� ���� ������, ��������� � ���������.
			i.Run(mdc);
		}

		for (auto& i : Pikes) {									//������� ���� ���, ��������� � ���������.
			i.Run(mdc);											//���������� ����: ���� ������� ��� ����� �������, �� �� ��������.

			if (i.Kill == 1) {									//���� ���� �������� �����, ��������� ���� ������. ��������� ���� � ����-������ ������ ���� �����.

				TKarp Dead = Karps.front();														//������� ���� ��� ����������, �� ��������� ������ ���� � ������.
				int dist = distance(i.Coord[0], i.Coord[1], Dead.Coord[0], Dead.Coord[1]);		//���������� �� ����. �� ��������� - �� ������� ����� � ������.

				for (auto& k : Karps) {
					if (distance(i.Coord[0], i.Coord[1], k.Coord[0], k.Coord[1]) < dist)
					{
						dist = distance(i.Coord[0], i.Coord[1], k.Coord[0], k.Coord[1]);		//���� ���������� ����������
						Dead = k;
					}
				}
				i.Size += (Dead.Size / 5);						//��� ������� ���� ������ ��������������� �����
				i.Speed = 50 / i.Size + 1;
				Karps.remove(Dead);								//������� �� ���� �����, ������� ��������
			}
		}

		BitBlt(hdc, 0, 0, 800, 800, mdc, 0, 0, SRCCOPY);		//��������� �������� �� MDC � �������� HDC.
		DeleteObject(mbmp);										//������� �����
		DeleteDC(mdc);
	}

	void Add(TKarp Fish) {											//����� ���������� ���� � ��������, � ��������� ��� ������ � ���
		Fish.Coord[0] = x0 + rand() % (aw - 20 - Fish.Size * 2) + 20 + Fish.Size;	//������ � ���� ������. ������� �� ��������� ������ � ���, ���
		Fish.Coord[1] = y0 + rand() % (ah - 20 - Fish.Size * 2) + 20 + Fish.Size;	//���� �������� �� "������", ������� ������ �������� � ���� ������ �� ���� ������.
		Fish.Dir = rand() % 360;
		Karps.push_back(Fish);
	}

	void Add(TPike Fish) {
		Fish.Coord[0] = x0 + rand() % (aw - 20 - Fish.Size * 2) + 20 + Fish.Size;
		Fish.Coord[1] = y0 + rand() % (ah - 20 - Fish.Size * 2) + 20 + Fish.Size;
		Fish.Dir = rand() % 360;
		Pikes.push_back(Fish);
	}

	void Done() {		//����� ����� ��������.
		done = !done;
	}

	void RemoveFish() {
		POINT p;							//������ ���������� �������
		if (GetCursorPos(&p)) {				//���������� ���������� �������
			if (ScreenToClient(hWnd, &p))	//��������� ������ � ��������� �� ����
			{
				if (GetPixel(hdc, p.x, p.y) == KarpC.ToCOLORREF()) {	//��������� ���� ����. ���� ����, ��������� ���� ��������� ������

					TKarp Dead = Karps.front();											//������� ���� ��� ����������, �� ��������� ������ � ������.
					int dist = distance(p.x, p.y, Dead.Coord[0], Dead.Coord[1]);		//���������� �� ����. �� ��������� - �� ������ ���� � ������.

					for (auto& k : Karps) {
						if (distance(p.x, p.y, k.Coord[0], k.Coord[1]) < dist)
						{
							dist = distance(p.x, p.y, k.Coord[0], k.Coord[1]);			//���� ���������� ����������
							Dead = k;
						}
					}
					Dead.Draw(hdc,{ 255,255,0,0 });					//���������� � ������� ���� ����� ���������
					Karps.remove(Dead);								//������� �� ���� ����, ������ �������
				}

				else if (GetPixel(hdc, p.x, p.y) == PikeC.ToCOLORREF()) {	//�� �� ��� ���

					TPike Dead = Pikes.front();											//������� ���� ��� ����������, �� ��������� ������ � ������.
					int dist = distance(p.x, p.y, Dead.Coord[0], Dead.Coord[1]);		//���������� �� ����. �� ��������� - �� ������ ���� � ������.

					for (auto& k : Pikes) {
						if (distance(p.x, p.y, k.Coord[0], k.Coord[1]) < dist)
						{
							dist = distance(p.x, p.y, k.Coord[0], k.Coord[1]);			//���� ���������� ����������
							Dead = k;
						}
					}
					Dead.Draw(hdc, { 255,255,0,0 });				//���������� � ������� ���� ����� ���������
					Pikes.remove(Dead);								//������� �� ���� ����, ������ �������
				}
			}
		}
	}
};

TAquarium Aquarium;		//��� ����� ��������  �� ���������
TKarp Karp;				//������� �������������� ������� ����� � ����. �������, ���
TPike Pike;				//���� � ��������� ����� ������������ �� ���� ���� ��������.

DWORD WINAPI tickThreadProc(HANDLE handle) {
	Sleep(500);				//��� ��������� ����� ������������
	HDC hdc = GetDC(hWnd);	//�������� DC ��������� �����
	for (;; ) {				//������ ���� ��������
		if (Aquarium.done == 0) Aquarium.Init(hdc);		//������ ��������, ���� �� ���������
		if (pause != 1)Aquarium.Run();					//��������� ���, ���� �� �����
		Sleep(1000/fps);								//��������� ������� ������
	}
	DeleteDC(hdc);
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);					//���������� ��������� �� ����

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, INT iCmdShow)
{
	MSG                 msg;
	WNDCLASS            wndClass;
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;

	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);			// ������������ GDI+.

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

	hWnd = CreateWindow(				//������� ���� ���������
		TEXT("Aquarium"),
		TEXT("��������"),
		WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,	//������� ����� THICKFRAME, ����� ������ ����� ����������� ������ ������ ����.
		CW_USEDEFAULT,
		CW_USEDEFAULT, 
		800,
		800,
		NULL,
		NULL,
		hInstance, 
		NULL); 

	ShowWindow(hWnd, iCmdShow);
	UpdateWindow(hWnd);
										//���� ��� �������� ����
	HMENU hMenubar = CreateMenu();		//�������� ������ ����
	HMENU hFishes = CreateMenu();		//������ ���� "��������/������ ����, �����"
	HMENU hAquarium = CreateMenu();		//������ ���� "����������, �����, �����"

	AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hFishes, L"����");			//������ � L, ����� ��������� char* � LPCWSTR
	AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hAquarium, L"��������");

	AppendMenu(hFishes, MF_STRING, ID_Add, L"��������...");
	AppendMenu(hFishes, MF_STRING, ID_Remove, L"�������...");
	AppendMenu(hFishes, MF_STRING, ID_Debug, L"�����");

	AppendMenu(hAquarium, MF_STRING, ID_Restart, L"����������");
	AppendMenu(hAquarium, MF_STRING, ID_Pause, L"�����");
	AppendMenu(hAquarium, MF_STRING, ID_Exit, L"�����");

	SetMenu(hWnd, hMenubar);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	GdiplusShutdown(gdiplusToken);
	return msg.wParam;

} 

HWND FWnd = NULL;									//�������� ���� ���������� (�������� ����).
BOOL CALLBACK DLGProc(HWND, UINT, WPARAM, LPARAM);	//���������� ��������� �� ��������� ����

void AddFish()							//������ ���� ���������� ����� ����
{
	FWnd = CreateDialog(
		GetModuleHandle(NULL),			//���������� ������� hinstance.
		MAKEINTRESOURCE(IDD_ADDFISH),	//���� �������� �� ���������� ���������� ������� ADDFISH �� ����� Aquarium.rc
		hWnd,
		DLGProc
	);
	CheckRadioButton(FWnd, 699, 1007, 699);	//�������� ��� ���� ���� �� ��������� (699 - ������ ����)

	SendDlgItemMessage(FWnd,				//������������ ���� �� ���� ��������
		IDC_FSIZE,
		EM_LIMITTEXT,
		(WPARAM)2,
		(LPARAM)0);
	ShowWindow(FWnd, SW_SHOW);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message,	//���������� ��������� �� ��������� ����
	WPARAM wParam, LPARAM lParam)
{
	srand(time(NULL));						//������ ������ ���������

	switch (message)
	{
	case WM_CREATE: {						//��� �������� ���� ��������� ���� ��� ��������.
		
		hTickThread = CreateThread(NULL, NULL, &tickThreadProc, NULL, NULL, NULL);

		break; }

	case WM_COMMAND: {						//������ ���������������� ������� �� ����
		if (LOWORD(wParam) == ID_Exit) {	//��������� ����.
			exit(0);		
		}
		if (LOWORD(wParam) == ID_Remove) {	//������� ���
			MessageBox(hWnd,				//��� ������ ��� ���������� ������������
				L"�������� �� ����, ����� �������",
				L"����",
				MB_OK | MB_ICONINFORMATION);
			break;
		}
		if (LOWORD(wParam) == ID_Add) {		//��������� ���� � ��������
			AddFish();
		}
		if (LOWORD(wParam) == ID_Restart) {	//���������� �������� (���� �� ��������).
			Aquarium.Done();
		}
		if (LOWORD(wParam) == ID_Pause) {	//������������� ��������.
			pause = !pause;	
		}
		if (LOWORD(wParam) == ID_Debug) {	//����� ��.
			test = !test;
			for (auto& i : Aquarium.Karps)
				i.Test = test;	
			for (auto& i : Aquarium.Pikes)
				i.Test = test;
		}
		break; }
	case WM_LBUTTONDOWN:{					//������ ������� ���� ��� �������� ���
		Aquarium.RemoveFish();
		break; }
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}	// WndProc

BOOL CALLBACK DLGProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) //���������� ��������� �� ��������� ����
{
	TCHAR FishSize[2];	//�����, � ������� �������� ������ ����, ������������ �������� �����
	switch (message)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			SendDlgItemMessage(FWnd,								//���������� ��������� ���� ��������� � �������� �������� ���������� ������ "������"
				IDC_FSIZE,
				EM_GETLINE,
				(WPARAM)0,
				(LPARAM)FishSize);									//���������� ����� �������� � ����� "FishSize", ������� ����� ��������� � int ����� _ttoi

			if (_ttoi(FishSize) < 15 || _ttoi(FishSize) > 50)			//��� ������������ ������� ������ ����������
			{
				MessageBox(FWnd,
					L"������� ������ �� 15 �� 50 px",
					L"������",
					MB_OK);
				break;
			}

			if (IsDlgButtonChecked(FWnd, IDC_KARP) == BST_CHECKED) {//���� ���� (699) ������, ��������� �����. ����� ����. ���� �� ����� ������� �� ���������
				Karp.Init(
					KarpC,
					_ttoi(FishSize),
					test
				);	
				Aquarium.Add(Karp);
				EndDialog(hwndDlg, wParam);
				return TRUE;
			}

			else {
				Pike.Init(
					PikeC,
					_ttoi(FishSize),
					test
				);
				Aquarium.Add(Pike);
				EndDialog(hwndDlg, wParam);
				return TRUE;
			}
		case IDCANCEL:
			EndDialog(hwndDlg, wParam);
			return TRUE;
		}
	}
	return FALSE;
}	//DLGProc