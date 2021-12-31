#include <iostream>
#include <fstream>
#include <time.h>
#include <sstream>

#if defined _WIN32
#include <windows.h>
#elif defined(__LINUX__) || defined(__gnu_linux__) || defined(__linux__) || defined(__APPLE__)
#include "stdio.h"
#include <signal.h>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

using namespace std;

void Clear()
{
#if defined _WIN32
	system("cls");
#elif defined(__LINUX__) || defined(__gnu_linux__) || defined(__linux__) || defined(__APPLE__)
	system("clear");
#endif
}

void sleep(int ms)
{
#if defined _WIN32
	Sleep(ms);
#elif defined(__LINUX__) || defined(__gnu_linux__) || defined(__linux__) || defined(__APPLE__)
	this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}

#define up 1
#define right 2
#define down 3
#define left 4
#define escape 5

#define KEY_UP 72
#define KEY_DOWN 80
#define KEY_RIGHT 75
#define KEY_LEFT 77
#define ESCAPE 27

#define ESC "\033"
#define UP "\033[A"
#define DOWN "\033[B"
#define LEFT "\033[D"
#define RIGHT "\033[C"

static struct termios oldtio;
static struct termios curtio;

void term_setup(void (*sighandler)(int)){
    struct sigaction sa;
    tcgetattr(0, &oldtio);

    if(sighandler){
        memset(&sa, 0, sizeof(struct sigaction));
        sa.sa_handler = sighandler;
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGQUIT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);
    }
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = SIG_IGN;
    sigaction(SIGTTOU, &sa, NULL);

    tcgetattr(0, &curtio);
    curtio.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &curtio);
}

void term_restore(){
    tcsetattr(0, TCSANOW, &oldtio);
}

static char kbhitChar[4]= {0};
bool kbhit(){
    struct pollfd pfds[1];
    int ret;
    memset(kbhitChar, 0, sizeof(char) * 4);

    pfds[0].fd = 0;
    pfds[0].events = POLLIN;
    ret = poll(pfds, 1, 0);

    if (ret > 0) {
        read(0, kbhitChar, 3);
        return strlen(kbhitChar) > 0;
    }
    return false;
}

bool keydown(const char* key){
    return !strcmp(kbhitChar, key);
}

bool ButtonClickOnSystems(int button)
{
#if defined _WIN32
	switch (button)
	{
	case up:
		return GetAsyncKeyState(VK_UP);
		break;
	case right:
		return GetAsyncKeyState(VK_RIGHT);
		break;
	case down:
		return GetAsyncKeyState(VK_DOWN);
		break;
	case left:
		return GetAsyncKeyState(VK_LEFT);
		break;
	case escape:
		return GetAsyncKeyState(VK_ESCAPE);
		break;
	default:
		return false;
	}
#elif defined(__LINUX__) || defined(__gnu_linux__) || defined(__linux__) || defined(__APPLE__)
	switch (button)
	{
	case up:
		return keydown(UP);
	case right:
		return keydown(RIGHT);
	case down:
		return keydown(DOWN);
	case left:
		return keydown(LEFT);
	case escape:
		return keydown(ESC);
	default:
		return false;
	}
#endif
}

/*
Tablice działają tak:
map[3]         map[3][5]
[Wartość1]      [Wartosc1] [Wartosc2] [Wartosc3] [Wartosc4] [Wartosc5]
[Wartość2]      [Wartosc6] [Wartosc7] [Wartosc8] [Wartosc9] [Wartosc10]
[Wartość3]      [Wartosc11][Wartosc12][Wartosc15][Wartosc14][Wartosc15]

map[2] == Wartość 2
map[3][2] == Wartość 12
map[2][4] == Wartość 9

W działaniach logicznych używasz normalnie x i y, ale korzystając z tablic pamiętaj o odwrotności!
*/

const int iMaxPunktow = 15;
int iloscDuchow = 3;
int iRozmiarMapyX = 50;
int iRozmiarMapyY = 25;
int iTablicaMapy[25][50]; // Zawiera każdy znak z mapy. Najpierw Y, potem X, bo tak działają tablice

#define WOLNE 32
#define MUR 35
#define GRACZ 88
#define PunktNaMapie 42
#define DUCH 79
#define BUFF 36
#define DEBUFF 45

clock_t currentTime, previousFrame;
int deltaTime;
bool bGranie = true;

// WINDOWS
static sig_atomic_t finish = 0;
static void sighandler(int signo)
{
	finish = 1;
	bGranie = false;
	printf("good bye!\n");
}

