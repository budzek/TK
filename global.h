#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <utility>
#include <list>
#include <map>

using namespace std;

//#define _DEBUG

#ifdef _DEBUG
#define DEBUG cout // or any other ostream
#else
#define DEBUG 0 && cout
#endif

/*This defines our architecture*/
#define FUNC_OFFSET 12
#define PROC_OFFSET 8

#define INTEGER_SIZE 4
#define REAL_SIZE 8
#define REFERENCE_SIZE INTEGER_SIZE

/*This defines our internal representation*/
#define SCOPE_GLOBAL "global"
#define MAIN_LABEL "main"

#define MARKUP_FOR_ENTER_VALUE "??"
#define WRITE_PROC_NAME "write"
#define READ_PROC_NAME "read"

#define ADDRESS 0   //represents a parameter used as an adress for code generation
#define VALUE 1     //represents a parameter used as a value for code generation
#define AS_IS -2    //don't make it a reference and don't dereference it -> this is used in func / proc invocation when we push a reference as an argument
#define UNUSED -1   //represents an unsued parameter


// wpis w tablicy symboli
struct element{
    int index;              //index in symTable, added in the late dev stage, just for convenience use in auto loops
    string name;            //nazwa lub numer (dla liczb)
    string scope;           //nazawa zakresu - dla elementow typu FUN i PROC, a take zmiennych globalnych jest to GLOBAL, w przeciwnym razie nazwa procedury / funkcji
    int token;              //typ tokenu
    int type;               //rodzaj wartości int/real, dla funkcji rodzaj wartosci zwracanej
    int offset;
    vector<int> parameters; // typy kolejnych parametrów (funkcja/procedura)
    bool isReference = false;
       
};

extern int yydebug; // ustawione na 1 w main.c generuje debug log parsera
int yylex();//odpala lekser
int yylex_destroy();//zabija parser
int yyparse();//odpala parser
void yyerror(char const* s);//funkcja błędu parsera

/*symbol table*/
extern vector<element> symTable;    //ST
extern string currentScope;
extern bool isArgumentList;         //set to true by parser if we are processing the argument list of func/proc declaration. Otherwise it's false and it means we are processing (local) variables declaration.
extern bool isProgramHeaderDeclaration;         
extern bool isGlobalDeclaration;         
extern bool isSubprogramNameDeclaration;         
extern bool isLocalDeclaration;         
extern vector<int> argsVector;       //lista argumentow przekazanych wywolaniu metody / procedury


extern vector<int> localVariables;  //lista id w tablicy symboli zmiennych lokalnych dla danej funckji / procedury

int lookup(const char* s);                                  //szuka w obecnym zakresie, jesli nie ma, to w globalnym
int lookupInScope(const char* eName, string scopeName);     //szuka w podanym zakresie
int lookupOrInsert (const char* s);                         //jesli element jest w ST, zwroc jego index, jesli nie dodaj i zwroc index token = NUM, type = INTEGER
int lookupOrInsert (const char* s, int token, int type);    //jesli element jest w ST, zwroc jego index, jesli nie dodaj i zwroc index
int insert (const char* s, int token, int type);//wrzuca do TS
void insertInitialSymbols();                                //dodaj do ST prodecury write, read oraz label glownej procedury
int createNewVar(int type);//tworzy dodatkową zmienną dla obliczeń to "t" z zajęć
int createLabel();//tworzy kolejny label do skoku
int getElementSize(element e);//podaje rozmiar elementu

void setCurrentScope(const char* name);            //sets the currentScope to a passed name
void setCurrentScope(int indexST);                 //sets the currentScope to symTable[indexST].name

void processIdentifier(int indexST);               //is it's function declaration process function arguments, otherwise declare local variables
void setTypeForIdentifiersList(int type);          //the identifiers of a given type have been loaded as part of function parameter, set their type and add them to the list of method parameters in ST
void addIdentifiersListToFuncProcParams();         //adds the current identifier list to the func/proc params and cleans the list

void processIdentifiersListInDeclaration(int type);
void processIdentifiersListInFuncProcArguments(int type);
void insertReturnValueVariable(int funcIndexST, int type); //wstawia do ST zmienna, ktora odpowiada wartosci zwracanej funkcji
int trySubstituteForFunctionReturnAddress(int index);   //Return function return variable or the variable

int getElementType(int index, int varMode);        //Return the type of a passed variable found in ST under index


void processGlobalDeclarations();
void processLocalDeclarations();


void setArgumentsOffset(int indexST, int offset);  //sets offset for the parameters of a function located in ST under given offset

int generateResultType(int a, int b);   //generuje typ wyniku dla 2 operandów
int generateNewVariablePosition(int type);      //calculates offset for a new variable in a current scope
int generateEnterValue();            //calculates highest / lowest offset in currentScope
    
void clearLocalVariables();


/*emitter*/
extern int lineno;//nr linii 
extern ofstream stream;//stream do zapisu
extern FILE* yyin;//plik wejściowy dla leksera
extern stringstream ss;        //this is a temporary string stream to which we write our code

#define PRINT_SS DEBUG << "ss:" << endl << ss.str() << endl;


int getToken(const char*);//pobiera przydzielony token na podstawie tekstu operacji np >= otrzyma token GE
void writeAllToFile();
void writeIntToOutput(int);
void writeToOutput(const char* s);//zapisuje bezpośrednio do pliku
void writeToOutput(string s);//zapisuje bezpośrednio do pliku
void genCode(int oper, int resultVar = UNUSED, int resultVarMode = UNUSED, int leftVar = UNUSED, int leftVarMode = UNUSED, int rightVar = UNUSED, int rightVarMode = UNUSED);

void handleAssign(int resultVar, int sourceVar);
void handleProcedureCall(int index);
int handleFunctionCall(int index);
int handleBiOperator(int oper, int leftVar, int rightVar);
int handleBiOperator(int oper, int leftVar, int rightVar, int resultType);
int handleUnarySign(int sign, int sourceVar);
int handleRelop(int oper, int leftVar, int rightVar);
int handleNot(int sourceVar);

void writeToOutputMainLabel();

/*utilities*/
void printArrayError();                  //wypisuje blad o uzyciu tablic  
void printSymTable();                   //drukuje tablicę symboli
string getTokenString(int tokenID);     //dla printSymTable
string getTypeString(int typeID);


inline std::ostream& operator<<(std::ostream &strm, const element &e) {
    return strm << "element[index: " << e.index << " name: " << e.name << ", scope: " << e.scope << ", token: " << getTokenString(e.token) 
            << ", type: " << getTypeString(e.type) 
//          << ", offset: " << e.offset << ", isReference: " << e.isReference 
//          << ", parameters: " << e.type << ", returnType: " 
            << "]";
}


//tokeny które mogą wystąpić 
#define _LABEL 1257
#define _WRITE 1259
#define _READ 1260
#define _PUSH 1261
#define _INCSP 1262
#define _PLUS 1264
#define _MINUS 1265
#define _MUL 1266
#define _IDIV 1267
#define _DIV 1267
#define _MOD 1268
#define _AND 1269
#define _INTTOREAL 1270
#define _REALTOINT 1271
#define _CALL 1272
#define _LEAVE_RETURN 1274 
#define _EQ 1275
#define _NE 1276
#define _GE 1277
#define _LE 1278
#define _G 1279
#define _L 1280
#define _JUMP 1281
#define _SPECIAL_PROC 1282

#endif