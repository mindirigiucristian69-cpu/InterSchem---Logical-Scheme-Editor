#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <string>
#include <cmath>
#include <climits>
#include <sstream>
#include "graphics.h"
#include "winbgim.h"
#include <windows.h>  // pt tipurile de date Windows
#include <commdlg.h> // pt open file
#pragma comment(lib, "Comdlg32.lib") // biblioteca de dialoguri or wtv that means
using namespace std;

#define NMAX 100005
#define PI 3.1415926
#define INF INT_MAX
#define EPSILON 0.0001

// VARIABILE GLOBALE PENTRU SETARI
int culoareContur = COLOR(50, 50, 50); // Culoarea implicita (gri inchis)
int delayExecutie = 400;               // Delay implicit
bool esteSetariDeschis = false;        // Starea meniului pop-up

enum TipBloc
{
	TIP_START = 0,
	TIP_ATRIBUIRE,
	TIP_CITIRE,
	TIP_DECIZIE,
	TIP_AFISARE,
	TIP_STOP
};

struct Bloc
{
	int x, y;
	int tip;
	char text[50];
};

struct Legatura
{
	int sursaIdx;
	int destIdx;
};

struct blocLogic
{
	int id = 0;	 // id unic
	int tip = 0; // mai jos
	/**
	0 stop
	1 start
	2 cin
	3 cout
	4 atribuire
	5 if
	**/
	double valoare = 0.0;	 // val pt cin/rezultatul if ului
	string expresie = ""; // expresia din cin/if
	char var = '\0';		 // variabila unde retinem rezultatul
	blocLogic* urm = NULL;	 // pt rulare din consola
	blocLogic* urmA = NULL; // pt if daca e adv expresia
	blocLogic* urmF = NULL; // pt if daca e falsa expresia
	int guiIndex = 0;	 // pentru culoarea verde
};

vector<Bloc> schema;
vector<Legatura> legaturi;
vector<string> outputConsole;
vector<string> cppCodeLines; // salvam liniile de cod undeva pentru afisare
int scrollPosCod = 0;        // pozitia de scroll pentru codificare in c++

bool esteMeniulDeschis = false;
int indexBlocSelectat = -1;
int indexSursaLegatura = -1;
int offsetX, offsetY;
int blocCurentExecutie = -1;

double variabila[26]; // A-Z
stack<char> operatori;
stack<double> operanzi;

/**
+ - * ^ < >
+ adunare
- scadere
* inmultire
^ putere
< mai mic
> mai mare
/ div
= egalitate ==
# diferit !=
$ direct asta: <=
@ direct asta: >=
% modulo
s sin
c cos
a modul
r radical
e exponentiala
l log
~ minus unar
**/
char operatiiBinare[] = "+-*/^<>=#$@%";
char operatiiUnare[] = "scarel~";
char operatii[] = "+-*/^<>=#scarel~$@%";

// declarari anticipate!!
void DeseneazaInterfata();
void AdaugaLog(string text);

//////////////////////////////////////////////////////////////////////////////////// eval expresiei

int prioritate(char ch) // prioritatea operatiilor
{
	if (ch == '=' || ch == '#' || ch == '<' || ch == '>' || ch == '$' || ch == '@')
		return 0;
	if (ch == '(' || ch == ')')
		return 1;
	if (ch == '+' || ch == '-')
		return 2;
	if (ch == '*' || ch == '/' || ch == '%')
		return 3;
	if (ch == '^')
		return 4;
	if (ch == 'c' || ch == 's' || ch == 'l' || ch == 'e' || ch == 't' || ch == 'a' || ch == 'r' || ch == '~')
		return 5;
	return -1;
}

bool difInf(double x)
{
	return fabs(INF - fabs(x)) > INF / 2.0;
}
double logaritm(double x)
{
	if (x < 0)
		return -INF;

	if (x > EPSILON && difInf(x))
		return log(x);

	return INF;
}
double exponential(double x)
{
	if (difInf(x))
		return exp(x);

	return INF;
}
double inmultire(double x, double y)
{
	if (fabs(x) < EPSILON || fabs(y) < EPSILON)
		return 0;

	if (difInf(x) && difInf(y))
		return x * y;

	return INF;
}
double putere(double x, double y)
{
	if (x == 0)
		return 0;

	if (y == 0)
		return 1;

	if (x == INF || y == INF)
		return INF;

	return pow(x, y);
}
double egalitate(double x, double y)
{
	return (fabs(x - y) < EPSILON);
}
double maiMic(double x, double y)
{
	return x < y;
}
double maiMare(double x, double y)
{
	return x > y;
}
double maiMicEgal(double x, double y)
{
	return x <= y + EPSILON; // folosim epsilon pentru float comparison
}
double maiMareEgal(double x, double y)
{
	return x >= y - EPSILON;
}
double adunare(double x, double y)
{
	if (difInf(x) && difInf(y))
		return x + y;

	return INF;
}
double scadere(double x, double y)
{
	if (difInf(x) && difInf(y))
		return x - y;

	return INF;
}
double impartire(double x, double y)
{
	if (fabs(y) > EPSILON)
		return x / y;

	return INF;
}
double sinus(double x)
{
	if (difInf(x))
	{
		double radiani = x * PI / 180.0;
		double rez = sin(radiani);
		return (fabs(rez) < EPSILON) ? 0 : rez;
	}
	return INF;
}
double cosinus(double x)
{
	if (difInf(x))
	{
		double radiani = x * PI / 180.0;
		double rez = cos(radiani);
		return (fabs(rez) < EPSILON) ? 0 : rez;
	}
	return INF;
}
double modul(double x)
{
	if (difInf(x))
		return fabs(x);
	else
		return INF;
}
double radical(double x)
{
	if (x < 0)
		return -INF;
	else if (difInf(x) && (x > -EPSILON))
		return sqrt(fabs(x));
	else
		return INF;
}
double rest(double x, double y)
{
	if (fabs(y) < EPSILON)
		return 0; // evitam impartirea la 0

	if (difInf(x) && difInf(y))
		return fmod(x, y);

	return INF;
}

bool esteNumar(string sir)
{
	if (sir.empty())
		return false;
	// e nr daca primul char e cifra
	if (isdigit(sir[0]))
		return true;
	// sau daca e negative avem un - si urmeaza o cifra
	if (sir[0] == '-' && sir.length() > 1 && isdigit(sir[1]))
		return true;
	return false;
}

struct functie
{
	string expresie = ""; // expresia introdusa
	vector<string> v = vector <string>(); // tokenizare, adc impart stringul pe operatii/nr
	bool valid = 1; // expresia corecta
};

bool expresieCorecta(string expresie)
{
	if (expresie.length() == 0)
		return 0; // nu acceptam gol

	// parantezare corecta
	int paranteze = 0;
	for (char c : expresie)
	{
		if (c == '(')
			paranteze++;
		if (c == ')')
			paranteze--;
		if (paranteze < 0)
			return 0;
	}
	if (paranteze != 0)
		return 0;

	/**
	expresii gresite:
	( cu +*^/<>=#
	+*^/<>=#scarel cu )
	++ sau ** sau ## sau ^^

	**/
	int i, n;
	i = 0;
	n = expresie.length();

	while (i < n - 1)
	{
		if (expresie[i] == '(' && strchr(operatiiBinare, expresie[i + 1]))
			return 0;

		if (strchr(operatii, expresie[i]) && expresie[i + 1] == ')')
			return 0;

		// buna siua am modificat aici ca sa mearga si pt >= si <= wowwwww, also si la buildfunction acelasi lucru kinda
		bool esteCompus = ((expresie[i] == '<' || expresie[i] == '>') && expresie[i + 1] == '=');

		if (!esteCompus && strchr(operatiiBinare, expresie[i]) && strchr(operatiiBinare, expresie[i + 1]))
			return 0;

		// daca e compus sarim peste urmatorul caracter ca sa nu il verifice iar
		if (esteCompus)
			i++;

		i++;
	}

	return 1;
}

// si asta am facut ca era acelasi cod de so multe ori si mna
void proceseazaOperatie()
{
	if (operatori.empty() || operanzi.empty())
		return; // sa fie safe

	char op = operatori.top();
	operatori.pop();
	double x2 = operanzi.top();
	operanzi.pop();
	double x1 = 0;

	if (strchr(operatiiBinare, op))
	{
		if (operanzi.empty())
			return; // sa fie safe :))

		x1 = operanzi.top();
		operanzi.pop();
	}

	double val = 0;
	// calculez
	switch (op)
	{
	case '=':
		val = egalitate(x1, x2);
		break;
	case '#':
		val = !egalitate(x1, x2);
		break;
	case '<':
		val = maiMic(x1, x2);
		break;
	case '>':
		val = maiMare(x1, x2);
		break;
	case '$': // simbolul pt <=
		val = maiMicEgal(x1, x2);
		break;
	case '@': // simbolul pt >=
		val = maiMareEgal(x1, x2);
		break;
	case '%':
		val = rest(x1, x2);
		break;
	case '+':
		val = adunare(x1, x2);
		break;
	case '-':
		val = scadere(x1, x2);
		break;
	case '*':
		val = inmultire(x1, x2);
		break;
	case '/':
		val = impartire(x1, x2);
		break;
	case '^':
		val = putere(x1, x2);
		break;
	case 's':
		val = sinus(x2);
		break;
	case 'c':
		val = cosinus(x2);
		break;
	case 'l':
		val = logaritm(x2);
		break;
	case 'e':
		val = exponential(x2);
		break;
	case 'a':
		val = modul(x2);
		break;
	case 'r':
		val = radical(x2);
		break;
	case '~':
		val = -x2;
		break;
	}
	operanzi.push(val);
}