class Player
{
public:
	int X = 0;
	int Y = 0;
	int score = 0;
	int speed = 0;
	int Delta = 0;
	int PozostalyCzas = 5000;
	bool buff = false;
	bool debuff = false;
	int MnoznikBuffa = 5;

public:
	Player(int x = 0, int y = 0, int s = 0) // Wartości domyślne, żeby klasa duch mogła mieć własny konstruktor
	{
		X = x;
		Y = y;
		speed = s;
	}
	void ZmienPozycje(int pozX, int pozY) // Zmień pozycję, jeśli X i Y są większe lub równe 0
	{
		if (pozX >= 0)
			X = pozX;
		if (pozY >= 0)
			Y = pozY;
	}
	void SprawdzMape(int pozX, int pozY, int przeciwnik) // Fragment liczący punkty, buffy i zderzenie z duszkiem
	{
		if ((iTablicaMapy[pozY][pozX] == PunktNaMapie)) // Jeżeli postać jest na punkcie, dodaj punkt i zmień pole na wolne
		{
			score++;
			iTablicaMapy[pozY][pozX] = WOLNE;
		}
		if ((iTablicaMapy[pozY][pozX] == BUFF)) // Jeżeli postać jest na buffie
		{
			if (PozostalyCzas <= 5000 && buff == false) // Jeżeli pozostały czas jest mniejszy lub równy 5000ms i buff jest nieaktywny, to zmień prędkość i ustaw buffa na true
			{
				speed = speed / MnoznikBuffa;
				buff = true;
			}
			else if (PozostalyCzas <= 5000 && buff == true) // Jeżeli pozostały czas jest mniejszy lub równy 5000ms i buff jest aktywny, to dodaj do czasu 5000ms
			{
				PozostalyCzas = PozostalyCzas + 5000;
			}
			iTablicaMapy[pozY][pozX] = WOLNE; // Usuń buffa z mapy
		}
		if (buff == true) // Jeżeli buff jest aktywny
		{
			if (PozostalyCzas <= 0) // Jeśli czas się skoczył to prędkość wraca do wartości początkowej, wyłącza się buff i postały czas się resetuje
			{
				speed = speed * MnoznikBuffa;
				buff = false;
				PozostalyCzas = 5000;
			}
		}

		if ((iTablicaMapy[pozY][pozX] == DEBUFF)) // Jeżeli postać jest na debuffie
		{
			if (PozostalyCzas <= 5000 && debuff == false) // Jeżeli pozostały czas jest mniejszy lub równy 5000ms i debuff jest nieaktywny, to zmień prędkość i ustaw debuffa na true
			{
				speed = speed * MnoznikBuffa;
				debuff = true;
			}
			else if (PozostalyCzas <= 5000 && debuff == true) // Jeżeli pozostały czas jest mniejszy lub równy 5000ms i debuff jest aktywny, to dodaj do czasu 5000ms
			{
				PozostalyCzas = PozostalyCzas + 5000;
			}
			iTablicaMapy[pozY][pozX] = WOLNE; // Usuń debuffa z mapy
		}
		if (debuff == true) // Jeżeli debuff jest aktywny
		{
			if (PozostalyCzas <= 0) // Jeśli czas się skoczył to prędkość wraca do wartości początkowej, wyłącza się debuff i postały czas się resetuje
			{
				speed = speed / MnoznikBuffa;
				debuff = false;
				PozostalyCzas = 5000;
			}
		}
		if (iTablicaMapy[pozY][pozX] == przeciwnik) // Przegraj grę jeśli zderzy się z duchem
		{
			bGranie = false;
		}
	}
	bool CzyMozeSieRuszyc(int iPozX, int iPozY, int Kierunek) // Sprawdza czy gracz może się ruszyć. Sprawdza czy jest MUR. Najpierw Y, potem X, bo tak działają tablice
	{
		switch (Kierunek)
		{
		case 1:
		{
			if (iTablicaMapy[iPozY + 1][iPozX] != MUR) return true;
			else return false;
		}
		case 2:
		{
			if (iTablicaMapy[iPozY - 1][iPozX] != MUR) return true;
			else return false;
		}
		case 3:
		{
			if (iTablicaMapy[iPozY][iPozX - 1] != MUR) return true;
			else return false;
		}
		case 4:
		{
			if (iTablicaMapy[iPozY][iPozX + 1] != MUR) return true;
			else return false;
		}
		default:
			return false;
		}
	}
	void ruch()
	{
		if (buff == true || debuff == true)
		{
			PozostalyCzas = PozostalyCzas - deltaTime; // Odejmij czas co każde wykonanie
		}

		iTablicaMapy[Y][X] = GRACZ;
		Delta += currentTime - previousFrame;

		if (speed * 50 < Delta)
		{
			#if defined(__LINUX__) || defined(__gnu_linux__) || defined(__linux__) || defined(__APPLE__)
			kbhit();
			#endif

			if (ButtonClickOnSystems(down))
			{
				if (CzyMozeSieRuszyc(X, Y, 1))
				{
					SprawdzMape(X, Y + 1, DUCH);
					iTablicaMapy[Y][X] = WOLNE;
					ZmienPozycje(X, Y + 1);
					iTablicaMapy[Y][X] = GRACZ;
				}
			}
			if (ButtonClickOnSystems(up))
			{
				if (CzyMozeSieRuszyc(X, Y, 2))
				{
					SprawdzMape(X, Y - 1, DUCH);
					iTablicaMapy[Y][X] = WOLNE;
					ZmienPozycje(X, Y - 1);
					iTablicaMapy[Y][X] = GRACZ;
				}
			}
			if (ButtonClickOnSystems(left))
			{
				if (CzyMozeSieRuszyc(X, Y, 3))
				{
					SprawdzMape(X - 1, Y, DUCH);
					iTablicaMapy[Y][X] = WOLNE;
					ZmienPozycje(X - 1, Y);
					iTablicaMapy[Y][X] = GRACZ;
				}
			}
			if (ButtonClickOnSystems(right))
			{
				if (CzyMozeSieRuszyc(X, Y, 4))
				{
					SprawdzMape(X + 1, Y, DUCH);
					iTablicaMapy[Y][X] = WOLNE;
					ZmienPozycje(X + 1, Y);
					iTablicaMapy[Y][X] = GRACZ;
				}
			}
			Delta = 0;
		}
	}
};
class Duch : public Player
{
private:
	int WylosowanyRuch;
	int PoprzedniLosDuszka = 0;
	int NarzuconyKierunek = 0;

public:
	Duch(int s = 0)
	{
		LosowaniePozycjiDuszka();
		speed = s;
	}
	void LosowaniePozycjiDuszka()
	{
		int roboczex;
		int roboczey;
		do
		{
			roboczex = rand() % iRozmiarMapyX + 1;
			roboczey = rand() % iRozmiarMapyY + 1;
		} while (iTablicaMapy[roboczey][roboczex] != WOLNE);
		X = roboczex;
		Y = roboczey;
	}
	void LosowanieRuchuDuszka()		// Losuje ruch i gwarantuje, że duszek nie będzie się wracał
	{								// 1 dół, 2 góra, 3 lewo, 4 prawo
		switch (PoprzedniLosDuszka) // Tu jest przekazywany poprzedni ruch duszka
		{
		case 1:
			do
			{
				WylosowanyRuch = 1 + (rand() % 4); // Losowanie ruchu
			} while (PoprzedniLosDuszka == 2);	   // Do momentu, w którym wylosowany kierunek duszka, nie będzie przeciwnym kierunkiem poprzedniego
			break;
		case 2:
			do
			{
				WylosowanyRuch = 1 + (rand() % 4);
			} while (PoprzedniLosDuszka == 1);
			break;
		case 3:
			do
			{
				WylosowanyRuch = 1 + (rand() % 4);
			} while (PoprzedniLosDuszka == 4);
			break;
		case 4:
			do
			{
				WylosowanyRuch = 1 + (rand() % 4);
			} while (PoprzedniLosDuszka == 3);
			break;
		default: // W przypadku braku poprzedniego ruchu, po prostu losuj
			WylosowanyRuch = 1 + (rand() % 4);
			break;
			PoprzedniLosDuszka = WylosowanyRuch; // Ustawia wylosowany ruch jako poprzedni
		}
	}
	bool CzyMozeSieRuszyc(int iPozX, int iPozY, int Kierunek) // Sprawdza czy duch może się ruszyć. Sprawdza czy jest MUR i inny duch obok. Najpierw Y, potem X, bo tak działają tablice
	{
		if (Kierunek == 1)
		{
			if (iTablicaMapy[iPozY + 1][iPozX] != MUR && iTablicaMapy[iPozY + 1][iPozX] != DUCH)
				return true;
			else return false;
		}
		if (Kierunek == 2)
		{
			if (iTablicaMapy[iPozY - 1][iPozX] != MUR && iTablicaMapy[iPozY - 1][iPozX] != DUCH)
				return true;
			else return false;
		}
		if (Kierunek == 3)
		{
			if (iTablicaMapy[iPozY][iPozX - 1] != MUR && iTablicaMapy[iPozY][iPozX - 1] != DUCH)
				return true;
			else return false;
		}
		if (Kierunek == 4)
		{
			if (iTablicaMapy[iPozY][iPozX + 1] != MUR && iTablicaMapy[iPozY][iPozX + 1] != DUCH)
				return true;
			else return false;
		}
		return false;
	}
	void ruch(int Narzutka) // Wywołuj ten ruch, wartość 0, jeśli ma być losowy ruch
	{
		NarzuconyKierunek = Narzutka;
		ruch();
		NarzuconyKierunek = 0;
	}
	void ruch()
	{
		if (buff == true || debuff == true)
		{
			PozostalyCzas = PozostalyCzas - deltaTime; // Odejmij czas co każde wykonanie
		}
		iTablicaMapy[Y][X] = DUCH;
		Delta += currentTime - previousFrame; // Dodaje różnicę czasu między klatkami
		if (speed * 100 < Delta)			  // Jeżeli prędkość * 100 jest mniejsza niż delta
		{
			if (NarzuconyKierunek > 0 && NarzuconyKierunek <= 4) // Jeżeli ruch jest narzucony
			{
				WylosowanyRuch = NarzuconyKierunek; // Ustaw narzucony kierunek
			}
			else if (NarzuconyKierunek == 0) // Jesli nie jest narzucony, losuj
			{
				LosowanieRuchuDuszka();
			}
			switch (WylosowanyRuch) // Wylosowany ruch
			{
			case 1:
				if (CzyMozeSieRuszyc(X, Y, 1))
				{
					SprawdzMape(X, Y + 1, GRACZ);
					iTablicaMapy[Y][X] = WOLNE;
					ZmienPozycje(X, Y + 1);
					iTablicaMapy[Y][X] = DUCH;
				}
				break;
			case 2:
				if (CzyMozeSieRuszyc(X, Y, 2))
				{
					SprawdzMape(X, Y - 1, GRACZ);
					iTablicaMapy[Y][X] = WOLNE;
					ZmienPozycje(X, Y - 1);
					iTablicaMapy[Y][X] = DUCH;
				}
				break;
			case 3:
				if (CzyMozeSieRuszyc(X, Y, 3))
				{
					SprawdzMape(X - 1, Y, GRACZ);
					iTablicaMapy[Y][X] = WOLNE;
					ZmienPozycje(X - 1, Y);
					iTablicaMapy[Y][X] = DUCH;
				}
				break;
			case 4:
				if (CzyMozeSieRuszyc(X, Y, 4))
				{
					SprawdzMape(X + 1, Y, GRACZ);
					iTablicaMapy[Y][X] = WOLNE;
					ZmienPozycje(X + 1, Y);
					iTablicaMapy[Y][X] = DUCH;
				}
				break;
			}
			Delta = 0;
		}
	}
};

