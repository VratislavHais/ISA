# Appdetector

Autor: Vratislav Hais, xhaisv00@stud.fit.vutbr.cz
Datum: 16. 11. 2016

Popis: Konzolova aplikace, ktera bude pravidelne kontrolovat seznam aplikaci/sluzeb generujici sitovou zatez. V tomto seznamu bude aplikace
vyhledavat spojeni, ktere generuje nektera aplikace definovana parametrem. Po prefiltrovani seznamu se na definovany syslog server odesle + 
na stdout vypise TCP/UDP petice a nazev aplikace.

Program je spustitelny na operacnim systemu Linux.

Seznam souboru:
	Makefile		- prekladovy soubor
	manual.pdf		- dokumentace
	Readme			- tento soubor
	appdetector.cpp		- zdrojovy kod projektu 

Preklad:
	g++ appdetector.cpp -o appdetector

Spusteni:
	./appdetector -h | --help
	- vypise napovedu

	./appdetector [ -s IPadresa ] [ -i interval ] [ -f filtr ]
	- spusti aplikaci vypisujici vysledek na syslog server udany parametrem -s (+ vypis na stdout)
	- vypisuje v intervalu udanym parametrem -i
	- vypisuje pouze aplikace zadane ve filtru