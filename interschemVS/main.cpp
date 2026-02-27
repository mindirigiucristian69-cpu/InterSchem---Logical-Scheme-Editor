/**
						  TO DO LIST
				V 1. Make sure ts works
				2. Make translator from bloc* B to actual c++ code
				3. sa mearga pt += -= /= etc
**/


#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>
#include <stack>
#include <string>
#define NMAX 100005
#define PI 3.1415926
#define INF INT_MAX
#define EPSILON 0.0001
#define ESC "\033["
#define GREEN "32"
#define RED "31"
#define YELLOW "33"
#define PURPLE "35"
#define BLUE "34"
#define RESET "\033[m"
using namespace std;

ifstream fin("file.in");
ofstream fout("file.out");

struct bloc {
	int id; // id unic
	int tip; // mai jos
	/**
	0 stop
	1 start 
	2 cin
	3 cout
	4 atribuire
	5 if
	**/
	double valoare; // val pt cin/rezultatul if ului
	string expresie; // expresia din cin/if
	char var; // variabila unde retinem rezultatul
	bloc* prim; // pt rulare din consola                        [nu e nevoie neaparat in final project]
	bloc* urm; // urmatorul nod
	bloc* urmA; // pt if daca e adv expresia
	bloc* urmF; // pt if daca e falsa expresia
};

double variabila[NMAX];

stack <char> operatori;
stack <double> operanzi;

char operatiiBinare[] = "+-*/^<>=#";
char operatiiUnare[] = "scarel";
char operatii[] = "+-*/^<>=#scarel";

int prioritate(char ch)
{
	if (ch == '(' || ch == ')')
		return 0;
	
	if (ch == '+' || ch == '-')
		return 1;

	if (ch == '*' || ch == '/')
		return 2;

	if (ch == '^')
		return 3;

	if (ch == '=' || ch == '#' || ch == '<' || ch == '>')
		return 4;
	
	if (ch == 'c' || ch == 's' || ch == 'l' || ch == 'e' || ch == 't' || ch == 'a' || ch == 'r')
		return 5;
}

bool difInf(double x)
{
	return fabs(INF - fabs(x)) > INF / 2.0;
}

double logaritm(double x)
{
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
	return x == y;
}

double maiMic(double x, double y)
{
	return x < y;
}

double maiMare(double x, double y)
{
	return x > y;
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

		if (fabs(rez) < EPSILON)
			return 0;

		return rez;
	}
	return INF;
}

double cosinus(double x)
{
	if (difInf(x))
	{
		double radiani = x * PI / 180.0;
		double rez = cos(radiani);

		if (fabs(rez) < EPSILON) 
			return 0;

		return rez;
	}
	return INF;
}

double modul(double x)
{
	if (difInf(x))
		return fabs(x);
	
	return INF;
}

double radical(double x)
{
	if (difInf(x) && (x > EPSILON))
		return sqrt(x);

	return INF;
}

bool esteNumar(string sir)
{
	if (sir.empty()) return false;
	// It's a number if the first char is a digit
	if (isdigit(sir[0])) return true;
	// Or if it's a negative number (minus followed by digit)
	if (sir[0] == '-' && sir.length() > 1 && isdigit(sir[1])) return true;
	return false;
}

struct functie {
	string expresie;
	vector <string> v;
	bool valid;
};

