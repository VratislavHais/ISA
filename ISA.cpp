using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <ctype.h>
#include <vector>

/*
	struktura uchovavajici hodnoty argumentu	
*/

typedef struct arguments {
	string ipAddress;
	int interval;
	string filter;
} ARGS;

/*
	Napoveda programu
*/

void help() {
	cout << "Pouziti:\n";
	cout << "./appdetector [ -s IPadresa ] [ -i interval ] [ -f filter ]\n";
	cout << "-s IPadresa - (povinny parametr) IP adresa syslog serveru, na ktery se odesilaji zpravy. UDP port je vzdy 514.\n";
	cout << "-i interval - (povinny parametr) hodnota v sekundach, jak casto se kontroluje seznam bezicich aplikaci/sluzeb (min. hodn. 1).\n";
	cout << "-f filtr - (povinny parametr) seznam aplikaci, ktere chceme detekovat. Seznam ma tvar app,app,app,... kde app je nazev aplikace skladajici se z tisknutelnych ASCII znaku. Napr \"vlc\" nebo \"vlc,firefox\", ...\n";
}

/*
	funkce overujici, zda se retezec sklada pouze z cisel
*/

bool isNum(string str) {
	for (unsigned int i = 0; i < str.size(); i++) {
		if (!isdigit(str[i])) {
			return false;
		}
	}
	return true;
}

/*
	funkce overujici, zda se retezec sklada pouze z hexadecimalnich znaku (0-9, a-f)
*/

bool isHexa(string str) {
	for (unsigned int i = 0; i < str.size(); i++) {
		if (!isxdigit(str[i])) {
			return false;
		}
	}
	return true;
}

/*
	funkce na kontrolu spravne zadane IPv4
*/

int checkIPv4(string ip) {
	size_t index;
	string tmp;
	int counter = 1;
	while ((index = ip.find(".")) != string::npos) {
		tmp = ip.substr(0, index);
		if (!isNum(tmp) || tmp.length() > 3 || (atoi(tmp.c_str()) > 255)) {
			return EXIT_FAILURE;
		}
		counter++;
		ip.erase(0, index + 1);
	}
	if (!isNum(ip) || ip.length() > 3 || counter != 4 || (atoi(ip.c_str()) > 255)) {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/*
	funkce na kontrolu spravne zadane IPv6
*/

int checkIPv6(string ip) {
	size_t index;
	bool colon = false;
	string tmp;
	int counter = 1;
	if (ip.find(":") == 0) {
		counter++;
		ip = ip.substr(0, 1);
	}
	while ((index = ip.find(":")) != string::npos) {
		tmp = ip.substr(0, index);
		if (tmp.length() == 0) {
			if (!colon) {
				counter++;
				colon = true;
				ip.erase(0, index+1);
				continue;
			}
			else {
				return EXIT_FAILURE;		// vicekrat :: v IPv6
			}
		}
		if (!isHexa(tmp) || tmp.length() > 5) {
			return EXIT_FAILURE;
		}
		ip.erase(0, index + 1);
		counter++;
	}
	if (ip.length() > 0) {
		if (!isHexa(ip) || ip.length() > 5 || counter > 8) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}


/*
	funkce rozhodne, zda se jedna o IPv4 nebo IPv6 a podle toho zavola funkci na kontrolu IP
*/

int checkIp(string ip) {
	if (ip.find(".") != string::npos) {
		if (checkIPv4(ip)) {
			return EXIT_FAILURE;
		}
		else {
			return EXIT_SUCCESS;
		}
	}
	else if (ip.find(":") != string::npos) {
		if (checkIPv6(ip)) {
			return EXIT_FAILURE;
		}
		else {
			return EXIT_SUCCESS;
		}
	}
	else {
		return EXIT_FAILURE;
	}
}

/*
	funkce na zpracovani argumentu
*/

int argParse(int argc, char **argv, ARGS &args) {
	bool sIsSet = false, iIsSet = false, fIsSet = false;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-s")) {
			if (sIsSet) {
				cerr << "Znovupouziti argumentu!\n";
				return EXIT_FAILURE;
			}
			sIsSet = true;
			if (checkIp(argv[++i])) {
				cerr << "Spatne zadana IP adresa!\n";
				return EXIT_FAILURE;
			}
			args.ipAddress = argv[i];
		}
		else if (!strcmp(argv[i], "-i")) {
			if (iIsSet) {
				cerr << "Znovupouziti argumentu!\n";
				return EXIT_FAILURE;
			}
			iIsSet = true;
			if (!isNum(argv[i+1])) {
				cerr << "Interval musi byt cislo!\n";
				return EXIT_FAILURE;
			}
			args.interval = atoi(argv[i+1]);
			i++;
		}
		else if (!strcmp(argv[i], "-f")) {
			if (fIsSet) {
				cerr << "Znovupouziti argumentu!\n";
				return EXIT_FAILURE;
			}
			fIsSet = true;
			args.filter = argv[++i];
			size_t index;
			index = args.filter.find(",");
			// v cyklu se znak carky nahradi za znaky \| pro praci s funkci grep 
			while(index != string::npos) {
				args.filter.replace(index, 1, "\\|");
				index = args.filter.find(",");
			}
		}
		else if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
			help();
			exit(0);
		}
		else {
			cerr << "Chybny argument!\n";
			return EXIT_FAILURE;
		}
	}
	if (!sIsSet || !iIsSet || !fIsSet) {
		cerr << "Nebyl zadan povinny argument\n";
	}
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
	ARGS args;
	if (argParse(argc, argv, args)) {
		return EXIT_FAILURE;
	}
	string command = "lsof -Pnl +M -i | grep ESTABLISHED | grep ";
	command += args.filter;  // pridani filtru na pozadovane aplikace 
	command += " | tr -s \" \" | cut -d\" \" -f 8,9,1 | awk '{ print $2 \" \" $3 \" \" $1}'";  // vyber pozadovanych sloupcu (nazev appky, IP, protokol) + prehazeni v pozadovanem poradi
	command += " | sed -r \"s/\\]|\\[|\\->/ /g\" | ";  // odmazani prebytecnych znaku + rozdeleni portu od IP v IPv6
	command += "sed -r \"s/\\.[[:digit:]]+:/ /g\" | "; // rozdeleni portu od IP v IPv4
	command += "sed -r \"s/ :/ /g\" | tr -s \" \"";
	FILE *in = popen(command.c_str(), "r");
	char buff[512];
	string test = "";
	while(fgets(buff, sizeof(buff), in) != NULL){
        test += buff;
    }
    cout << test;
	return EXIT_SUCCESS;
}