double valoareFunctie(functie E)
{
	// resetez stivele
	while (!operanzi.empty())
		operanzi.pop();

	while (!operatori.empty())
		operatori.pop();

	bool waitForSign;
	waitForSign = 1;

	for (int i = 0; i < (int)E.v.size(); i++)
	{
		string token = E.v[i];
		if (esteNumar(token))
		{
			operanzi.push(atof(token.c_str()));
			waitForSign = 0;
		}
		else
		{
			if (token.length() == 1 && token[0] >= 'A' && token[0] <= 'Z')
			{
				operanzi.push(variabila[token[0] - 'A']);
				waitForSign = 0;
			}
			else if (token == "q")
			{
				operanzi.push(PI);
				waitForSign = 0;
			}
			else if (token == "(")
			{
				operatori.push('(');
				waitForSign = 1;
			}
			else if (token == ")")
			{
				// procesez tot pana la '('
				while (!operatori.empty() && operatori.top() != '(')
					proceseazaOperatie();

				if (!operatori.empty())
					operatori.pop(); // pop la '('

				waitForSign = 0;
			}
			else
			{
				char opCurent = token[0];

				if (opCurent == '-' && waitForSign == true)
					opCurent = '~';

				// e un operator + - * etc, deci procesez cei cu prioritate mai mare primiiwhile (!operatori.empty() && operatori.top() != '(')
				while (!operatori.empty() && operatori.top() != '(')
				{
					bool trebuieProcesat = false;

					if (prioritate(opCurent) == 5)
					{
						if (prioritate(operatori.top()) > prioritate(opCurent))
							trebuieProcesat = true;
					}
					else
					{
						if (prioritate(operatori.top()) >= prioritate(opCurent))
							trebuieProcesat = true;
					}

					if (trebuieProcesat)
						proceseazaOperatie();
					else
						break;
				}

				operatori.push(opCurent);

				waitForSign = 1;
			}
		}
	}
	// dam clean up
	while (!operatori.empty())
		proceseazaOperatie();
	if (!operanzi.empty())
		return operanzi.top();

	return 0;
}

functie buildFunction(string expresie)
{
	functie F;
	string clean = "";
	for (char c : expresie)
		if (c != ' ')
			clean += c;

	F.expresie = clean;
	F.valid = true;
	int i = 0, n = clean.length();
	bool areParanteze = (n > 0 && clean.front() == '(' && clean.back() == ')');

	if (!areParanteze)
		F.v.push_back("(");

	while (i < n)
	{
		if (isdigit(clean[i]) || clean[i] == '.')
		{
			string nr = "";
			while (i < n && (isdigit(clean[i]) || clean[i] == '.'))
			{
				nr += clean[i];
				i++;
			}
			F.v.push_back(nr);
		}
		else
		{
			// buna siua iar, si aici, lfl ca la expresiecorecta
			if (i < n - 1 && clean[i] == '<' && clean[i + 1] == '=')
			{
				F.v.push_back("$"); // $ inseamna <=
				i += 2;
			}
			else if (i < n - 1 && clean[i] == '>' && clean[i + 1] == '=')
			{
				F.v.push_back("@"); // % inseamna >=
				i += 2;
			}
			else
			{
				// codul vechi pentru caractere simple
				string s(1, clean[i]);
				if (strchr(operatii, clean[i]) || (clean[i] >= 'A' && clean[i] <= 'Z') || clean[i] == '(' || clean[i] == ')')
					F.v.push_back(s);

				i++;
			}
		}
	}
	if (!areParanteze)
		F.v.push_back(")");
	return F;
}

void evalueaza(string expresie, double& valoare)
{
	functie F = buildFunction(expresie);
	valoare = valoareFunctie(F);
}

/////////////////////////////////////////////////////////////////////////////////////// front end

// afisare consola
void AdaugaLog(string text)
{
	outputConsole.push_back(text); //vector care stocheaza liniile de text afisate
	if (outputConsole.size() > 14) //limita de 14 linii
		outputConsole.erase(outputConsole.begin());  //daca trecem limita stergem prima linie ca sa adaugam una noua in jos

	//redesenam
	int p = getactivepage();
	setactivepage(1 - getvisualpage());
	DeseneazaInterfata();
	setvisualpage(1 - getvisualpage());
	setactivepage(p);
}

// pentru citire cin>>x
double GuiInput(char varName)
{
	string msg = "INTRODU "; //mesaj de intampinare
	msg += varName;
	msg += ": ";
	AdaugaLog(msg);
	string inputBuffer = "";
	while (true) //programul este captiv aici pana apasam ENTER
	{
		if (kbhit())
		{
			char c = getch(); //stocam tasta pe care am apasat o
			if (c == 13) //daca apasam ENTER ne oprim
			{
				AdaugaLog(" > " + inputBuffer);
				break;
			}
			else if (c == 8) //cu BACKSPACE stergem
			{
				if (inputBuffer.length() > 0)
					inputBuffer.pop_back();
			}
			else if ((c >= '0' && c <= '9') || c == '.' || c == '-') //punem acolo caracterul
			{
				inputBuffer += c;
			}

			if (!outputConsole.empty() && outputConsole.back().substr(0, 3) == " > ")
				outputConsole.pop_back(); //curatarea liniei precedente daca exista
			outputConsole.push_back(" > " + inputBuffer); //adaugarea starii curente
			//redesenam
			int p = getactivepage();
			setactivepage(1 - getvisualpage());
			DeseneazaInterfata();
			setvisualpage(1 - getvisualpage());
			setactivepage(p);
			outputConsole.pop_back(); //curatam de pe consola
		}
	}
	return inputBuffer.empty() ? 0 : atof(inputBuffer.c_str()); //transformam sirul de caractere intr un numar real double
}

// executam program
void ExecutaPasCuPas(blocLogic* B)
{
	if (B == nullptr)
		return; //daca pointerul e nul ne oprim sa nu dea crash
	blocCurentExecutie = B->guiIndex; //trebuie colorat in verde
	//redesenam ca sa vada utilizatorul blocul verde
	int p = getactivepage();
	setactivepage(1 - getvisualpage());
	DeseneazaInterfata();
	setvisualpage(1 - getvisualpage());
	setactivepage(p);

	// AICI AM MODIFICAT SA FOLOSEASCA VARIABILA DE DELAY
	delay(delayExecutie);

	if (B->tip == 0)
	{ // STOP
		AdaugaLog("[STOP]");
		blocCurentExecutie = -1;
		return;
	}
	else if (B->tip == 1)
	{ // START
		AdaugaLog("[START]");
		ExecutaPasCuPas(B->urm);
	}
	else if (B->tip == 2)
	{
		B->valoare = GuiInput(B->var);
		variabila[B->var - 'A'] = B->valoare; //stocam variabila
		ExecutaPasCuPas(B->urm);
	}
	else if (B->tip == 3)
	{ // AFISARE
		double val;
		if (B->expresie.length() > 0)
			evalueaza(B->expresie, val); //evaluam expresia
		else
		{
			string tempVar(1, B->var);
			evalueaza(tempVar, val);
		}
		char buffer[64];
		sprintf_s(buffer, "OUT: %.2f", val);
		AdaugaLog(buffer); //il afisam in consola
		ExecutaPasCuPas(B->urm);
	}
	else if (B->tip == 4)
	{ // ATRIBUIRE
		evalueaza(B->expresie, variabila[B->var - 'A']);
		B->valoare = variabila[B->var - 'A'];
		char buffer[64];
		sprintf_s(buffer, "%c <- %.2f", B->var, B->valoare);
		AdaugaLog(buffer);
		ExecutaPasCuPas(B->urm);
	}
	else if (B->tip == 5)
	{ // DECIZIE
		evalueaza(B->expresie, B->valoare);
		if (B->valoare != 0)
		{
			AdaugaLog("TRUE");
			ExecutaPasCuPas(B->urmA);
		}
		else
		{
			AdaugaLog("FALSE");
			ExecutaPasCuPas(B->urmF);
		}
	}
}