double valoareFunctie(functie E)
{
	double val, x1, x2;
	val = x1 = x2 = 0;

	// resetez stivele
	while (!operanzi.empty()) 
		operanzi.pop();

	while (!operatori.empty()) 
		operatori.pop();

	for (int i = 0; i < E.v.size(); i++)
	{
		string token = E.v[i];

		if (esteNumar(token))
		{
			val = atof(token.c_str());
			operanzi.push(val);
		}
		else
		{
			if (token >= "A" && token <= "Z")
				operanzi.push(variabila[token[0] - 'A']);
			else if (token == "q") 
				operanzi.push(PI);
			else if (token == "(") 
				operatori.push('(');
			else if (token == ")") 
			{
				// procesez tot pana la '('
				while (!operatori.empty() && operatori.top() != '(')
				{
					char op = operatori.top();
					operatori.pop();

					if (operanzi.empty()) // sa fie safe :))
						break;

					x2 = operanzi.top(); 
					operanzi.pop();

					if (strchr(operatiiBinare, op)) 
					{
						if (operanzi.empty()) 
							break;  // sa fie safe :))

						x1 = operanzi.top(); 
						operanzi.pop();
					}

					// calculez
					switch (op) {
					case '=': val = egalitate(x1, x2); break;
					case '#': val = !egalitate(x1, x2); break;
					case '<': val = maiMic(x1, x2); break;
					case '>': val = maiMare(x1, x2); break;
					case '+': val = adunare(x1, x2); break;
					case '-': val = scadere(x1, x2); break;
					case '*': val = inmultire(x1, x2); break;
					case '/': val = impartire(x1, x2); break;
					case '^': val = putere(x1, x2); break;
					case 's': val = sinus(x2); break;
					case 'c': val = cosinus(x2); break;
					case 'l': val = logaritm(x2); break;
					case 'e': val = exponential(x2); break;
					case 'a': val = modul(x2); break;
					case 'r': val = radical(x2); break;
					}

					operanzi.push(val);
				}

				if (!operatori.empty()) // pop la '('
					operatori.pop();
			}
			else 
			{
				// e un operator + - * etc, deci procesez cei cu prioritate mai mare primii
				while (!operatori.empty() && operatori.top() != '(' &&
					prioritate(operatori.top()) >= prioritate(token[0]))
				{
					char op = operatori.top();
					operatori.pop();

					if (operanzi.empty()) 
						break;

					x2 = operanzi.top(); 
					operanzi.pop();

					if (strchr(operatiiBinare, op)) {
						if (operanzi.empty()) 
							break;

						x1 = operanzi.top(); 
						operanzi.pop();
					}

					switch (op) {
					case '=': val = egalitate(x1, x2); break;
					case '#': val = !egalitate(x1, x2); break;
					case '<': val = maiMic(x1, x2); break;
					case '>': val = maiMare(x1, x2); break;
					case '+': val = adunare(x1, x2); break;
					case '-': val = scadere(x1, x2); break;
					case '*': val = inmultire(x1, x2); break;
					case '/': val = impartire(x1, x2); break;
					case '^': val = putere(x1, x2); break;
					case 's': val = sinus(x2); break;
					case 'c': val = cosinus(x2); break;
					case 'l': val = logaritm(x2); break;
					case 'e': val = exponential(x2); break;
					case 'a': val = modul(x2); break;
					case 'r': val = radical(x2); break;
					}

					operanzi.push(val);
				}
				// push la operatorul curent
				operatori.push(token[0]);
			}
		}
	}

	// dam clean up 
	while (!operatori.empty()) {
		char op = operatori.top();
		operatori.pop();

		if (operanzi.empty()) 
			break;

		x2 = operanzi.top(); 
		operanzi.pop();

		if (strchr(operatiiBinare, op)) 
		{
			if (operanzi.empty()) 
				break;

			x1 = operanzi.top(); 
			operanzi.pop();
		}

		switch (op) {
		case '=': val = egalitate(x1, x2); break;
		case '#': val = !egalitate(x1, x2); break;
		case '<': val = maiMic(x1, x2); break;
		case '>': val = maiMare(x1, x2); break;
		case '+': val = adunare(x1, x2); break;
		case '-': val = scadere(x1, x2); break;
		case '*': val = inmultire(x1, x2); break;
		case '/': val = impartire(x1, x2); break;
		case '^': val = putere(x1, x2); break;
		case 's': val = sinus(x2); break;
		case 'c': val = cosinus(x2); break;
		case 'l': val = logaritm(x2); break;
		case 'e': val = exponential(x2); break;
		case 'a': val = modul(x2); break;
		case 'r': val = radical(x2); break;
		}

		operanzi.push(val);
	}

	if (!operanzi.empty())
		return operanzi.top();

	return 0;
}