class map
{
public:
	int NumerMapy;
	string NazwaMapy;

	void WczytywanieMapyZPliku()
	{
		fstream plik;
		plik.open(NazwaMapy, ios::in);
		if (plik.good() == false)
		{
			cout << "Plik nie istnieje!\n";
			cin.get();
		}
		else
		{
			for (int i = 0; i < iRozmiarMapyY; i++)
			{
				for (int j = 0; j < iRozmiarMapyX; j++)
				{
					plik >> iTablicaMapy[i][j];
				}
			}
		}
		plik.close();
	}
	void ZapisywanieMapyDoPliku()
	{
		fstream plik;
		plik.open(NazwaMapy, ios::out | ios::app);
		if (plik.good() == false)
		{
			cout << "Plik nie istnieje!\n";
			cin.get();
		}
		else
		{
			plik << iRozmiarMapyX << " \n";
			plik << iRozmiarMapyY << " \n";
			for (int i = 0; i < iRozmiarMapyY; i++)
			{
				for (int j = 0; j < iRozmiarMapyX; j++)
				{
					plik << iTablicaMapy[i][j] << " ";
				}
				plik << endl;
			}
		}
		plik.close();
	}
	void LosowaniePozycjiPunktow() // Losuje pozycję punktów, buffów i debuffów na mapie
	{
		srand(time(NULL));
		for (int i = 0; i < iMaxPunktow; i++)
		{
			int roboczex;
			int roboczey;
			do
			{
				roboczex = rand() % iRozmiarMapyX + 1;
				roboczey = rand() % iRozmiarMapyY + 1;
			} while (iTablicaMapy[roboczey][roboczex] != WOLNE);
			iTablicaMapy[roboczey][roboczex] = PunktNaMapie;
		}
		// Buff
		srand(time(NULL));
		for (int i = 1; i <= 3; i++)
		{
			int roboczex;
			int roboczey;
			do
			{
				roboczex = rand() % iRozmiarMapyX + 1;
				roboczey = rand() % iRozmiarMapyY + 1;
			} while (iTablicaMapy[roboczey][roboczex] != WOLNE);
			iTablicaMapy[roboczey][roboczex] = BUFF;
		}
		// DeBuff
		srand(time(NULL));
		for (int i = 1; i <= 3; i++)
		{
			int roboczex;
			int roboczey;
			do
			{
				roboczex = rand() % iRozmiarMapyX + 1;
				roboczey = rand() % iRozmiarMapyY + 1;
			} while (iTablicaMapy[roboczey][roboczex] != WOLNE);
			iTablicaMapy[roboczey][roboczex] = DEBUFF;
		}
	}
};

