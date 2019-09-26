#include "stdafx.h"
#include "Aquarium.h"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <ctime>        //Для рандома
#include <math.h>       //Для геометрических формул
#include <list>
#include <iterator>
using namespace Gdiplus;
using namespace std;
#pragma comment (lib,"Gdiplus.lib")

#define PI 3.14159265   //Для перевода, т.к. math.h предпочитает радианы
						//Инициализуем ID для меню
#define ID_Add 1		//Добавить рыбу
#define ID_Remove 2		//Убрать рыбу
#define ID_Pause 3		//Остановить анимацию
#define ID_Exit 4		//Закрыть окно
#define ID_Restart 5	//Регенерация аквариума
#define ID_Debug 6		//Отобразить ИИ рыб

HANDLE hTickThread;		//Указатель на анимационный тред
HWND hWnd;				//Основное окно приложения
HWND nWnd;				//Второе окно для ввода данных
HDC aquarium;			//КУ с пустым аквариумом
bool pause = 0;			//Пауза анимации
bool test = 0;			//Тест ИИ
const int fps = 30;		//Скорость анимации
Color KarpC = { 255,240,100,50 };
Color PikeC = { 255,50,200,100 };

int distance(int x1, int  y1, int  x2, int  y2) {			//Просто для удобства: находит расстояние между двумя точками.
	int d;
	d = sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2));		//d = sqrt((x2-x1)^2+(y2-y1)^2)	
	return d;
}

struct TFish {			//Абстрактная рыба

private:				//Частное: стартовые переменные - задаются в Init и не меняются; HDC - задаётся через метод; методы Draw, Look и AITest - используются рекурсивно в методе Draw

	HDC hdc;			//Контекст устройства

	virtual void Look(HDC hdc)				//Зрение рыбы - три луча из носа. Обзор 100 градусов, дальность - два размера рыбы. Определяют цвет и дистанцию (в пикселях i) до объекта.
	{
		for (int i = 1; i < Size; i++)	//Дальность - случайное значение, благо у нас нет рыб, пролетающих две собственные длины за кадр.
		{
			COLORREF CRF_Left = GetPixel(hdc, Coord[0] - i * cos((Dir - 50) * PI / 180), Coord[1] - i * sin((Dir - 50) * PI / 180));	//Левый луч зрения
			if (CRF_Left != 0x00ffffeb && CRF_Left != TColor.ToCOLORREF()) {															//При обзоре игнорируем цвет фона (белый) и собственный цвет.
				Dir += 45;																												//Иначе поворачиваем.
				break;																													//При препятствии сразу забиваем на остаток вычислений
			}
			COLORREF CRF_Mid = GetPixel(hdc, Coord[0] - i * cos(Dir * PI / 180), Coord[1] - i * sin(Dir * PI / 180));					//Центральный луч
			if (CRF_Mid != 0x00ffffeb && CRF_Mid != TColor.ToCOLORREF()) {
				Dir += 180;
				break;
			}
			COLORREF CRF_Right = GetPixel(hdc, Coord[0] - i * cos((Dir + 50) * PI / 180), Coord[1] - i * sin((Dir + 50)* PI / 180));	//Правый луч (Мб левый, не проверял))))
			if (CRF_Right != 0x00ffffeb && CRF_Right != TColor.ToCOLORREF()) {
				Dir -= 45;
				break;
			}
		}
	}