bool expresieCorecta(string expresie)
{
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

	if (n <= 2) // nu crd ca exista expresii de 2 caractere
		return 0;

	while (i < n -	1)
	{
		if (expresie[i] == '(' && strchr(operatiiBinare, expresie[i + 1]))
			return 0;

		if (strchr(operatii, expresie[i]) && expresie[i + 1] == ')')
			return 0;

		if (strchr(operatiiBinare, expresie[i]) && strchr(operatiiBinare, expresie[i + 1]))
			return 0;

		i++;
	}

	return 1;
}

functie buildFunction(string& expresie)
{
	functie F;
	string temp;
	int i;


	i = 0;
	while (expresie[i] != NULL)
	{
		if (expresie[i] != ' ')
			temp += expresie[i];

		i++;
	}

	expresie = temp;
	F.expresie = expresie;
	F.valid = expresieCorecta(expresie);

	if (F.valid)
	{
		bool areParantezeFinale = (expresie.length() > 0 && 
								   expresie.front() == '(' && 
								   expresie.back() == ')');

		if (!areParantezeFinale) 
			F.v.push_back("(");

		i = 0;
		int n = expresie.length();

		while (i < n)
		{
			if (expresie[i] == ' ')
				i++;
			else if (isdigit(expresie[i]) || expresie[i] == '.')
			{
				string nr = "";
				while (i < n && (isdigit(expresie[i]) || expresie[i] == '.'))
				{
					nr += expresie[i];
					i++;
				}

				F.v.push_back(nr);
			}
			else
			{
				string s(1, expresie[i]);
				// check sa scot literele mici fara cele din operatiiUnare
				if (strchr(operatii, expresie[i]) ||
					(expresie[i] >= 'A' && expresie[i] <= 'Z') ||
					expresie[i] == '(' ||
					expresie[i] == ')')
					F.v.push_back(s);

				i++;
			}
		}

		if (!areParantezeFinale)
			F.v.push_back(")");
	}

	// debug
	//for (auto w : F.v) 
	//	fout << w << " ";

	//fout << "\n";
	//fout << F.valid << "\n";
	//fout << F.expresie << "\n";

	return F;
}

void evalueaza(string& expresie, double& valoare)
{
	functie F;
	// string expresie remove spatii                      V
	// din expresia din string in vectorul v              V
	// corect parantezat                                  V
	// sir corect, sa nu am ++ sau ** etc                 V
	// pt numere reale float gen daca am A + 3.2          V
	F = buildFunction(expresie);
	valoare = valoareFunctie(F);

	//cout << valoareFunctie(F, valoare);
}

void executie(bloc* B) {
	if (B == nullptr)
	{
		cout << ESC << PURPLE << "m" << "[EXEC] NULL\n" << RESET;
		return;
	}

	if (B->tip == 0) { // stop
		cout << ESC << PURPLE << "m" << "[EXEC] Stop Block\n" << RESET;
		return;
	}
	else if (B->tip == 1) { // start
		cout << ESC << PURPLE << "m" << "[EXEC] Start Block\n" << RESET;
	}
	else if (B->tip == 2) { // citire

		cout << ESC << PURPLE << "m" << "[EXEC] Read Block " << RESET;
		cout << "[" << B->var << "]";
		cout << ESC << PURPLE << "m" << " is equal to (enter value): " << RESET;
		cin >> variabila[B->var - 'A'];
	}
	else if (B->tip == 3) { // scriere/afisare
		cout << ESC << PURPLE << "m" << "[EXEC] Write Block " << RESET;
		cout << "[" << B->var << "]";
		cout << ESC << PURPLE << "m" << " has the output " << RESET;
		cout << "[" << variabila[B->var - 'A'] << "]\n";
	}
	else if (B->tip == 4) { // atribuire
		cout << ESC << PURPLE << "m" << "[EXEC] Assignment Block " << RESET;
		evalueaza(B->expresie, variabila[B->var - 'A']);
		B->valoare = variabila[B->var - 'A'];
		cout << "[" << B->var << "] = " << "[" << B->expresie << "]" << ESC << PURPLE << "m" << " with value "
			<< RESET << "[" << B->valoare << "]\n";
	}
	else if (B->tip == 5) { // decizie
		cout << ESC << PURPLE << "m" << "[EXEC] Decision Block " << RESET;
		evalueaza(B->expresie, B->valoare); // evaluez expresia

		if (B->valoare == true)
		{
			B->urm = B->urmA;
			cout << ESC << GREEN << "m" << " TRUE" << RESET;
		}
		else
		{
			B->urm = B->urmF;
			cout << ESC << RED << "m" << " FALSE" << RESET;
		}
		cout << "\n";
	}

	executie(B->urm);
}

