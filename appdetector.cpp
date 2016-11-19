using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <ctype.h>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

/*
	struktura uchovavajici hodnoty argumentu	
*/

typedef struct arguments {
	string ipAddress;
	int interval;
	string filter;
	bool ipv4;
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

/**
 *	funkce overujici, zda se retezec sklada pouze z cisel
 *  @str - string obsahujici retezec, ktery testujeme zda obsahuje pouze cislice 
**/

bool isNum(string str) {
	for (unsigned int i = 0; i < str.size(); i++) {
		if (!isdigit(str[i])) {
			return false;
		}
	}
	return true;
}

/**
 *	funkce overujici, zda se retezec sklada pouze z hexadecimalnich znaku (0-9, a-f)
 *	@str - string obsahujici retezec, ktery testujeme zda obsahuje pouze hexadecimalni znaky 
**/

bool isHexa(string str) {
	for (unsigned int i = 0; i < str.size(); i++) {
		if (!isxdigit(str[i])) {
			return false;
		}
	}
	return true;
}

/**
 *	funkce na kontrolu spravne zadane IPv4
 *	@ip - string obsahujici IP adresu, u ktere budeme kontrolovat validitu
**/

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

/**
 *	funkce na kontrolu spravne zadane IPv6
 *	@ip - string obsahujici IP adresu, u ktere budeme kontrolovat validitu
**/

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


/**
 *	funkce rozhodne, zda se jedna o IPv4 nebo IPv6 a podle toho zavola funkci na kontrolu IP
 *	@ip - string, ktery obsahuje IP adresu, u ktere se bude kontrolovat validita
 *	@args - struktura, do ktere ulozime, zda se jedna o IPv4 nebo IPv6
**/

int checkIp(string ip, ARGS &args) {
	if (ip.find(".") != string::npos) {
		if (checkIPv4(ip)) {
			return EXIT_FAILURE;
		}
		else {
			args.ipv4 = true;
			return EXIT_SUCCESS;
		}
	}
	else if (ip.find(":") != string::npos) {
		if (checkIPv6(ip)) {
			return EXIT_FAILURE;
		}
		else {
			args.ipv4 = false;
			return EXIT_SUCCESS;
		}
	}
	else {
		return EXIT_FAILURE;
	}
}

/**
 *	funkce na zpracovani argumentu
 *	@argc - pocet zadanych argumentu
 *	@argv - pole obsahujici jednotlive argumenty
 *	@args - struktura, do ktere budeme ukladat hodnoty argumentu
**/

int argParse(int argc, char **argv, ARGS &args) {
	bool sIsSet = false, iIsSet = false, fIsSet = false;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-s")) {
			if (sIsSet) {
				cerr << "Znovupouziti argumentu!\n";
				return EXIT_FAILURE;
			}
			sIsSet = true;
			if (checkIp(argv[++i], args)) {
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
				cout << args.filter;
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
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/**
 *	funkce na vytvoreni socketu
 *	@ipstr - string obsahujici ip adresu
 *	@sock - ukazatel na promennou vytvorenou ve funkci main, do ktere se ulozi socket
 *	@ipv4 - booleovska promenna, ktera nabyva true, pokud se jedna o IPv4
**/

int connection(string ipstr, int *sock, bool ipv4) {
	struct in_addr ip;
	struct sockaddr_in sockaddr;
	struct hostent *host;
	if (ipv4) {
		*sock = socket(AF_INET, SOCK_STREAM, 0);
	}
	else {
		*sock = socket(AF_INET6, SOCK_STREAM, 0);
	}
	if (*sock <= 0) {
		cerr << "Chyba inicializace socketu\n";
		return EXIT_FAILURE;
	}
	if (ipv4) {
		if (!inet_pton(AF_INET, ipstr.c_str(), &ip)) {
			cerr << "Nelze parsovat IP\n";
		}
	}
	else {
		if (!inet_pton(AF_INET6, ipstr.c_str(), &ip)) {
			cerr << "Nelze parsovat IP\n";
		}
	}
	if (ipv4) {
		sockaddr.sin_family = AF_INET;
	}
	else {
		sockaddr.sin_family = AF_INET6;
	}
	sockaddr.sin_port = htons(514);
	if (ipv4) {
		host = gethostbyaddr((const void*)&ip, sizeof ip, AF_INET);
	}
	else {
		host = gethostbyaddr((const void*)&ip, sizeof ip, AF_INET6);
	}
	if (host == NULL) {
			cerr << "Neco se porouchalo\n";
	}
	memcpy (&sockaddr.sin_addr, host->h_addr, host->h_length);
	if ((connect(*sock, (const struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0)) {
		cerr << "chyba pripojeni\n";
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/**
 *	ze stringu vybere 1 radek, ktery z puvodniho stringu vymaze a vrati jako navratovou hodnotu
 *	@str - string obsahujici vysledek operace lsof
**/

string getLine(string *str) {
	size_t index;
	string line;
	index = (*str).find("\n");
	if (index != string::npos) {
		line = (*str).substr(0, index);
	}
	else {
		line = (*str);
	}
	(*str).erase(0, index+1);
	return line;
}

/**
 *  funkce na overeni, zda se jedna o pripojeni localhostu
 *  @str - string obsahujici radek s podrobnosti o pripojeni
**/

bool isLocalhost(string str) {
	size_t found;
	found = str.find("127.0.0.1");
	if (found != string::npos) {
		return true;
	}
	else {
		return false;
	}
}

int main(int argc, char *argv[]) {
	cout << "here";
	int sock, resIndex, index, toSend;
	char buff[512];
	bool found;
	string str = "";
	ARGS args;
	vector<string> result;		// pole na ukladani vysledku. Pomoci tohoto pole se bude kontrolovat, ktere pripojeni jiz bylo vypsano
	vector<string> toProcess;	// pole na ukladani vysledku prikazu v promenne command
	FILE *in;
	if (argParse(argc, argv, args)) {
		return EXIT_FAILURE;
	}
	if (connection(args.ipAddress, &sock, args.ipv4)) {
		return EXIT_FAILURE;
	}
	string command = "lsof -Pnl +M -i | grep ESTABLISHED | grep \"";
	command += args.filter;  // pridani filtru na pozadovane aplikace 
	command += "\" | tr -s \" \" | cut -d\" \" -f 8,9,1 | awk '{ print $2 \" \" $3 \" \" $1}'";  // vyber pozadovanych sloupcu (nazev appky, IP, protokol) + prehazeni v pozadovanem poradi
	command += " | sed -r \"s/\\]|\\[|\\->/ /g\" | ";  // odmazani prebytecnych znaku + rozdeleni portu od IP v IPv6
	command += "sed -r \"s/(\\.[[:digit:]]+):/\\1 /g\" | "; // rozdeleni portu od IP v IPv4
	command += "sed -r \"s/ :/ /g\" | tr -s \" \" | uniq -u";
	while (1) {
		in = popen(command.c_str(), "r");
		while(fgets(buff, sizeof(buff), in) != NULL){
	        str += buff;
	    }
	    while (strcmp(str.c_str(), "")) {
	    	toProcess.push_back(getLine(&str));
	    }
	    resIndex = result.size() - 1;
	    toSend = result.size();
	    if (resIndex < 0) {	// prvni pruchod nebo v predchozim pruchodu nebylo zadne spojeni => muzeme naplnit pole bez jakekoliv kontroly
	    	for (int i = 0; i < toProcess.size(); i++) {
	    		if (isLocalhost(toProcess[i])) {
	    			continue;
	    		}
	    		result.push_back(toProcess[i]);
	    	}
	    }
	    else {
	    	for (; resIndex >= 0; resIndex--) {
	    		found = false;
	    		for (index = 0; index < toProcess.size(); index++) {
	    			if (result[resIndex].compare(toProcess[index]) == 0 || isLocalhost(toProcess[index])) {
	    				toProcess.erase(toProcess.begin() + index);
	    				index--;
	    				found = true;
	    			}
	    		}
	    		if (!found) {		// proslo se cele pole a prave kontrolovana komunikace nebyla nalezena => byla ukoncena
	    			result.erase(result.begin() + resIndex);
	    			toSend--;
	    		}
	    	}
	    	for (int i = 0; i < toProcess.size(); i++) {	// nyni vlozime zaznamy o nove komunikaci
	    		result.push_back(toProcess[i]);
	    	}
	    }
	    // vypis
	    for (; toSend < result.size(); toSend++) {
	    	if ((send(sock, result[toSend].c_str(), result[toSend].length(), 0)) < 0) {
				cerr << "chyba odesilani zpravy\n";
				return EXIT_FAILURE;
			}
	    	cout << result[toSend] << endl;
	    }
	    sleep(args.interval);
	}
	return EXIT_SUCCESS;
}