// convertim program si apoi executam prin functia de deasupra
void ConvertesteSiRuleaza()
{
	if (schema.empty())
	{
		AdaugaLog("SCHEMA GOALA!"); // in caz ca rulam o schema goala
		return;
	}
	outputConsole.clear(); //sterge textu vechi din consola
	AdaugaLog("Initializare...");

	vector<blocLogic*> noduri(schema.size()); //blocurile vizuale
	blocLogic* startNode = nullptr; //blocurile logice

	for (size_t i = 0; i < schema.size(); i++)
	{
		noduri[i] = new blocLogic; //punem  blocul in memoria dinamica
		noduri[i]->id = i;
		noduri[i]->guiIndex = i; //ca sa vedem cand rulam logic ca sa stim daca e verde
		noduri[i]->urm = noduri[i]->urmA = noduri[i]->urmF = nullptr;
		string textRaw = schema[i].text;

		if (schema[i].tip == TIP_ATRIBUIRE || schema[i].tip == TIP_DECIZIE || schema[i].tip == TIP_AFISARE)
		{
			string deVerificat = textRaw;

			if (schema[i].tip == TIP_ATRIBUIRE)
			{
				size_t pos = textRaw.find('='); //extragere expresie fara "A="
				if (pos != string::npos)
					deVerificat = textRaw.substr(pos + 1);
			}

			if (!expresieCorecta(deVerificat)) //daca nu e corecta expresia abortam
			{
				char err[64];
				sprintf_s(err, "ERR: Expresie gresita la blocul %d!", i);
				AdaugaLog(err);
				for (int j = 0; j <= (int)i; j++)
					delete noduri[j];
				return;
			}
		}
		switch (schema[i].tip) //aici transformam textul in instructiuni
		{
		case TIP_START:
			noduri[i]->tip = 1;
			startNode = noduri[i];
			break;
		case TIP_STOP:
			noduri[i]->tip = 0;
			break;
		case TIP_CITIRE:
			noduri[i]->tip = 2;
			if (textRaw.size() > 0)
				noduri[i]->var = textRaw[0];
			break;
		case TIP_AFISARE:
			noduri[i]->tip = 3;
			if (textRaw.size() == 1 && isalpha(textRaw[0]))
			{
				noduri[i]->var = textRaw[0];
				noduri[i]->expresie = textRaw;
			}
			else
				noduri[i]->expresie = textRaw;
			break;
		case TIP_ATRIBUIRE:
			noduri[i]->tip = 4;
			{
				size_t pos = textRaw.find('=');
				if (pos != string::npos)
				{
					noduri[i]->var = textRaw[0];
					noduri[i]->expresie = textRaw.substr(pos + 1);
				}
			}
			break;
		case TIP_DECIZIE:
			noduri[i]->tip = 5;
			noduri[i]->expresie = textRaw;
			break;
		}
	}

	if (!startNode) //daca n avem start spunem lipsa start
	{
		AdaugaLog("LIPSA START!");
		return;
	}

	for (const auto& leg : legaturi) //aici construim graful
	{
		if (leg.sursaIdx >= (int)schema.size() || leg.destIdx >= (int)schema.size())
			continue;
		blocLogic* src = noduri[leg.sursaIdx];
		blocLogic* dst = noduri[leg.destIdx];
		if (src->tip == 5) //daca e bloc de decizie
		{
			if (src->urmA == nullptr)
				src->urmA = dst; //prima sageata e adevarat
			else
				src->urmF = dst; //a doua sageata e falsa
		}
		else
		{
			src->urm = dst; //daca nu e if e o legatura normala
		}
	}

	memset(variabila, 0, sizeof(variabila)); //punem zero la toate variabilele
	ExecutaPasCuPas(startNode); //dam drumul la executie
	blocCurentExecutie = -1;
	for (auto* n : noduri)
		delete n; //stergem nodurile din ram
}

// stergemblocul si legaturile
void StergeBlocSiLegaturi(int indexDeSters)
{
	for (int i = legaturi.size() - 1; i >= 0; i--) //stergem legaturile adiacente
	{
		if (legaturi[i].sursaIdx == indexDeSters || legaturi[i].destIdx == indexDeSters)
		{
			legaturi.erase(legaturi.begin() + i);
		}
	}
	schema.erase(schema.begin() + indexDeSters); //stergem blocul
	for (size_t i = 0; i < legaturi.size(); i++) //reindexarea legaturilor
	{
		if (legaturi[i].sursaIdx > indexDeSters)
			legaturi[i].sursaIdx--;
		if (legaturi[i].destIdx > indexDeSters)
			legaturi[i].destIdx--;
	}
}

// ca sa stim latimea blocului
int getLatimeBloc(int idx) {
	int scale = 10;
	int lat = 40 + scale;
	if (strlen(schema[idx].text) > 0) {
		int tw = textwidth(schema[idx].text); //masuram cat de mare e textul din interiorul blocului
		if (tw / 2 + 15 > lat) lat = tw / 2 + 15; //impartim lunimea textului la 2 pentru ca acestea sunt desenate pornind din centru
	}
	return lat;
}
//in caz ca un bloc este in calea unei linii
bool esteInCaleaOrizontala(int x1, int x2, int y) {
	if (x1 > x2) swap(x1, x2);
	for (int i = 0; i < (int)schema.size(); i++) { //scanam fiecare bloc
		int w = getLatimeBloc(i); //latimea blocului
		if (y > schema[i].y - 35 && y < schema[i].y + 35) { //verificam daca linia trece prin bloc
			if (!(x2 < schema[i].x - w || x1 > schema[i].x + w)) return true;
		}
	}
	return false;
}

// ca sa vedem daca clickul este pe o legatura sa o stergem
bool AproapeDeSegment(int mx, int my, int x1, int y1, int x2, int y2) {
	int tol = 5; //raza acceptata ca doar nu putem da click fix pe un pixel
	if (x1 == x2) { // vertical
		if (abs(mx - x1) <= tol && my >= min(y1, y2) - tol && my <= max(y1, y2) + tol) return true;
	}
	else if (y1 == y2) { // orizontal
		if (abs(my - y1) <= tol && mx >= min(x1, x2) - tol && mx <= max(x1, x2) + tol) return true;
	}
	return false;
}

// desenam sagetile
void DeseneazaSageata(int startX, int startY, int endX, int endY, int sIdx, int dIdx)
{
	setcolor(BLACK);
	setlinestyle(SOLID_LINE, 0, 2);

	// verificare de siguranta sa nu desenam in afara ecranului (evita crash)
	if (startX < -100 || startX > 2000 || endX < -100 || endX > 2000) return;

	int finalDestY = endY - 25;

	//cazum 1: in caz ca merge in sus pentru while sau alte chestii
	if (finalDestY < startY + 10)
	{
		int latS = getLatimeBloc(sIdx);
		// iesim prin stanga
		int exitX = startX - (latS + 30);

		line(startX, startY, exitX, startY);
		line(exitX, startY, exitX, finalDestY - 20);
		line(exitX, finalDestY - 20, endX, finalDestY - 20);
		line(endX, finalDestY - 20, endX, finalDestY);
	}
	// cazul 2: linie normala in jos
	else
	{
		int midY = (startY + finalDestY) / 2;
		//coboram pana la jumatatea dinstantei, facem stanga/dreapta apoi coboram iar

		if (esteInCaleaOrizontala(startX, endX, midY)) { //in caz ca este un bloc care s ar putea intersecta cu sageata
			midY += 40;
		}

		line(startX, startY, startX, midY); //desenarea in trei pasi
		line(startX, midY, endX, midY);
		line(endX, midY, endX, finalDestY);
	}

	// desenam varful sagetii
	line(endX, finalDestY, endX - 5, finalDestY - 10);
	line(endX, finalDestY, endX + 5, finalDestY - 10);
	line(endX - 5, finalDestY - 10, endX + 5, finalDestY - 10);
}

//    desenam forma!
void DeseneazaForma(int x, int y, int tip, const char* textOverride = NULL, bool esteInMeniu = false, bool evidentiat = false, bool executie = false)
{
	if (executie)
		setcolor(GREEN); //daca e in procesul de executie
	else if (evidentiat)
		setcolor(BLUE); //daca am dat click dreapta
	else
		setcolor(RED); //normal

	setlinestyle(SOLID_LINE, 0, 3);
	int scale = esteInMeniu ? 0 : 10;

	// CALCULARE DINAMICA LATIME
	int latimeDinamica = 40 + scale;
	if (!esteInMeniu && textOverride != NULL && strlen(textOverride) > 0) {
		int txtW = textwidth((char*)textOverride);
		if (txtW / 2 + 15 > latimeDinamica)
			latimeDinamica = txtW / 2 + 15;
	}

	switch (tip)
	{
	case TIP_START:
	case TIP_STOP:
		ellipse(x, y, 0, 360, latimeDinamica, 15 + scale / 2);
		break;
	case TIP_ATRIBUIRE:
		rectangle(x - (latimeDinamica), y - (12 + scale / 2), x + (latimeDinamica), y + (12 + scale / 2));
		break;
	case TIP_CITIRE:
		line(x - (latimeDinamica - 10), y - (12 + scale / 2), x + (latimeDinamica - 10), y - (12 + scale / 2));
		line(x - (latimeDinamica), y + (12 + scale / 2), x + (latimeDinamica), y + (12 + scale / 2));
		line(x - (latimeDinamica - 10), y - (12 + scale / 2), x - (latimeDinamica), y + (12 + scale / 2));
		line(x + (latimeDinamica - 10), y - (12 + scale / 2), x + (latimeDinamica), y + (12 + scale / 2));
		break;
	case TIP_DECIZIE:
		line(x, y - (17 + scale), x + (latimeDinamica), y);
		line(x + (latimeDinamica), y, x, y + (17 + scale));
		line(x, y + (17 + scale), x - (latimeDinamica), y);
		line(x - (latimeDinamica), y, x, y - (17 + scale));
		break;
	case TIP_AFISARE:
		line(x - (latimeDinamica), y - (12 + scale / 2), x + (latimeDinamica), y - (12 + scale / 2));
		line(x - (latimeDinamica - 10), y + (12 + scale / 2), x + (latimeDinamica - 10), y + (12 + scale / 2));
		line(x - (latimeDinamica), y - (12 + scale / 2), x - (latimeDinamica - 10), y + (12 + scale / 2));
		line(x + (latimeDinamica), y - (12 + scale / 2), x + (latimeDinamica - 10), y + (12 + scale / 2));
		break;
	}

	if (!esteInMeniu)
	{
		settextjustify(CENTER_TEXT, CENTER_TEXT);
		setbkcolor(COLOR(230, 230, 220));
		setcolor(BLACK);
		if (textOverride != NULL && strlen(textOverride) > 0)
		{
			outtextxy(x, y, (char*)textOverride);
		}
		else
		{
			char label[20] = "";
			switch (tip)
			{
			case TIP_START:
				strcpy_s(label, "START");
				break;
			case TIP_ATRIBUIRE:
				strcpy_s(label, "X=...");
				break;
			case TIP_CITIRE:
				strcpy_s(label, "CITIRE");
				break;
			case TIP_DECIZIE:
				strcpy_s(label, "?");
				break;
			case TIP_AFISARE:
				strcpy_s(label, "SCRIE");
				break;
			case TIP_STOP:
				strcpy_s(label, "STOP");
				break;
			}
			outtextxy(x, y, label);
		}
		settextjustify(LEFT_TEXT, TOP_TEXT);
	}
}