void fileDoctor()
{
	{
		if (!fin.is_open()) {
			cout << "ERROR: File not open.\n";
			return;
		}

		// Check file size
		fin.seekg(0, ios::end);
		int fileSize = fin.tellg();
		fin.seekg(0, ios::beg); // Go back to start

		if (fileSize == 0) {
			cout << "CRITICAL ERROR: The program sees 'file.in' as EMPTY (0 bytes).\n";
			cout << "You are editing the wrong file. Check your project folder.\n";
			return;
		}

		cout << "File found! Size: " << fileSize << " bytes.\n";

		// Check for hidden characters (BOM)
		int firstChar = fin.peek();
		if (!isdigit(firstChar) && firstChar != '-') {
			cout << "WARNING: The first character is NOT a number. It is ASCII code: " << firstChar << "\n";
			cout << "This breaks 'fin >> id'. Save your file as 'ANSI' or 'UTF-8 NO BOM'.\n";
		}
	}
}

void codificare(bloc *B)
{
	string program;
	program = "#include <iostream>\n#include <cmath>\nusing namespace std;\n\nint main() {\n";


	fout << program;
}

int main()
{
	fileDoctor();

	int id, tip;
	char sarPesteEgal;
	memset(variabila, 0, sizeof(variabila));

	bloc* prim = NULL;
	bloc* ultim = NULL;

	// 2. Read Loop
	while (fin >> id && id != -1) {

		bloc* temp = new bloc;
		temp->id = id;
		temp->urm = NULL;
		temp->urmA = NULL;
		temp->urmF = NULL;

		fin >> tip;
		temp->tip = tip;

		// Debug output to see progress
		cout << "[LOAD] ID: " << id << " Type: " << tip << "... ";

		if (tip == 2 || tip == 3) { // citire si scriere
			fin >> temp->var;
			cout << "Var: " << temp->var << "\n";
		}
		else if (tip == 4) { // atribuire
			fin >> temp->var;
			fin >> sarPesteEgal; // citeste '=' ca sa sar peste el, ca n am nevoie sa l retin
			string expresie;
			getline(fin >> ws, expresie); // ia restul liniei netinand cont de spatii (asta face ws)
			temp->expresie = expresie;
			cout << "Expr: " << temp->expresie << "\n";
		}
		else if (tip == 5) { // if
			string conditie;
			getline(fin >> ws, conditie); // ia restul liniei netinand cont de spatii (asta face ws)
			temp->expresie = conditie;
			cout << "Decision: " << temp->expresie << "\n";
		}
		else {
			cout << "Done.\n";
		}

		if (prim == NULL) {
			prim = temp;
			ultim = temp;
		}
		else {
			ultim->urm = temp; // dai link la nodu vechi la asta nou
			ultim = temp;      // dau move la ultim in nodu curent (ultimul curent practic)
		}
	}

	cout << "\n--- Starting Execution ---\n";
	executie(prim); // prim o sa fie adresa la start

	//codificare(prim);
	return 0;
}

/**
1 1
2 2 A
3 2 B
5 4 X   =   (A * 10) + 2 * 4 * B
6 3 X
7 5 X = 30
7 0
-1
**/