	void AITest(HDC hdc, Color color) {				//Дебаг - рисуем область зрения рыбы (TFish::Look). 

		Graphics graphics(hdc);
		Pen red(color, 1);
		int x = (Coord[0] - cos((Dir)* PI / 180));	//DrawLine не способен решить, работать ему с int или float, из-за чего останавливает компиляцию. Поэтому насильно делаем один из аргументов int.
		graphics.DrawLine(&red,						//Формулы прямых прямо из Look
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

	bool Test = 0;			//Переключатель режима разработчика
	int Coord[2];			//Координаты носа (Если у рыб есть нос?). 0 - х, 1 - у
	int Dir;				//Направление в градусах
	int Size;				//Размер в пикселях
	int Speed;				//Скорость в пикселях, >1
	Color TColor;			//ARGB

	void Init(				//Устанавливает значения рыбы. 
		Color iColor,		//Мы не можем привязать цвет рыбы к типу рыбы, потому что он динамически меняется (на белый)
		int iSize,
		bool iTest			//По той же причине у Test не статическое значение
	) {
		TColor = iColor;
		Size = iSize;
		Test = iTest;
		Speed = 50 / Size+1;//Скорость примерно определяется размером
	}

	virtual void Draw(HDC hdc, Color color)		//Рисует рыбу. Выбор цвета ручки нужен для стирания (а точнее, закрашивания) предыдущего кадра.
	{
		Graphics graphics(hdc);
		Pen   pen(color, Size / 5);
		SolidBrush fill(color);

		graphics.DrawLine(&pen, Coord[0], Coord[1], Coord[0] + Size * cos((Dir + 30)*PI / 180), Coord[1] + Size * sin((Dir + 30)*PI / 180));		//Туша из двух линий (уголка) и глаза по формуле точки
		graphics.DrawLine(&pen, Coord[0], Coord[1], Coord[0] + Size * cos((Dir - 30)*PI / 180), Coord[1] + Size * sin((Dir - 30)*PI / 180));        //на окружности x=rcosf, y=rsinf; угол между линиями 60.
		graphics.DrawEllipse(&pen, Coord[0] + Size / 2 * cos((Dir)*PI / 180), Coord[1] + Size / 2 * sin((Dir)*PI / 180), Size / 8, Size / 8);       //Глаз выглядит так, будто рыба готова тебе втащить. Фича.
	}

	void Run(HDC hdc)							//Двигает рыбу (+ ИИ и проч.)
	{
		Graphics graphics(hdc);
		Color Water(255, 235, 255, 255);

		Draw(hdc, Water);											//Закрашиваем рыбу предыдущем кадре фоновым цветом
		if (Test == 1) AITest(hdc, Water);							//В тестовом режиме также закрашивает всё тестовое

		Coord[0] -= Speed * cos(Dir*PI / 180);                      //Двигаем рыбу по направлению Dir со скоростью Speed.
		Coord[1] -= Speed * sin(Dir*PI / 180);
		if (rand() % 10 < 6) {
			Dir += rand() % 31 - 15;                                //Двойной рандом: с вероятностью 60% поворачивает от 0 до 15 влево или вправо
		}

		Look(hdc);
		Draw(hdc, TColor);                                          //Рисуем новый кадр
		if (Test == 1) AITest(hdc, TColor);
	}
};

struct TKarp : public TFish {										//Карп как тип рыбы.

	bool operator ==(const TKarp& K)								//Нам прийдётся сравнивать карпов (struct==struct), что C++ по умолчанию не может. Поэтому делаем свой оператор. 
	{
		return Coord[0] == K.Coord[0] && Coord[1] == K.Coord[1];	//"Равными" считаются карпы с одинаковыми координатами. Не идеальное решение, но если два карпа чудом окажутся в одной точке перед щукой, они оба умрут.
	}

	void Draw(HDC hdc, Color color) {								//У карпов свой стиль рисования, поэтому своя функция Draw

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

struct TPike : public TFish {								//Щука как тип рыбы.

	bool Kill;												//Показывает, что щука кого-то убила в данном кадре.

/*
Про Kill:
По ТЗ, щуки должны есть карпов. Для этого (опять по ТЗ) нужно перебрать список карпов и удалить ближайшего к щуке.
Список карпов есть объект аквариума (ТЗ). Следовательно, для работы со списком нужен указатель на аквариум.
Здесь проблема: мы не можем хранить указатель на аквариум в объекте щуки - ведь щука сама есть
принадлежность аквариума (ТЗ), а ребёнок не может создать собственного родителя без перемещений во времени.
Чтобы обойти проблему, мы даём щуке сигнал "Хочу есть", а её родитель каждый кадр ищет этот сигнал и кормит щуку.
*/

	bool operator ==(const TPike& K)								//Нам прийдётся сравнивать щук (struct==struct), что C++ по умолчанию не может. Поэтому делаем свой оператор. 
	{
		return Coord[0] == K.Coord[0] && Coord[1] == K.Coord[1];	//"Равными" считаются щуки с одинаковыми координатами.
	}

	void Draw(HDC hdc, Color color) { 						//У щук свой стиль рисования, поэтому своя функция Draw

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

	void Look(HDC hdc) override				//Зрение рыбы - три луча из носа. Обзор 100 градусов, дальность - два размера рыбы. Определяют цвет и дистанцию (в пикселях i) до объекта.
	{
		Kill = 0;

		for (int i = 1; i < Size; i++)	//Дальность - случайное значение, благо у нас нет рыб, пролетающих две собственные длины за кадр.
		{
			COLORREF CRF_Left = GetPixel(hdc, Coord[0] - i * cos((Dir - 50) * PI / 180), Coord[1] - i * sin((Dir - 50) * PI / 180));	//Левый луч зрения

			if (CRF_Left == 0x003264f0) {																								//При виде карпа поворачиваем к нему
				Dir -= 25;
			}
			else if (CRF_Left != 0x00ffffeb && CRF_Left != TColor.ToCOLORREF()) {														//Если не карп, не фон и не другая щука, отворачиваемся
				Dir += 45;
				break;																													//При препятствии сразу забиваем на остаток вычислений
			}

			COLORREF CRF_Mid = GetPixel(hdc, Coord[0] - i * cos(Dir * PI / 180), Coord[1] - i * sin(Dir * PI / 180));					//Центральный луч

			if (CRF_Mid == 0x003264f0) {																								//Подтверждаем убийство при виде карпа
				Kill = 1;
			}
			else if (CRF_Mid != 0x00ffffeb && CRF_Mid != TColor.ToCOLORREF()) {
				Dir += 180;
				break;
			}

			COLORREF CRF_Right = GetPixel(hdc, Coord[0] - i * cos((Dir + 50) * PI / 180), Coord[1] - i * sin((Dir + 50)* PI / 180));	//Правый луч (Мб левый, не проверял))))

			if (CRF_Right == 0x003264f0) {																								//При виде карпа поворачиваем к нему
				Dir += 25;
			}
			else if (CRF_Right != 0x00ffffeb && CRF_Right != TColor.ToCOLORREF()) {
				Dir -= 45;
				break;
			}
		}
	}
};

struct TAquarium {	//Объект аквариума

private:

	int x0 = 90;    //Координаты аквариума: верхний левый угол + полстекла.
	int y0 = 60;	//Отступ сверху меньше, потому что часть окна занимает меню.
	int aw = 600;   //Размеры аквариума: ширина, высота.
	int ah = 600;
	HDC hdc;

public:

	bool done = 0;				//Переключатель: готов ли аквариум
	std::list<TKarp> Karps;		//Стая карпов
	std::list<TPike> Pikes;		//Стая щук

	void Init(HDC Ahdc) {		//Рисует аквариум и его содержимое
		hdc = Ahdc;

		Graphics graphics(hdc);
		Pen      Glass(Color(255, 100, 200, 200), 20);		//Рассчётная толщина стекла, от края до края, 20 пикселей.
		SolidBrush Water(Color(255, 235, 255, 255));
		SolidBrush Rock(Color(255, 100, 100, 100));

		graphics.FillRectangle(&Water, x0, y0, aw, ah);		//Стенки аквариума
		graphics.DrawRectangle(&Glass, x0, y0, aw, ah);		//Стенки аквариума

		for (int i = 0; i < 10; i++) {						//Процедурно сгенерированные ландшафты!! Некст ген и пр.
			int x1 = x0 + rand() % (aw - 70) + 10;			//Чтобы не кидать камни в стекло, задаём для них область. Камни рисуются от верхнего левого угла.
			int y1 = y0 + rand() % (ah - 70) + 10;			//Удобная местность: от верхнего левого угла + стекло, до нижнего правого - (стекло + тощина камня).
			graphics.FillEllipse(&Rock, x1, y1, rand() % 20 + 30, rand() % 20 + 30);
		}

		aquarium = CreateCompatibleDC(hdc);
		HBITMAP initbmp = CreateCompatibleBitmap(hdc, 800, 800);
		SelectObject(aquarium, initbmp);
		BitBlt(aquarium, 0, 0, 800, 800, hdc, 0, 0, SRCCOPY);				//Сохраняем пустой аквариум, чтобы случайно не стереть его позднее.
		done = 1;
	}

	void Run() {	//Анимация с тройным буфером: основной (видимый) DC; буферный DC, где проводится всё рисование; DC с аквариумом, чтобы не рисовать поверх него.
		HDC		mdc = CreateCompatibleDC(hdc);					//Буферный DC (Memory DC) для операций за кадром
		HBITMAP mbmp = CreateCompatibleBitmap(hdc, 800, 800);	//Буферная bmp для MDC
		SelectObject(mdc, mbmp);
		BitBlt(mdc, 0, 0, 800, 800, hdc, 0, 0, SRCCOPY);		//Делаем копию текущего кадра и отправляем её в MDC для обработки

		BitBlt(mdc, 0, 0, 800, 800, aquarium, 0, 0, SRCCOPY);	//Восстанавливаем аквариум на случай, если предыдущая операция его повредила.

		for (auto& i : Karps) {									//Двигаем всех карпов, найденных в аквариуме.
			i.Run(mdc);
		}

		for (auto& i : Pikes) {									//Двигаем всех щук, найденных в аквариуме.
			i.Run(mdc);											//Интересный факт: если двигать щук перед карпами, ИИ не работает.

			if (i.Kill == 1) {									//Если щука намерена убить, проверяем всех карпов. Ближайший карп к щуке-убийце должен быть удалён.

				TKarp Dead = Karps.front();														//Целевой карп как переменная, по умолчанию первый карп в списке.
				int dist = distance(i.Coord[0], i.Coord[1], Dead.Coord[0], Dead.Coord[1]);		//Расстояние до цели. По умолчанию - до первого карпа в списке.

				for (auto& k : Karps) {
					if (distance(i.Coord[0], i.Coord[1], k.Coord[0], k.Coord[1]) < dist)
					{
						dist = distance(i.Coord[0], i.Coord[1], k.Coord[0], k.Coord[1]);		//Ищем наименьшее расстояние
						Dead = k;
					}
				}
				i.Size += (Dead.Size / 5);						//При питании щука жиреет пропорционально блюду
				i.Speed = 50 / i.Size + 1;
				Karps.remove(Dead);								//Убираем из стаи карпа, равного целевому
			}
		}

		BitBlt(hdc, 0, 0, 800, 800, mdc, 0, 0, SRCCOPY);		//Переносим картинку из MDC в основной HDC.
		DeleteObject(mbmp);										//Очищаем буфер
		DeleteDC(mdc);
	}

	void Add(TKarp Fish) {											//Метод добавления рыбы в аквариум, с вариацией для карпов и щук
		Fish.Coord[0] = x0 + rand() % (aw - 20 - Fish.Size * 2) + 20 + Fish.Size;	//Кидаем её куда попало. Отличие от генерации камней в том, что
		Fish.Coord[1] = y0 + rand() % (ah - 20 - Fish.Size * 2) + 20 + Fish.Size;	//рыба рисуется от "центра", поэтому отступ размером с рыбу делаем со всех сторон.
		Fish.Dir = rand() % 360;
		Karps.push_back(Fish);
	}

	void Add(TPike Fish) {
		Fish.Coord[0] = x0 + rand() % (aw - 20 - Fish.Size * 2) + 20 + Fish.Size;
		Fish.Coord[1] = y0 + rand() % (ah - 20 - Fish.Size * 2) + 20 + Fish.Size;
		Fish.Dir = rand() % 360;
		Pikes.push_back(Fish);
	}

	void Done() {		//Метод паузы анимации.
		done = !done;
	}

	void RemoveFish() {
		POINT p;							//Хранит координаты курсора
		if (GetCursorPos(&p)) {				//Записывает координаты курсора
			if (ScreenToClient(hWnd, &p))	//Переводит коорды в локальные на окне
			{
				if (GetPixel(hdc, p.x, p.y) == KarpC.ToCOLORREF()) {	//Проверяем цвет рыбы. Если карп, копипаста кода пожирания карпов

					TKarp Dead = Karps.front();											//Целевая рыба как переменная, по умолчанию первая в списке.
					int dist = distance(p.x, p.y, Dead.Coord[0], Dead.Coord[1]);		//Расстояние до цели. По умолчанию - до первой рыбы в списке.

					for (auto& k : Karps) {
						if (distance(p.x, p.y, k.Coord[0], k.Coord[1]) < dist)
						{
							dist = distance(p.x, p.y, k.Coord[0], k.Coord[1]);			//Ищем наименьшее расстояние
							Dead = k;
						}
					}
					Dead.Draw(hdc,{ 255,255,0,0 });					//Окрашивает в красный рыбу перед удалением
					Karps.remove(Dead);								//Убираем из стаи рыбу, равную целевой
				}

				else if (GetPixel(hdc, p.x, p.y) == PikeC.ToCOLORREF()) {	//То же для щук

					TPike Dead = Pikes.front();											//Целевая рыба как переменная, по умолчанию первая в списке.
					int dist = distance(p.x, p.y, Dead.Coord[0], Dead.Coord[1]);		//Расстояние до цели. По умолчанию - до первой рыбы в списке.

					for (auto& k : Pikes) {
						if (distance(p.x, p.y, k.Coord[0], k.Coord[1]) < dist)
						{
							dist = distance(p.x, p.y, k.Coord[0], k.Coord[1]);			//Ищем наименьшее расстояние
							Dead = k;
						}
					}
					Dead.Draw(hdc, { 255,255,0,0 });				//Окрашивает в красный рыбу перед удалением
					Pikes.remove(Dead);								//Убираем из стаи рыбу, равную целевой
				}
			}
		}
	}
};

TAquarium Aquarium;		//Тот самый аквариум  по умолчанию
TKarp Karp;				//Заранее инициализируем объекты карпа и щуки. Позднее, все
TPike Pike;				//рыбы в аквариуме будут базироваться на этих двух объектах.

DWORD WINAPI tickThreadProc(HANDLE handle) {
	Sleep(500);				//Даём основному треду прогрузиться
	HDC hdc = GetDC(hWnd);	//Получаем DC основного треда
	for (;; ) {				//Вечный цикл анимации
		if (Aquarium.done == 0) Aquarium.Init(hdc);		//Рисуем аквариум, если не нарисован
		if (pause != 1)Aquarium.Run();					//Анимируем рыб, если не пауза
		Sleep(1000/fps);								//Соблюдаем частоту кадров
	}
	DeleteDC(hdc);
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);					//Обработчик сообщений от окна

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, INT iCmdShow)
{
	MSG                 msg;
	WNDCLASS            wndClass;
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;

	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);			// Инициализуем GDI+.

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

	hWnd = CreateWindow(				//Главное окно программы
		TEXT("Aquarium"),
		TEXT("Аквариум"),
		WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,	//Убираем стиль THICKFRAME, чтобы лишить юзера возможности менять размер окна.
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
										//Меню для главного окна
	HMENU hMenubar = CreateMenu();		//Основная панель меню
	HMENU hFishes = CreateMenu();		//Раздел меню "Добавить/Убрать рыбу, Дебаг"
	HMENU hAquarium = CreateMenu();		//Раздел меню "Перезапуск, Пауза, Выход"

	AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hFishes, L"Рыбы");			//Строки с L, чтобы перевести char* в LPCWSTR
	AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hAquarium, L"Аквариум");

	AppendMenu(hFishes, MF_STRING, ID_Add, L"Добавить...");
	AppendMenu(hFishes, MF_STRING, ID_Remove, L"Удалить...");
	AppendMenu(hFishes, MF_STRING, ID_Debug, L"Дебаг");

	AppendMenu(hAquarium, MF_STRING, ID_Restart, L"Переделать");
	AppendMenu(hAquarium, MF_STRING, ID_Pause, L"Пауза");
	AppendMenu(hAquarium, MF_STRING, ID_Exit, L"Выход");

	SetMenu(hWnd, hMenubar);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	GdiplusShutdown(gdiplusToken);
	return msg.wParam;

} 

HWND FWnd = NULL;									//Дочернее окно приложения (Добавить рыбу).
BOOL CALLBACK DLGProc(HWND, UINT, WPARAM, LPARAM);	//Обработчик сообщений от дочернего окна

void AddFish()							//Создаёт окно добавления новой рыбы
{
	FWnd = CreateDialog(
		GetModuleHandle(NULL),			//Возвращают текущую hinstance.
		MAKEINTRESOURCE(IDD_ADDFISH),	//Окно делается по специально созданному ресурсу ADDFISH из файла Aquarium.rc
		hWnd,
		DLGProc
	);
	CheckRadioButton(FWnd, 699, 1007, 699);	//Выбирает тип рыбы Карп по умолчанию (699 - кнопка Карп)

	SendDlgItemMessage(FWnd,				//Ограничиваем ввод до двух символов
		IDC_FSIZE,
		EM_LIMITTEXT,
		(WPARAM)2,
		(LPARAM)0);
	ShowWindow(FWnd, SW_SHOW);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message,	//Обработчик сообщений от основного окна
	WPARAM wParam, LPARAM lParam)
{
	srand(time(NULL));						//Делаем рандом рандомным

	switch (message)
	{
	case WM_CREATE: {						//При создании окна отдельный тред для анимации.
		
		hTickThread = CreateThread(NULL, NULL, &tickThreadProc, NULL, NULL, NULL);

		break; }

	case WM_COMMAND: {						//Читаем пользовательские команды из меню
		if (LOWORD(wParam) == ID_Exit) {	//Закрывает окно.
			exit(0);		
		}
		if (LOWORD(wParam) == ID_Remove) {	//Удаляет рыб
			MessageBox(hWnd,				//Для начала даёт инструкцию пользователю
				L"Кликните по рыбе, чтобы удалить",
				L"Инфо",
				MB_OK | MB_ICONINFORMATION);
			break;
		}
		if (LOWORD(wParam) == ID_Add) {		//Добавляет рыбу в аквариум
			AddFish();
		}
		if (LOWORD(wParam) == ID_Restart) {	//Пересоздаёт аквариум (рыбы не меняются).
			Aquarium.Done();
		}
		if (LOWORD(wParam) == ID_Pause) {	//Останавливает анимацию.
			pause = !pause;	
		}
		if (LOWORD(wParam) == ID_Debug) {	//Дебаг ИИ.
			test = !test;
			for (auto& i : Aquarium.Karps)
				i.Test = test;	
			for (auto& i : Aquarium.Pikes)
				i.Test = test;
		}
		break; }
	case WM_LBUTTONDOWN:{					//Читает нажатия мыши для удаления рыб
		Aquarium.RemoveFish();
		break; }
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}	// WndProc

BOOL CALLBACK DLGProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) //Обработчик сообщений от дочернего окна
{
	TCHAR FishSize[2];	//Буфер, в котором хранится размер рыбы, передаваемый дочерним окном
	switch (message)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			SendDlgItemMessage(FWnd,								//Отправляем дочернему окну сообщение с просьбой передать содержимое строки "Размер"
				IDC_FSIZE,
				EM_GETLINE,
				(WPARAM)0,
				(LPARAM)FishSize);									//Полученный ответ помещаем в буфер "FishSize", который позже переводим в int через _ttoi

			if (_ttoi(FishSize) < 15 || _ttoi(FishSize) > 50)			//При некорректном размере просим переделать
			{
				MessageBox(FWnd,
					L"Введите размер от 15 до 50 px",
					L"Ошибка",
					MB_OK);
				break;
			}

			if (IsDlgButtonChecked(FWnd, IDC_KARP) == BST_CHECKED) {//Если карп (699) выбран, добавляем карпа. Иначе щуку. Одна из опций выбрана по умолчанию
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