// desenam interfata wowowo
void DeseneazaInterfata()
{
	setbkcolor(COLOR(230, 230, 220)); //setam bej deschis
	cleardevice(); //incepem cu o hartie noua

	for (size_t i = 0; i < legaturi.size(); i++) //pentru desenarea legaturilor
	{
		if (legaturi[i].sursaIdx < (int)schema.size() && legaturi[i].destIdx < (int)schema.size())
		{
			Bloc& src = schema[legaturi[i].sursaIdx];
			Bloc& dst = schema[legaturi[i].destIdx];

			int sx, sy;

			int latimeSursa = getLatimeBloc(legaturi[i].sursaIdx);

			if (src.tip == TIP_DECIZIE) //trebuie sa vedem ce legatura desenam din stanga sau din dreapta 
			{
				int nrLegaturiAnterioare = 0;
				for (int j = 0; j < (int)i; j++)
				{
					if (legaturi[j].sursaIdx == legaturi[i].sursaIdx)
					{
						nrLegaturiAnterioare++;
					}
				}

				if (nrLegaturiAnterioare == 0)  //daca e prima legatura, incepem din stanga
				{
					sx = src.x - latimeSursa;
					sy = src.y;
				}
				else //daca e a doua, din dreapta
				{
					sx = src.x + latimeSursa;
					sy = src.y;
				}
			}
			else //pentru oricare alt bloc
			{
				sx = src.x;
				sy = src.y + 25;
			}

			DeseneazaSageata(sx, sy, dst.x, dst.y, legaturi[i].sursaIdx, legaturi[i].destIdx); //functie de desenare sageata

			if (src.tip == TIP_DECIZIE) //ca sa desenam T ul si F ul
			{
				setbkcolor(COLOR(230, 230, 220));
				setcolor(BLACK);
				settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 1);
				int nrLegaturiAnterioare = 0;
				for (int j = 0; j < (int)i; j++)
					if (legaturi[j].sursaIdx == legaturi[i].sursaIdx)
						nrLegaturiAnterioare++;
				if (nrLegaturiAnterioare == 0)
					outtextxy(src.x - (latimeSursa + 10), src.y - 10, "T");
				else
					outtextxy(src.x + (latimeSursa + 10), src.y - 10, "F");
			}
		}
	}

	// desenare meniu stanga
	//un dreptunghi rosu cu un chenar peste el si scriem toate alea 
	settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 2);
	setfillstyle(SOLID_FILL, RED);
	bar(0, 0, 300, 190);
	setcolor(culoareContur); // culoare variabila
	setlinestyle(SOLID_LINE, 0, 10);
	rectangle(0, 0, 300, 190);

	setcolor(COLOR(230, 230, 220));
	setbkcolor(RED);
	outtextxy(10, 10, "INCARCA SCHEMA");
	setcolor(culoareContur);
	line(0, 40, 300, 40);
	setcolor(COLOR(230, 230, 220));
	outtextxy(10, 50, "SALVARE SCHEMA");
	setcolor(culoareContur);
	line(0, 90, 300, 90);
	setcolor(COLOR(230, 230, 220));
	outtextxy(10, 100, "SCHEMA NOUA");
	setcolor(culoareContur);
	line(0, 140, 300, 140);
	setcolor(COLOR(230, 230, 220));
	outtextxy(10, 150, "INTRODUCE BLOC");
	setcolor(culoareContur);
	line(0, 190, 300, 190);
	line(250, 165, 280, 165);
	line(265, 150, 265, 180);

	// BUTONU DE MENIU
	setfillstyle(SOLID_FILL, RED);
	bar(300, 0, 1000, 50);
	setcolor(culoareContur);
	rectangle(300, 0, 1000, 50);
	setcolor(COLOR(230, 230, 220));
	setbkcolor(RED);
	settextjustify(CENTER_TEXT, CENTER_TEXT);
	outtextxy(650, 25, "MENIU / SETARI");
	settextjustify(LEFT_TEXT, TOP_TEXT);

	// desenare panouri dreapta pentru output si cod c++
	setfillstyle(SOLID_FILL, RED);
	bar(1000, 0, 1490, 370);
	setcolor(culoareContur);
	rectangle(1000, 0, 1490, 370);
	rectangle(1000, 0, 1250, 50);
	setcolor(COLOR(230, 230, 220));
	setbkcolor(RED);
	outtextxy(1015, 12, "EXECUTA COD");

	// desenare consola
	setfillstyle(SOLID_FILL, BLACK);
	bar(1005, 55, 1485, 365);
	setcolor(GREEN);
	setbkcolor(BLACK);
	settextstyle(COMPLEX_FONT, HORIZ_DIR, 1);
	int consoleY = 60;
	for (const string& linie : outputConsole) //asta e ca sa scriem cu verde mesajele din consola
	{
		outtextxy(1010, consoleY, (char*)linie.c_str());
		consoleY += 20;
	}

	setfillstyle(SOLID_FILL, RED);
	bar(1000, 380, 1490, 750);
	setcolor(culoareContur);
	rectangle(1000, 380, 1490, 750);
	rectangle(1000, 380, 1310, 430);
	setcolor(COLOR(230, 230, 220));
	setbkcolor(RED);
	settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 2);
	outtextxy(1015, 392, "CODIFICARE IN C++");

	setfillstyle(SOLID_FILL, BLACK);
	bar(1005, 435, 1485, 745);
	setcolor(GREEN);
	setbkcolor(BLACK);
	settextstyle(COMPLEX_FONT, HORIZ_DIR, 1);
	int cppY = 440;
	for (size_t i = scrollPosCod; i < cppCodeLines.size(); i++) //asta e ca sa putem da scroll
	{
		if (cppY < 730)
		{
			outtextxy(1010, cppY, (char*)cppCodeLines[i].c_str());
			cppY += 20;
		}
	}

	if (esteMeniulDeschis)//daca meniul cu blocuri e deschis
	{
		setfillstyle(SOLID_FILL, RED);
		bar(0, 195, 300, 565);
		setcolor(culoareContur);
		setlinestyle(SOLID_LINE, 0, 10);
		rectangle(0, 195, 300, 565);
		setbkcolor(RED);
		int yStart = 205;
		int hItem = 60;
		setlinestyle(SOLID_LINE, 0, 3);
		char labels[6][15] = { "START", "ATRIBUIRE", "CITIRE", "DECIZIE", "AFISARE", "STOP" };
		for (int i = 0; i < 6; i++)
		{
			setcolor(COLOR(230, 230, 220));
			outtextxy(10, yStart + i * hItem, labels[i]); //desenam fiecare nume de bloc
			DeseneazaForma(240, yStart + i * hItem + 12, i, NULL, true);
			if (i < 5) //tragem linie intre ele
			{
				setcolor(culoareContur);
				line(0, yStart + i * hItem + 35, 300, yStart + i * hItem + 35);
			}
		}
	}
	else
	{
		// tabelui  de variabile
		setfillstyle(SOLID_FILL, RED);
		bar(0, 195, 300, 750);
		setcolor(culoareContur);
		setlinestyle(SOLID_LINE, 0, 10);
		rectangle(0, 195, 300, 750);

		setbkcolor(RED);
		setcolor(COLOR(230, 230, 220));
		settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 2);
		settextjustify(CENTER_TEXT, TOP_TEXT);
		outtextxy(150, 210, "MEMORIE");

		settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 1);
		settextjustify(LEFT_TEXT, TOP_TEXT);

		int startY = 250;
		for (int i = 0; i < 26; i++) //iteram toate cele 26 de variabile pe care le putem utiliza pe timpul programului
		{
			char txt[30];
			if (abs(variabila[i]) < 0.001)
				sprintf_s(txt, "%c = 0", 'A' + i); //din 0 in A etc...
			else
				sprintf_s(txt, "%c = %.2f", 'A' + i, variabila[i]); //exact doua zecimale

			if (i < 13) //primele 13 variabile pe o coloana
			{
				outtextxy(20, startY + i * 35, txt);
			}
			else //urmatoarele 13 variabile [pe alta coloana
			{
				outtextxy(160, startY + (i - 13) * 35, txt);
			}
		}
	}

	settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 1);
	for (size_t i = 0; i < schema.size(); i++)//desenarea tuturor blocurilor
	{
		bool eSursa = (i == indexSursaLegatura); //e selectat cu clic dreapta?!? ca sa l facem albastru
		bool eExec = (i == blocCurentExecutie); //este selectat in procesul de executie!?!? il facem verde
		DeseneazaForma(schema[i].x, schema[i].y, schema[i].tip, schema[i].text, false, eSursa, eExec); //il desenam
	}

	if (indexSursaLegatura != -1 && indexSursaLegatura < (int)schema.size()) //daca avem un bloc selectat
	{
		Bloc b = schema[indexSursaLegatura];

		int latimeB = getLatimeBloc(indexSursaLegatura); //ne trebuie latimea blocului ca sa stim unde punem butoanele
		//butonul X si il desenam rosu
		int bx = b.x + (latimeB - 5);
		int by = b.y - 45;
		int marime = 20;
		setcolor(RED);
		setfillstyle(SOLID_FILL, RED);
		bar(bx, by, bx + marime, by + marime);
		setcolor(WHITE);
		setlinestyle(SOLID_LINE, 0, 2);
		line(bx + 3, by + 3, bx + marime - 3, by + marime - 3);
		line(bx + marime - 3, by + 3, bx + 3, by + marime - 3);
		//butonul de editare E 
		if (b.tip == TIP_ATRIBUIRE || b.tip == TIP_CITIRE || b.tip == TIP_DECIZIE || b.tip == TIP_AFISARE)
		{
			int ex = b.x - (latimeB + 15);
			int ey = b.y - 45;
			setcolor(COLOR(255, 200, 0));
			setfillstyle(SOLID_FILL, COLOR(255, 200, 0));
			bar(ex, ey, ex + marime, ey + marime);
			setcolor(BLACK);
			setbkcolor(COLOR(255, 200, 0));
			settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 1);
			outtextxy(ex + 6, ey + 2, "E");
		}
	}

	// DESENARE FEREASTRA POP-UP (SETARI)
	if (esteSetariDeschis)
	{
		setfillstyle(CLOSE_DOT_FILL, BLACK); // pattern care lasa sa se vada putin
		bar(0, 0, 1500, 750); //desenam un dreptunghi negru peste tot ecranul

		// fereastra
		int popW = 500, popH = 400;
		int popX = (1500 - popW) / 2;
		int popY = (750 - popH) / 2;

		setfillstyle(SOLID_FILL, COLOR(220, 220, 220));
		bar(popX, popY, popX + popW, popY + popH);
		setcolor(BLACK);
		setlinestyle(SOLID_LINE, 0, 3);
		rectangle(popX, popY, popX + popW, popY + popH);

		setbkcolor(COLOR(220, 220, 220));
		settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 3);
		settextjustify(CENTER_TEXT, TOP_TEXT);
		outtextxy(popX + popW / 2, popY + 20, "SETARI APLICATIE");

		// BUTON DE INCHIDERE (X)
		int btnSize = 30;
		int btnX = popX + popW - btnSize - 10;
		int btnY = popY + 10;
		setcolor(RED);
		rectangle(btnX, btnY, btnX + btnSize, btnY + btnSize);
		line(btnX, btnY, btnX + btnSize, btnY + btnSize);
		line(btnX, btnY + btnSize, btnX + btnSize, btnY);

		// SELECTARE CULOARE
		settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 2);
		setcolor(BLACK);
		settextjustify(LEFT_TEXT, TOP_TEXT);
		outtextxy(popX + 40, popY + 80, "Alege Culoare Contur:");

		int startSpecX = popX + 40;
		int startSpecY = popY + 110;
		int specW = 420;
		int specH = 60;

		// DESENARE SPECTRU ALA DE CULORI
		for (int i = 0; i < specW; i++)
		{
			float ratio = (float)i / specW;
			int r = 0, g = 0, b = 0;
			// ceva algorimt pentru curcubeu
			float h = ratio * 6.0f;
			float f = h - (int)h;
			int q = 255 * (1 - f);
			int t = 255 * f;
			switch ((int)h % 6) {
			case 0: r = 255; g = t; b = 0; break;
			case 1: r = q; g = 255; b = 0; break;
			case 2: r = 0; g = 255; b = t; break;
			case 3: r = 0; g = q; b = 255; break;
			case 4: r = t; g = 0; b = 255; break;
			case 5: r = 255; g = 0; b = q; break;
			}
			setcolor(COLOR(r, g, b));
			line(startSpecX + i, startSpecY, startSpecX + i, startSpecY + specH);
		}
		// Chenar spectru
		setcolor(BLACK);
		rectangle(startSpecX, startSpecY, startSpecX + specW, startSpecY + specH);


		// delay
		outtextxy(popX + 40, popY + 200, "Viteza Executie (Delay):");

		char bufferDelay[30];
		sprintf_s(bufferDelay, "%d ms", delayExecutie);
		outtextxy(popX + 350, popY + 200, bufferDelay);

		int sliderX = popX + 40;
		int sliderY = popY + 240;
		int sliderW = 420;

		setcolor(BLACK);
		setlinestyle(SOLID_LINE, 0, 3);
		line(sliderX, sliderY, sliderX + sliderW, sliderY);

		// cercu de care tragem
		int cercX = sliderX + (int)((float)delayExecutie / 2000.0f * sliderW);
		setfillstyle(SOLID_FILL, BLUE);
		fillellipse(cercX, sliderY, 10, 10);
	}
}