int ZblizenieDuszka(int DuchX, int DuchY, int GraczX, int GraczY) // Sprawdza czy gracz jest w określonej odległości. Jeśli tak, idzie w jego stronę, jeśli nie, wuyrzuca 0, czyli losuje kierunek
{
	// 1 dół, 2 góra, 3 lewo, 4 prawo
	if (DuchY - GraczY <= 3 && DuchY - GraczY > 0)
	{
		if (DuchX - GraczX <= 3 && DuchX - GraczX > 0)
			return 3;
		else if (DuchX - GraczX >= -3 && DuchX - GraczX < 0)
			return 4;
		else if (DuchX - GraczX == 0)
			return 2;
	}
	else if (DuchY - GraczY == 0)
	{
		if (DuchX - GraczX <= 3 && DuchX - GraczX > 0)
			return 3;
		else if (DuchX - GraczX >= -3 && DuchX - GraczX < 0)
			return 4;
	}
	else if (DuchY - GraczY >= -3 && DuchY - GraczY < 0)
	{
		if (DuchX - GraczX <= 3 && DuchX - GraczX > 0)
			return 3;
		else if (DuchX - GraczX >= -3 && DuchX - GraczX < 0)
			return 4;
		else
			return 1;
	}
	else return 0;
	return 0;
}
int main()
{
	while (!finish)
	{
		#if defined(__LINUX__) || defined(__gnu_linux__) || defined(__linux__) || defined(__APPLE__)
		term_setup(sighandler);
		#endif

		// SetConsoleTitleA("PacMacieson by Maciej Nasiadka");
		bGranie = false;
		std::stringstream TextBuffer;

		map map;
		map.NazwaMapy = "map.txt";
		map.NumerMapy = 1;
		map.WczytywanieMapyZPliku();
		map.LosowaniePozycjiPunktow();
		// map.ZapisywanieMapyDoPliku();

		// Postacie
		Duch duch[iloscDuchow];
		for (int i = 0; i < iloscDuchow; i++)
		{
			duch[i].speed = (rand() % 11) + 5;
			duch[i].LosowaniePozycjiDuszka();
		}

		Player gracz(3, 1, 5);
		currentTime = clock();
		previousFrame = clock();

		// SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);

		cout << "______         ___  ___           _                       \n";
		cout << "| ___ \\        |  \\/  |          (_)                      \n";
		cout << "| |_/ /_ _  ___| .  . | __ _  ___ _  ___  ___  ___  _ __  \n";
		cout << "|  __/ _` |/ __| |\\/| |/ _` |/ __| |/ _ \\/ __|/ _ \\| '_ \\ \n";
		cout << "| | | (_| | (__| |  | | (_| | (__| |  __/\\__ \\ (_) | | | |\n";
		cout << "\\_|  \\__,_|\\___\\_|  |_/\\__,_|\\___|_|\\___||___/\\___/|_| |_|\n";
		cout << "\nWelcome in PacMacieson! Press enter to start the game!\n\n";

		cout << static_cast<char>(GRACZ) << " - Your character\n";
		cout << static_cast<char>(DUCH) << " - Enemy\n";
		cout << static_cast<char>(PunktNaMapie) << " - Point\n";
		cout << static_cast<char>(BUFF) << " - Speed x 5 for 5 secs\n";
		cout << static_cast<char>(DEBUFF) << " - Speed / 5 for 5 secs\n";
		cout << static_cast<char>(MUR) << " - Wall\n";

		cout << "\nEnemies will approach you when you're within 3 tiles\n";
		cout << "Contact with an enemy is deadly\n";
		cout << "You'll get to the next level after collecting 10 points\n";
		cout << "Enemies can collect points and buffs too!\n";
		cout << "Have a nice game! - Maciej Nasiadka";
		cin.get();
		bGranie = true;

		while (bGranie)
		{
			previousFrame = currentTime;
			currentTime = clock();
			deltaTime = currentTime - previousFrame;

			gracz.ruch();

			for (int i = 0; i < 3; i++)
			{
				if (ZblizenieDuszka(duch[i].X, duch[i].Y, gracz.X, gracz.Y) > 0 && ZblizenieDuszka(duch[i].X, duch[i].Y, gracz.X, gracz.Y))
				{
					duch[i].ruch(ZblizenieDuszka(duch[i].X, duch[i].Y, gracz.X, gracz.Y));
				}
				else
				{
					duch[i].ruch(0);
				}
			}
			if (ButtonClickOnSystems(escape))
			{
				bGranie = false;
			}
			Clear();
			TextBuffer.str("");
			for (int i = 0; i < iRozmiarMapyY; i++)
			{
				for (int j = 0; j < iRozmiarMapyX; j++)
				{
					if (gracz.score == 10 & map.NumerMapy == 1)
					{
						map.NazwaMapy = "map2.txt";
						map.NumerMapy = 2;
						map.WczytywanieMapyZPliku();
						map.LosowaniePozycjiPunktow();
						for (int i = 1; i <= iloscDuchow; i++)
						{
							duch[i].LosowaniePozycjiDuszka();
						}
						gracz.ZmienPozycje(47, 22);
						cout << "Nowy poziom!";
						sleep(1000);
					}
					TextBuffer << static_cast<char>(iTablicaMapy[i][j]) << flush;
				}
				if (i < 25)
				{
					TextBuffer << endl
							   << flush;
				}
			}
			cout << TextBuffer.str();
			cout << "Your points: " << gracz.score << " | Ghosts points: ";
			for (int i = 0; i < iloscDuchow; i++)
			{
				cout << duch[i].score << " ";
			}
			cout << "| Lvl: " << map.NumerMapy << "\n";
			cout << "Your buff:   " << gracz.buff << " | Ghosts buff  : ";
			for (int i = 0; i < iloscDuchow; i++)
			{
				cout << duch[i].buff << " ";
			}
			cout << "\nYour X:Y " << gracz.X << ":" << gracz.Y << " | Ghosts X:Y ";
			for (int i = 0; i < iloscDuchow; i++)
			{
				cout << duch[i].X << ":" << duch[i].Y << " ";
			}
			cout << "\nYour speed: " << gracz.speed << " | Ghosts speed: ";
			for (int i = 0; i < iloscDuchow; i++)
			{
				cout << duch[i].speed << " ";
			}
			cout << "\n";
			sleep(35);
		}
		Clear();
		cout << " _____                        _____                 \n";
		cout << "|  __ \\                      |  _  |                \n";
		cout << "| |  \\/ __ _ _ __ ___   ___  | | | |_   _____ _ __  \n";
		cout << "| | __ / _` | '_ ` _ \\ / _ \\ | | | \\ \\ / / _ \\ '__| \n";
		cout << "| |_\\ \\ (_| | | | | | |  __/ \\ \\_/ /\\ V /  __/ |    \n";
		cout << " \\____/\\__,_|_| |_| |_|\\___|  \\___/  \\_/ \\___|_|   \n";
		cout << "\n";
		cout << "---------------------------------------------------------\n";

		cout << "Game over!\nYour points: " << gracz.score << " | Ghosts points: ";
		for (int i = 0; i < iloscDuchow; i++)
		{
			cout << duch[i].score << " ";
		}
		cout << " Lvl: " << map.NumerMapy << "\nPress enter to play again!\n";
		cin.get();
		Clear();
		main();
	}
#if defined(__LINUX__) || defined(__gnu_linux__) || defined(__linux__) || defined(__APPLE__)
	term_restore();
#endif
	return 0;
}