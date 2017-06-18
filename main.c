#include "global.h"
#include "parser.h"

using namespace std;
ofstream stream;

int main(int argc, char* argv[]) {
//    yydebug = 1;
    
	FILE *plik_wejsciowy = fopen(argv[1],"r");
	
	if (!plik_wejsciowy) {
            perror("Error");
		printf("Brak pliku\n");
		return -1;
	}
	
	//plik dla leksera
	yyin = plik_wejsciowy;
	
	// ustaw zakres na globalny 
	// otwórz plik wynikowy
	stream.open("output.asm", ofstream::trunc);
	if(!stream.is_open()){
		printf("Nie można utworzyć pliku wynikowego");
		return 0;
	}
                
        insertInitialSymbols();
	
	int result = yyparse();
        
        writeAllToFile();
        
	printSymTable();
        
	stream.close();
	fclose(plik_wejsciowy);
	yylex_destroy();
	return 0;
}