// efectiv editam bloc
void EditeazaTextBloc(int idx)
{
	while (kbhit()) //ca sa nu salveze tastele de dinainte ce pornim editarea
		getch();
	char buffer[50]; //facem o copie temporara a textului din bloc ca sa putem sterge, adauga 
	strcpy_s(buffer, schema[idx].text);
	int len = strlen(buffer);

	while (1) //cat timp nu citeste programul ENTER se ruleaza asta
	{
		strcpy_s(schema[idx].text, buffer);
		setactivepage(1 - getvisualpage());
		DeseneazaInterfata(); //copiaza continutului in structura schema la fiecare tasta apasata
		setcolor(BLACK);
		setbkcolor(COLOR(230, 230, 220));
		//facem o linie mica _ care imita cursorul de text
		int tw = textwidth(buffer);
		outtextxy(schema[idx].x + tw / 2 + 5, schema[idx].y - 5, "_");
		setvisualpage(1 - getvisualpage());

		char c = getch(); //nimic nu se misca pe ecran pana nu tastam
		if (c == 13) //daca e ENTER dam break
			break;
		if (c == 8) //daca e BACKSPACE ca sa stergem
		{
			if (len > 0)
			{
				buffer[len - 1] = '\0';
				len--;
			}
		}
		else if (len < 49 && c >= 32 && c <= 126) //pentru a verifica daca e caracter valid
		{
			buffer[len] = c;
			buffer[len + 1] = '\0';
			len++;
		}
	}
}

// efectiv adaugam bloc
void AdaugaBloc(int tip)
{
	Bloc b; //creem bloc nou
	b.tip = tip; //spunem tipul
	b.x = 650; //il punem pe centru
	b.y = 375;
	b.text[0] = '\0'; //cu text nul
	schema.push_back(b); //il bagam in schema
}

// functia verifica pe care bloc dau click
int ObtineIndexBlocLaMouse(int mx, int my)
{
	for (int i = schema.size() - 1; i >= 0; i--) //parcurgem invers (de la ultimele adaugate in schema) fiindca ele se suprapun in ordinea adaugarii
	{
		int latimeH = getLatimeBloc(i); //functie care zice cat de lat e blocul

		if (mx >= schema[i].x - latimeH && mx <= schema[i].x + latimeH &&
			my >= schema[i].y - 30 && my <= schema[i].y + 30)
		{
			return i; //daca am dat click pe un anuit bloc returnam indexul acestuia
		}
	}
	return -1; //daca s a terminat functia si n am gasit niciun bloc sub mouse
}

//////////////////////////////////////////////////////////////////////////////////////////////////// convertire in cod c++

// pentru a vedea daca un drum duce inapoi la un anumit bloc (pt detectie while)
bool drumulDuceLa(blocLogic* start, blocLogic* tinta, vector<int> vizitat) //e un fel de dfs fgbs
{
	if (start == nullptr) return false; //am ajuns la capatul unui drum
	if (start == tinta) return true; //e blocul pe care il cautam
	for (int id : vizitat) if (start->id == id) return false; //daca l am vizitat returnam false
	vizitat.push_back(start->id); //daca nu l am vizitat il vizitam

	if (start->tip == 5) //daca blocul e IF drumul se poate bifurca
		return drumulDuceLa(start->urmA, tinta, vizitat) || drumulDuceLa(start->urmF, tinta, vizitat); //verificam ambele ramuri (T/F)
	return drumulDuceLa(start->urm, tinta, vizitat); //asta pentru orice alt bloc
}

// transform simbolurile folosite pt expresii in actual c++ symbols
string formateazaPtCod(string s)
{
	string rezultat = "";
	for (size_t i = 0; i < s.length(); i++)
	{
		char c = s[i];

		// verificam special pt <= sau >=
		// daca gasim < sau > urmat imediat de =, le pastram asa cum sunt
		if ((c == '<' || c == '>') && i + 1 < s.length() && s[i + 1] == '=')
		{
			rezultat += c;      // add < sau >
			rezultat += "=";    // add =
			i++;                // jump peste urm char (=) ca sa nu il ia switchul
			continue;
		}

		// cazurile speciale
		switch (c)
		{
		case '=':
			rezultat += "=="; // ajunge doar daca e egalitate simpla (gen A=B sau G = 32)
			break;
		case '~':
			rezultat += "-"; // ajunge doar daca e egalitate simpla (gen A=B sau G = 32)
			break;
		case '#':
			rezultat += "!=";
			break;
		case 's':
			rezultat += "sin";
			break;
		case 'c':
			rezultat += "cos";
			break;
		case 'l':
			rezultat += "log";
			break;
		case 'e':
			rezultat += "exp";
			break;
		case 'a':
			rezultat += "abs";
			break;
		case 'r':
			rezultat += "sqrt";
			break;
			// simbolurile $ si @ sunt convertite doar daca au fost salvate asa
			// daca utilizatorul scrie direct <= in bloc, if-ul de mai sus se ocupa de ele
		case '@':
			rezultat += ">=";
			break;
		case '%':
			rezultat += "%";
			break;
		default:
			rezultat += c; // caracter normal (cifre, variabile A-Z, paranteze)
			break;
		}
	}
	return rezultat;
}

void scrieCodRecursiv(blocLogic* B, string& prog, int indent, blocLogic* stopAt = nullptr)
{
	if (B == nullptr || B == stopAt) return;

	string spatii = "";
	for (int k = 0; k < indent; k++)
		spatii += "    ";

	if (B->tip == 0) // STOP
	{
		return;
	}
	else if (B->tip == 1) // START
	{
		scrieCodRecursiv(B->urm, prog, indent, stopAt);
	}
	else if (B->tip == 2) // CITIRE
	{
		prog += spatii + "cin >> " + B->var + ";\n";
		scrieCodRecursiv(B->urm, prog, indent, stopAt);
	}
	else if (B->tip == 3) // AFISARE
	{
		if (B->expresie.length() > 0)
			prog += spatii + "cout << " + formateazaPtCod(B->expresie) + " << endl;\n";
		else
		{
			string s(1, B->var);
			prog += spatii + "cout << " + s + " << endl;\n";
		}
		scrieCodRecursiv(B->urm, prog, indent, stopAt);
	}
	else if (B->tip == 4) // ATRIBUIRE
	{
		string s(1, B->var);
		prog += spatii + s + " = " + formateazaPtCod(B->expresie) + ";\n";
		scrieCodRecursiv(B->urm, prog, indent, stopAt);
	}
	else if (B->tip == 5) // DECIZIE (IF sau WHILE)
	{
		vector<int> viz;
		// Daca drumul de TRUE se intoarce la blocul curent, e un WHILE
		if (drumulDuceLa(B->urmA, B, viz))
		{
			prog += spatii + "while (" + formateazaPtCod(B->expresie) + ")\n";
			prog += spatii + "{\n";
			scrieCodRecursiv(B->urmA, prog, indent + 1, B);
			prog += spatii + "}\n";
			// Dupa while, continuam pe ramura FALSE
			scrieCodRecursiv(B->urmF, prog, indent, stopAt);
		}
		else
		{
			prog += spatii + "if (" + formateazaPtCod(B->expresie) + ")\n";
			prog += spatii + "{\n";
			scrieCodRecursiv(B->urmA, prog, indent + 1, stopAt);
			prog += spatii + "}\n";
			prog += spatii + "else\n";
			prog += spatii + "{\n";
			scrieCodRecursiv(B->urmF, prog, indent + 1, stopAt);
			prog += spatii + "}\n";
		}
	}
}

void convertInCode()
{
	// validare initiala: verificam daca exista blocuri in schema
	if (schema.empty())
	{
		AdaugaLog("Schema goala! Nu pot genera cod.");
		return;
	}

	// alocam memorie pentru nodurile logice (structura interna folosita la generare)
	vector<blocLogic*> noduri(schema.size());
	blocLogic* startNode = nullptr;

	// etapa de creare a nodurilor: transformam datele din GUI in structuri logice
	for (size_t i = 0; i < schema.size(); i++)
	{
		noduri[i] = new blocLogic;
		noduri[i]->id = i;
		noduri[i]->guiIndex = i; // pastram referinta la indexul original
		noduri[i]->urm = noduri[i]->urmA = noduri[i]->urmF = nullptr; // initializam legaturile cu null
		string textRaw = schema[i].text;

		// setam tipul si datele specifice in functie de tipul blocului din schema
		switch (schema[i].tip)
		{
		case TIP_START:
			noduri[i]->tip = 1;
			startNode = noduri[i]; // memoram nodul de start pentru a incepe parcurgerea mai tarziu
			break;
		case TIP_STOP:
			noduri[i]->tip = 0;
			break;
		case TIP_CITIRE:
			noduri[i]->tip = 2;
			// presupunem ca variabila e primul caracter (ex: "A")
			if (textRaw.length() > 0) noduri[i]->var = textRaw[0];
			break;
		case TIP_AFISARE:
			noduri[i]->tip = 3;
			noduri[i]->expresie = textRaw; // tot textul este expresia de afisat
			break;
		case TIP_ATRIBUIRE:
			noduri[i]->tip = 4;
			{
				// parsam textul de tip "A=B+C" pentru a separa variabila de expresie
				size_t pos = textRaw.find('=');
				if (pos != string::npos) {
					noduri[i]->var = textRaw[0]; // variabila din stanga
					noduri[i]->expresie = textRaw.substr(pos + 1); // expresia din dreapta
				}
			}
			break;
		case TIP_DECIZIE:
			noduri[i]->tip = 5;
			noduri[i]->expresie = textRaw; // conditia (ex: "A > 0")
			break;
		}
	}

	// etapa de conectare: refacem legaturile dintre noduri pe baza liniilor trase in GUI
	for (const auto& leg : legaturi)
	{
		// verificam validitatea indecsilor
		if (leg.sursaIdx >= (int)schema.size() || leg.destIdx >= (int)schema.size())
			continue;

		blocLogic* src = noduri[leg.sursaIdx];
		blocLogic* dst = noduri[leg.destIdx];

		// daca sursa este un bloc de decizie, avem doua ramuri (T/F)
		if (src->tip == 5)
		{
			// logica simpla: prima legatura gasita e T, a doua e F
			if (src->urmA == nullptr)
				src->urmA = dst;
			else
				src->urmF = dst;
		}
		else
		{
			// pentru blocuri normale, exista o singura legatura urmatoare
			src->urm = dst;
		}
	}

	// validare: Nu putem genera cod fara un punct de start
	if (!startNode)
	{
		AdaugaLog("Lipsa start! Nu pot genera.");
		return;
	}

	// generarea structurii codului C++
	string program = "";
	// adaugam librariile standard si functia main
	program += "#include <iostream>\n#include <cmath>\nusing namespace std;\n\nint main() {\n";

	// detectarea automata a variabilelor folosite (A-Z)
	bool usedVar[26] = { false };
	for (auto* n : noduri)
	{
		// verificam variabila stocata in nod (pentru citire/atribuire)
		if (n->var >= 'A' && n->var <= 'Z') usedVar[n->var - 'A'] = true;
		// verificam variabilele care apar in expresii (pentru calcul/decizie/afisare)
		for (char c : n->expresie)
			if (c >= 'A' && c <= 'Z') usedVar[c - 'A'] = true;
	}

	// verificam daca exista cel putin o variabila de declarat
	bool existsAny = false;
	for (int i = 0; i < 26; i++) if (usedVar[i]) existsAny = true;

	// generam linia de declaratie (ex: double A, B, X;)
	if (existsAny)
	{
		program += "    double ";
		bool first = true;
		for (int i = 0; i < 26; i++) {
			if (usedVar[i]) {
				if (!first) program += ", ";
				program += (char)('A' + i);
				first = false;
			}
		}
		program += ";\n";
	}

	// generarea instructiunilor (apel recursiv)
	// functia scrieCodRecursiv va parcurge graful si va completa string-ul 'program'
	scrieCodRecursiv(startNode, program, 1);

	// incheiem functia main
	program += "    return 0;\n}\n";

	// finalizare: transferam codul in interfata grafica
	cppCodeLines.clear();
	scrollPosCod = 0;
	stringstream ss(program);
	string line;
	// spargem codul pe linii pentru a fi afisat usor intr-un editor/lista
	while (getline(ss, line, '\n')) {
		cppCodeLines.push_back(line);
	}

	AdaugaLog("Codul C++ a fost generat!");

	// stergem nodurile alocate dinamic la inceputul functiei
	for (auto* n : noduri)
		delete n;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////// save/load files 

bool existaFisierul(const string& nume)
{
	ifstream f(nume.c_str()); // deschidem fereastra
	return f.good(); // daca e ok
}

void salveazaSchemaCuDialog()
{
	OPENFILENAME ofn; // contine configurarile ferestrei de dialog (titlu, unde sa salveze etc)
	char szFile[260]; // buffer (sir de caractere) unde windows scrie calea completa a fisierului ales

	// initializam structura cu 0 ca sa nu avem "gunoaie" pe memorie :)))
	ZeroMemory(&ofn, sizeof(ofn));

	szFile[0] = '\0'; // initializam bufferul sa fie gol ca sa fim siguri

	// configuram setarile ferestrei
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL; // fereastra parinte (n are una specifica)
	ofn.lpstrFile = szFile; // pune windows calea fisierului rezultat
	ofn.nMaxFile = sizeof(szFile); // dim maxima a locatiei unde e salvat
	ofn.lpstrFilter = "Text Files\0*.txt\0All Files\0*.*\0"; // filtru doar pt .txt
	ofn.nFilterIndex = 1; // selecteaza implicit primul .txt
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL; // directorul initial (NULL inseamna ca windows decide ultimul accesat usually)
	ofn.lpstrDefExt = "txt"; // adauga automat extensia .txt daca userul nu o scrie

	// flag-uri: path trebuie sa existe, si cere confirmare daca suprascrii un fisier
	// OFN_PATHMUSTEXIST: calea folderului trebuie sa existe
	// OFN_OVERWRITEPROMPT: daca fisierul exista deja, intrb utilizatorul daca vrea sa il suprascrie
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	// deschidem fereastra de dialog
	if (GetSaveFileName(&ofn) == TRUE)
	{
		// daca utilizatorul a dat click pe SAVE, ofn.lpstrFile contine calea completa
		ofstream fout(ofn.lpstrFile);

		if (!fout.is_open())
		{
			AdaugaLog("ERR: Nu pot scrie in fisier!");
			return;
		}

		AdaugaLog("Salvare...");

		fout << schema.size() << '\n'; // scriem nr total de blocks
		for (const auto& w : schema) // parcurgem fiecare block si salvam proprietatile importante de la fiecare
		{
			fout << w.tip << " " << w.x << " " << w.y << "\n"; // scriem tipul (exm start/stop), poz x si poz y
			// scriem textul din bloc
			// daca blocul nu are text scriem "[GOL]" ca sa stim la citire ca e gol
			if (strlen(w.text) == 0)
				fout << "[GOL]" << "\n";
			else
				fout << w.text << "\n";
		}

		fout << legaturi.size() << "\n"; // scriem nr de legaturi
		for (const auto& leg : legaturi) // aici avem sursa si destinatia lor
			fout << leg.sursaIdx << " " << leg.destIdx << "\n";

		fout.close(); // inchidem fisierul ca sa salvam modificarile
		AdaugaLog("Schema salvata cu succes!");
	}
	else
		AdaugaLog("Salvare anulata."); // daca utilizatorul a dat cancel sau X la fereastra
}

void loadSchemaDinFolder()
{
	OPENFILENAME ofn; // contine configurarile ferestrei de dialog (titlu, unde sa salveze etc)
	char szFile[260]; // buffer (sir de caractere) unde windows scrie calea completa a fisierului ales

	// initializam structura cu 0 ca sa nu avem "gunoaie" pe memorie :)))
	ZeroMemory(&ofn, sizeof(ofn));

	// configuram setarile ferestrei
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL; // fereastra parinte (n are una specifica)
	ofn.lpstrFile = szFile; // pune windows calea fisierului rezultat
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile); // dim maxima a locatiei unde e salvat
	ofn.lpstrFilter = "Text Files\0*.txt\0All Files\0*.*\0"; // filtru doar pt .txt
	ofn.nFilterIndex = 1; // selecteaza implicit primul .txt
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL; // directorul initial (NULL inseamna ca windows decide ultimul accesat usually)
	ofn.lpstrDefExt = "txt"; // adauga automat extensia .txt daca userul nu o scrie

	// flag-uri: path trebuie sa existe, si cere confirmare daca suprascrii un fisier
	// OFN_PATHMUSTEXIST: calea folderului trebuie sa existe
	// OFN_OVERWRITEPROMPT: daca fisierul exista deja, intrb utilizatorul daca vrea sa il suprascrie
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	if (GetOpenFileName(&ofn) == TRUE) // deschide fereastra "Open File", return 1 daca utilizatorul a ales un fisier
	{
		ifstream fin(ofn.lpstrFile); // deschidem fisierul sa citim din el
		if (!fin.is_open())
		{
			AdaugaLog("ERR: Nu pot deschide fisierul!");
			return;
		}

		AdaugaLog("Incarcare...");

		// curatam memoria ca daca nu se vor suprapune schemele
		schema.clear();
		legaturi.clear();
		indexSursaLegatura = -1; // resetam selectiile curente
		indexBlocSelectat = -1;

		int nrBlocuri;
		fin >> nrBlocuri; // citim cate blocuri urmeaza

		for (int i = 0; i < nrBlocuri; i++)
		{
			Bloc b;
			fin >> b.tip >> b.x >> b.y; // tipul si pozitia
			fin.get(); // ca sa trecem peste \n
			char bufferText[100];
			fin.getline(bufferText, 100); // citim tot restul liniei ca text pana la enter

			if (strstr(bufferText, "[GOL]") != NULL) // verificam daca e gol marcatorul special inseamna ca blocul n avea text
				b.text[0] = '\0';
			else
				strcpy_s(b.text, bufferText); // copiam textul citit in structura

			schema.push_back(b); // adaugam ce am construit in vector
		}

		int nrLegaturi;
		fin >> nrLegaturi; // citim nr de leg care urmeaza
		for (int i = 0; i < nrLegaturi; i++)
		{
			Legatura leg;
			fin >> leg.sursaIdx >> leg.destIdx;
			legaturi.push_back(leg);
		}

		fin.close(); // inchidem fisierul
		AdaugaLog("Schema incarcata cu succes!");
	}
	else
		AdaugaLog("Incarcare anulata.");
}

int main()
{
	initwindow(1500, 750, "TABLA");
	int paginaActiva = 0;
	bool trebuieDesenat = true;
	int oldMx = -1, oldMy = -1;

	while (1)
	{
		if (ismouseclick(WM_LBUTTONUP))    //astea trei sunt pentru atunci cand ridicam degetul de pe mouse
			clearmouseclick(WM_LBUTTONUP);
		if (ismouseclick(WM_RBUTTONUP))
			clearmouseclick(WM_RBUTTONUP);
		if (ismouseclick(WM_MOUSEMOVE))
			clearmouseclick(WM_MOUSEMOVE);

		if (GetAsyncKeyState(VK_UP) & 0x8000) { //daca apasam in sus, la partea aia de codificare in c++ scroll in sus
			if (scrollPosCod > 0) {
				scrollPosCod--;
				trebuieDesenat = true;
				delay(60);
			}
		}
		if (GetAsyncKeyState(VK_DOWN) & 0x8000) { //daca apasam in jos, la partea aia de codificare in c++ scroll in jos
			if (scrollPosCod < (int)cppCodeLines.size() - 1) {
				scrollPosCod++;
				trebuieDesenat = true;
				delay(60);
			}
		}

		if (mousex() != oldMx || mousey() != oldMy) //daca s-a schimbat coordonata mouseului inseamna ca efectiv el s a miscat
		{
			if (indexBlocSelectat != -1) //daca avem un bloc in mana trebuie sa desenam
				trebuieDesenat = true;
			oldMx = mousex(); //actualizam variabilele cu pozitia curenta
			oldMy = mousey();
		}

		if (ismouseclick(WM_LBUTTONDOWN)) //daca e apasat click stanga
		{
			int mx, my;
			getmouseclick(WM_LBUTTONDOWN, mx, my);

			// DACA MENIUL DE SETARI E DESCHIS, ACCEPTAM CLICK-URILE DOAR AICI
			if (esteSetariDeschis)
			{
				int popW = 500, popH = 400; //plasam fereastra de setari fix in mijlocul ecranului
				int popX = (1500 - popW) / 2;
				int popY = (750 - popH) / 2;

				// click pe butonul X / butonul de inchidere l ao adica
				int btnSize = 30;
				int btnX = popX + popW - btnSize - 10;
				int btnY = popY + 10;
				if (mx >= btnX && mx <= btnX + btnSize && my >= btnY && my <= btnY + btnSize)
				{
					esteSetariDeschis = false;
					trebuieDesenat = true;
				}
				// click pe spectrul de culori
				int startSpecX = popX + 40;
				int startSpecY = popY + 110;
				int specW = 420;
				int specH = 60;
				if (mx >= startSpecX && mx <= startSpecX + specW && my >= startSpecY && my <= startSpecY + specH)
				{
					// calculam culoarea bazata pe pozitia X
					int i = mx - startSpecX; //afla la cati pixeli distanta de inceputul barei am dat click
					float ratio = (float)i / specW; //transforma pozitia intr un procent
					float h = ratio * 6.0f;
					float f = h - (int)h;
					int q = 255 * (1 - f);
					int t = 255 * f;
					int r = 0, g = 0, b = 0;
					switch ((int)h % 6) { //ceva algoritm clasic de schimbare a culorii
					case 0: r = 255; g = t; b = 0; break;
					case 1: r = q; g = 255; b = 0; break;
					case 2: r = 0; g = 255; b = t; break;
					case 3: r = 0; g = q; b = 255; break;
					case 4: r = t; g = 0; b = 255; break;
					case 5: r = 255; g = 0; b = q; break;
					}
					culoareContur = COLOR(r, g, b);//variabila globala pentru contur
					trebuieDesenat = true;
				}

				// click pe bara de Delay
				int sliderX = popX + 40;
				int sliderY = popY + 240;
				int sliderW = 420;
				// detectam clickul
				if (mx >= sliderX && mx <= sliderX + sliderW && my >= sliderY - 15 && my <= sliderY + 15)
				{
					float ratio = (float)(mx - sliderX) / sliderW;
					delayExecutie = (int)(ratio * 2000); // max 2 secunde
					if (delayExecutie < 0) delayExecutie = 0; //variabila globala pentru delay
					trebuieDesenat = true;
				}
			}
			else
			{
				// LOGICA NORMALA A APLICATIEI (CAND SETARILE NU SUNT DESCHISE)
				bool actiuneFacuta = false;

				// CLICK PE BUTONUL DE SETARI (SUS)
				if (mx >= 300 && mx <= 1000 && my >= 0 && my <= 50)
				{
					esteSetariDeschis = true;
					trebuieDesenat = true;
					actiuneFacuta = true;
				}

				if (!actiuneFacuta && mx >= 1000 && mx <= 1250 && my >= 0 && my <= 50) //daca am dat click pe EXECUTA COD
				{
					ConvertesteSiRuleaza();
					trebuieDesenat = true;
					actiuneFacuta = true;
				}

				if (!actiuneFacuta && mx >= 1000 && mx <= 1310 && my >= 380 && my <= 430) //daca am dat click pe CODIFICARE IN C++
				{
					convertInCode();
					trebuieDesenat = true;
					actiuneFacuta = true;
				}

				if (!actiuneFacuta && indexSursaLegatura != -1 && indexSursaLegatura < (int)schema.size()) //indexsursalegatura e ca sa vedem daca avem click pe un bloc
				{
					Bloc b = schema[indexSursaLegatura];
					//cu astea doua vedem cat de lat e blocul
					int latimeB = getLatimeBloc(indexSursaLegatura);

					int bx = b.x + (latimeB - 5); //butonul de X (stergere) al blocului
					int by = b.y - 45;
					if (mx >= bx && mx <= bx + 20 && my >= by && my <= by + 20)
					{
						StergeBlocSiLegaturi(indexSursaLegatura); //efectiv stergem blocul
						indexSursaLegatura = -1; //ca sa nu creada programul ca mai avem vreun bloc in mana
						indexBlocSelectat = -1;
						trebuieDesenat = true;
						actiuneFacuta = true;
					}

					int ex = b.x - (latimeB + 15); //butonul E (de editare) al blocului
					int ey = b.y - 45;
					if (!actiuneFacuta && (b.tip == TIP_ATRIBUIRE || b.tip == TIP_CITIRE || b.tip == TIP_DECIZIE || b.tip == TIP_AFISARE)) //blocurile de start si stop n au nevoie de editare
					{
						if (mx >= ex && mx <= ex + 20 && my >= ey && my <= ey + 20)
						{
							EditeazaTextBloc(indexSursaLegatura);//efectiv editam blocu
							trebuieDesenat = true;
							actiuneFacuta = true;
						}
					}
				}

				if (!actiuneFacuta)
				{
					if (mx >= 0 && mx <= 300 && my >= 0 && my <= 40) //incarcam schema din folder
					{
						loadSchemaDinFolder();
						actiuneFacuta = true;
						trebuieDesenat = true;
					}
					else if (mx >= 0 && mx <= 300 && my >= 40 && my <= 90) //salvam schema in folder
					{
						salveazaSchemaCuDialog();
						actiuneFacuta = true;
						trebuieDesenat = true;
					}
					// LOGICA PENTRU SCHEMA NOUA (CURATARE TABLA)
					else if (mx >= 0 && mx <= 300 && my >= 90 && my <= 140) //efectiv curatam tot
					{
						schema.clear();
						legaturi.clear();
						indexSursaLegatura = -1;
						indexBlocSelectat = -1;
						blocCurentExecutie = -1;
						cppCodeLines.clear();
						outputConsole.clear();
						AdaugaLog("TABLA A FOST CURATATA.");
						trebuieDesenat = true;
						actiuneFacuta = true;
					}
					else if (mx >= 0 && mx <= 300 && my >= 142 && my <= 190)
					{
						esteMeniulDeschis = !esteMeniulDeschis; //un fel de intrerupator (cand apsam isi schimba valoarea)
						trebuieDesenat = true;
					}
					else if (esteMeniulDeschis && mx >= 0 && mx <= 300 && my > 195 && my < 565) //daca e meniul de blocuri deschis
					{
						int idx = (my - 205) / 60; //transformam coordonata in tipul blocului
						if (idx >= 0 && idx <= 5)
						{
							AdaugaBloc(idx); //adaugam blocul de tipul ala efectiv
							trebuieDesenat = true;
						}
					}
					else if (mx > 300 && mx < 1000) //asta e patratul in care punem blocurile
					{
						int idx = ObtineIndexBlocLaMouse(mx, my); //verificam daca am dat click pe vreun bloc
						if (idx != -1)
						{
							indexBlocSelectat = idx;
							offsetX = schema[idx].x - mx; //distanta dintre centrul blocului si mouse ca sa putem trage blocul cum trebuie
							offsetY = schema[idx].y - my;
							if (indexSursaLegatura != idx)
								indexSursaLegatura = -1;
							trebuieDesenat = true;
						}
						else //daca dam click pe fundal se anuleaza selectia blocului
						{
							indexSursaLegatura = -1;
							trebuieDesenat = true;
						}
					}
				}
			}
		}

		if (ismouseclick(WM_RBUTTONDOWN) && !esteSetariDeschis) // click dreaptae este blocat cand sunt setarile ca sa nu stricam programul
		{
			int mx, my;
			getmouseclick(WM_RBUTTONDOWN, mx, my);
			int idx = ObtineIndexBlocLaMouse(mx, my);

			if (idx != -1) //daca am dat click pe un bloc
			{

				if (indexSursaLegatura == -1) //incercam sa stabilim de unde pleaa sageata
				{
					if (schema[idx].tip != TIP_STOP) //fiindca nu putem trage o legatura din STOP
					{
						if (schema[idx].tip == TIP_START)
						{
							bool areDejaLegatura = false; //blocul de start poate avea doar o legatura iesita din el
							for (const auto& l : legaturi) //parcurgem toate legaturile existente sa verificam daca are vreo legatura
							{
								if (l.sursaIdx == idx)
								{
									areDejaLegatura = true;
									break;
								}
							}
							if (!areDejaLegatura)
							{
								indexSursaLegatura = idx;
								trebuieDesenat = true;
							}
						}
						else //daca nu e start sau stop punem direct ca sursa e blocul
						{
							indexSursaLegatura = idx;
							trebuieDesenat = true;
						}
					}
				}
				else if (indexSursaLegatura != idx) //inseamna ca avem deja o sursa selectata si nu permite sa puna legatura tot la el
				{
					if (schema[idx].tip != TIP_START) //nu putem trage legatura intr un bloc start
					{
						Legatura l; //facem legatura
						l.sursaIdx = indexSursaLegatura; //salvam sursa
						l.destIdx = idx; //salvam destinatia
						legaturi.push_back(l); //o punem in vectoru de legaturi
						indexSursaLegatura = -1; //gata procesu de sageti
						trebuieDesenat = true;
					}
					else
					{
						indexSursaLegatura = -1;
						trebuieDesenat = true;
					}
				}
				else
				{
					indexSursaLegatura = -1; //daca ne am razgandit din a pune legatura, un fel de cancel la o adica
					trebuieDesenat = true;
				}
			}
			else
			{
				//asta e ca sa stergem legatura cand dam click dreapta pe ea
				bool sters = false;
				for (int k = 0; k < (int)legaturi.size(); k++) {
					// recalculam coordonatele legaturii exact ca in DeseneazaInterfata
					if (legaturi[k].sursaIdx >= (int)schema.size() || legaturi[k].destIdx >= (int)schema.size()) continue;
					//recalculam traseul ca sa stergem fiecare pixel
					Bloc& src = schema[legaturi[k].sursaIdx];
					Bloc& dst = schema[legaturi[k].destIdx];
					int sx, sy;
					int latimeSursa = getLatimeBloc(legaturi[k].sursaIdx);

					if (src.tip == TIP_DECIZIE) { //recalculam daca linia pleaca din dreapta sau stanga
						int nrLegaturiAnterioare = 0;
						for (int j = 0; j < k; j++) {
							if (legaturi[j].sursaIdx == legaturi[k].sursaIdx) nrLegaturiAnterioare++;
						}
						if (nrLegaturiAnterioare == 0) sx = src.x - latimeSursa;
						else sx = src.x + latimeSursa;
						sy = src.y;
					}
					else {
						sx = src.x;
						sy = src.y + 25;
					}


					int finalDestY = dst.y - 25;
					bool hit = false;

					if (finalDestY < sy) { //daca destinatia este mai sus decat sursa sageata face un ocol prin stanga
						int latS = getLatimeBloc(legaturi[k].sursaIdx);
						int exitX = sx - (latS + 30);
						if (AproapeDeSegment(mx, my, sx, sy, exitX, sy)) hit = true;
						if (AproapeDeSegment(mx, my, exitX, sy, exitX, finalDestY - 20)) hit = true;
						if (AproapeDeSegment(mx, my, exitX, finalDestY - 20, dst.x, finalDestY - 20)) hit = true;
						if (AproapeDeSegment(mx, my, dst.x, finalDestY - 20, dst.x, finalDestY)) hit = true;
					}
					else { // elbow in jos
						int midY = (sy + finalDestY) / 2;
						bool blocat = true;
						int incercari = 0;
						while (blocat && incercari < 10) {
							if (esteInCaleaOrizontala(sx, dst.x, midY)) { midY += 40; incercari++; }
							else blocat = false;
						}
						if (AproapeDeSegment(mx, my, sx, sy, sx, midY)) hit = true;
						if (AproapeDeSegment(mx, my, sx, midY, dst.x, midY)) hit = true;
						if (AproapeDeSegment(mx, my, dst.x, midY, dst.x, finalDestY)) hit = true;
					}

					if (hit) { //daca trebuie sters
						legaturi.erase(legaturi.begin() + k);
						trebuieDesenat = true;
						sters = true;
						break;
					}
				}

				if (!sters) {//un buton de cancel, gen daca voiam sa tragem o sageata si ne am razgandit
					indexSursaLegatura = -1;
					trebuieDesenat = true;
				}
			}
		}

		if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) && indexBlocSelectat != -1 && !esteSetariDeschis) //daca tinem apasat click stanga si avem un bloc in mana
		{
			int mx = mousex(); //calculam pozitia
			int my = mousey();
			int newX = mx + offsetX;
			int newY = my + offsetY;
			if (newX < 350) newX = 350; //peretii
			if (newX > 950) newX = 950;
			if (newY < 75) newY = 75;
			if (newY > 725) newY = 725;

			if (schema[indexBlocSelectat].x != newX || schema[indexBlocSelectat].y != newY) //daca schimbam blocul actualizam structura si cerem redesenarea
			{
				schema[indexBlocSelectat].x = newX;
				schema[indexBlocSelectat].y = newY;
				trebuieDesenat = true;
			}
		}
		else
		{
			indexBlocSelectat = -1; //dam drumu blocului
		}

		if (trebuieDesenat) //sistemul de DOUBLE BUFFERING, tehnica pentru animatii fluide in winbgim
		{
			setactivepage(paginaActiva); //desenam o pagina invizibila
			DeseneazaInterfata(); //functia care deseneaza totul
			setvisualpage(paginaActiva); //muta instantaneu ecranul sa arate pagina pe care tocmai am desenat o
			paginaActiva = 1 - paginaActiva; //schimbam pagina
			trebuieDesenat = false; //am terminat
		}

		delay(10); //ca sa nu dea crash programul
	}

	getch();
	closegraph(ALL_WINDOWS);
	return 0;
}