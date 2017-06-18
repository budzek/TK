#include "global.h"
#include "parser.h"
using namespace std;
vector<element> symTable;
string currentScope = string(SCOPE_GLOBAL);        //holds the name of the current scope

int currentFunctionDeclarationIndexST;             //holds the ST index of the function being currently declared
bool isArgumentList = false;                       //set to true by parser if we are processing the argument list of func/proc declaration. Otherwise it's false and it means we are processing (local) variables declaration.
bool isProgramHeaderDeclaration = false;  
bool isGlobalDeclaration = false;  
bool isSubprogramNameDeclaration = false;  
bool isLocalDeclaration = false;  
vector<int> identifierList;
vector<int> declarationList;



int counterGeneratedVariables=0;
int counterGeneratedLabels=1;

/*utilities*/
map<int, const char*> tokenDescriptorMap = {
    {_SPECIAL_PROC,      "SPECIAL_PROC"},
    {PROC,      "PROC"},
    {FUN,       "FUN"},
    {_LABEL,    "LABEL"},
    {ID,        "ID"},
    {VAR,       "VAR"},
    {NUM,       "NUM"},
    {INTEGER,   "INTEGER"},
    {REAL,      "REAL"},
    {NONE,      "NONE"}
};

map<int, const char*> typeDescriptorMap = {
    {INTEGER,   "INTEGER"},
    {REAL,      "REAL"},
//    {REFERENCE, "REFERENCE"},
    {NONE,      "NONE"}
};


//wstawia wpis do ST
int insert (const char* s, int token, int type) {
    element e; 
    e.token = token;        // typ tokenu (VAR, NUM, FUN, PROC)
    e.name = string(s);     //nazwa
    e.scope = currentScope;
    e.type = type;	
    e.offset=0;
    e.index = symTable.size();
    symTable.push_back(e);

    DEBUG << "insert: " << e << ", isLocalDeclaration: " << isLocalDeclaration << endl;

    return e.index;
}

/**
 * Return true if we are in a scope, where unknown names ma appear.
 * @return 
 */
bool isDeclarationScope(){
    return isProgramHeaderDeclaration|| isGlobalDeclaration || isSubprogramNameDeclaration || isLocalDeclaration || isArgumentList;
}

int lookup(const char* s){
    int p = lookupInScope(s, currentScope);
    if(p == -1)
        p = lookupInScope(s, SCOPE_GLOBAL);
    return p;
}

int lookupInScope(const char* eName, string scopeName){
    for(int p=symTable.size()-1; p>=0; p--){
        element e = symTable[p];
        if(e.name == eName && e.scope == scopeName)
            return p;
    }
    return -1;
}

int lookupOrInsert (const char* s){
    return lookupOrInsert(s, NUM, INTEGER);
}

int lookupOrInsert(const char* s, int token, int type){
    //the name of function or procedure can't be reused for variable name
    int p = lookupInScope(s, SCOPE_GLOBAL);
    if(p != -1 && (symTable[p].token==FUN || symTable[p].token==PROC || symTable[p].token==_SPECIAL_PROC))
        return p;
     
    p = lookupInScope(s, currentScope); 
    if (p == -1){
        if(isDeclarationScope()){
            p = insert (s, token, type);
        } else {
            p = lookupInScope(s, SCOPE_GLOBAL);
            if (p == -1){
                if(token == VAR){
                    yyerror("Odniesienie do niezdefiniowanej nazwy");
                } else {
                    p = insert (s, token, type);
                }
                       
            }
        }
    }
    return p;
}

void insertReturnValueVariable(int funcIndexST, int type){
    int p = insert(symTable[funcIndexST].name.c_str(), VAR, type);
    symTable[p].isReference = true;
    symTable[p].offset = 8;
}

int lookupFunctionReturnVariable(int index){
    element eFun = symTable[index];
    for(auto &e : symTable){
        if(e.scope == eFun.name && e.name == eFun.name)
            return e.index;
    }
    
    return -1;
}

/**
 * If index references the function in ST, return this function return variable, otherwise return the passed index.
 * @param index
 * @return 
 */
int trySubstituteForFunctionReturnAddress(int index){
    if(symTable[index].token == FUN){
        if(currentScope == symTable[index].name)
            return lookupFunctionReturnVariable(index);
        else
            yyerror("function used as variable");
    } else 
        return index;
}


/**
 * Creates a new temporary variable in ST and assigns it an offset.
 * @param type
 * @return 
 */
int createNewVar(int type){
    string varName = string("$t") + to_string(counterGeneratedVariables++);
    int id = insert(varName.c_str(), VAR, type);
    symTable[id].offset = generateNewVariablePosition(type);
    return id;
}

/**
 * Return the type of a passed variable found in ST under index. If we request an address it's always a reference in our architecture.
 * @param index
 * @param varMode
 * @return 
 */
int getElementType(int index, int varMode){
    if(varMode == ADDRESS) 
        return INTEGER;
    else 
        return symTable[index].type;
}

int getTypeSize(int type) {
    if (type == INTEGER) return INTEGER_SIZE;
    else if (type == REAL) return REAL_SIZE;
}

int getElementSize(element e){
    if(e.token == VAR){	
        return getTypeSize(e.type);
    }
    else if (e.isReference) 
        return REFERENCE_SIZE;

    return 0;
}

bool offsetCopmare(element a, element b){
    return a.offset < b.offset;
}

/**
 * Depending on current scope returns the index of variable with the minimal (if local scope) or maximal (if global scope).
 * @return 
 */
int findMinMaxOffsetIndex(){
    int minMaxOffsetIndex = -1; // last element of current scope in ST
    for(int i=0;i<symTable.size();i++){
        element e = symTable[i];
        if(e.scope == currentScope && 
                ((currentScope == SCOPE_GLOBAL && e.offset > symTable[minMaxOffsetIndex].offset) 
                ||(currentScope != SCOPE_GLOBAL && e.offset < symTable[minMaxOffsetIndex].offset)))
            minMaxOffsetIndex = i;   
    }
    return minMaxOffsetIndex;
}

/**
 * Calculates the next free space in memory offset.
 * @return 
 */
int generateNewVariablePosition(int type){
    int minMaxOffsetIndex = findMinMaxOffsetIndex();
    
    int offset = (minMaxOffsetIndex >= 0 ? symTable[minMaxOffsetIndex].offset : 0);
    int lastElementSize = getElementSize(symTable[minMaxOffsetIndex]);
    int newElementSize = getTypeSize(type);
    
    int result = offset + (currentScope == SCOPE_GLOBAL ? lastElementSize : -newElementSize);
    
    DEBUG << "generated offset is: " << result <<" currentScope = " << currentScope << ", previous element index = " << minMaxOffsetIndex << ", previous element size = " << lastElementSize << ", new element size = " << newElementSize << endl;
    
    return result;
}

/**
 * returns the lowest offset in current scope * (-1) => the value is used for enter.i invocation
 * @return 
 */
int generateEnterValue(){
    int minMaxOffsetIndex = findMinMaxOffsetIndex();
    int result = symTable[minMaxOffsetIndex].offset;
    
    DEBUG << "generated enter value is: " << result <<" currentScope = " << currentScope << ", previous element index = " << minMaxOffsetIndex << endl;
    
    return result * -1; // we assume that it's used only in local scope, never global
}

int createLabel(){
	stringstream s;
	s << "lab" << counterGeneratedLabels++;
	int id = insert(s.str().c_str(), _LABEL, NONE);
	return id;
}

void processArgument(int indexST){
    symTable[indexST].isReference = true;
    identifierList.push_back(indexST);
}

void processDeclaration(int indexST){
    identifierList.push_back(indexST);
    declarationList.push_back(indexST);
    
}

void processIdentifier(int indexST){
    if(isArgumentList)
        processArgument(indexST);
    else
       processDeclaration(indexST); 
}

void setTypeForIdentifiersList(int type){
    for (auto &indexID : identifierList)
        symTable[indexID].type = type;
}

//void setTypeForIdentifiersListInParams(int type){
//    setTypeForIdentifiersList(type);
//    addIdentifiersListToFuncProcParams();    
//}

void processIdentifiersListInDeclaration(int type){
    setTypeForIdentifiersList(type);
    identifierList.clear();    
}

void processIdentifiersListInFuncProcArguments(int type){
    setTypeForIdentifiersList(type);
    addIdentifiersListToFuncProcParams();
}


void addIdentifiersListToFuncProcParams(){
    /*copy list of identifier to the params of a function*/
    vector<int> *params = &symTable[currentFunctionDeclarationIndexST].parameters;
    params->insert(params->end(), identifierList.begin(), identifierList.end());
    identifierList.clear();
}

void processGlobalDeclarations() {
    int offset = 0;
    for (auto i : declarationList){
        symTable[i].offset = offset;
        offset += getElementSize(symTable[i]);
    }
    declarationList.clear();
}

void processLocalDeclarations(){
    int offset = 0;
    for (auto i : declarationList){
        offset -= getElementSize(symTable[i]);
        symTable[i].offset = offset;
    }
    declarationList.clear();
}

/**
 * sets the offset for function / procedure argument in regard to BP - base pointer
 * @param indexST index of function / procedure in ST
 * @param offset initial offset in respect to BP
 */
void setArgumentsOffset(int indexST, int offset) {
    vector<int> *params = &symTable[indexST].parameters;

    for (vector<int>::reverse_iterator i = params->rbegin(); i != params->rend(); ++i ) {
        symTable[*i].offset = offset;
        offset += 4;
    }
}


void insertInitialSymbols(){
    insert(MAIN_LABEL, _LABEL, NONE);
    insert(WRITE_PROC_NAME, _SPECIAL_PROC, NONE);
    insert(READ_PROC_NAME, _SPECIAL_PROC, NONE);
}

void setCurrentScope(const char* name){
    currentScope = string(name);
    DEBUG << "current scope set to: " << currentScope << endl;
    
    if(name == SCOPE_GLOBAL)
        currentFunctionDeclarationIndexST = -1; //we are not declaring any function right now
}

/**
 * Sets the current scope, to the name of the element located in symbol table under given index.
 * All new entries in ST, created by the insert function will have their scope set to this name value.
 * @param indexST
 */
void setCurrentScope(int indexST){
    setCurrentScope(symTable[indexST].name.c_str());
    
    currentFunctionDeclarationIndexST = indexST; // if we are setting current scope, it means we are in the function / procedure definition;
}

string getTokenString(int tokenID){
    return tokenDescriptorMap.count(tokenID) ? string(tokenDescriptorMap[tokenID]) : to_string(tokenID);
}

string getTypeString(int typeID){
    return typeDescriptorMap.count(typeID) ? string(typeDescriptorMap[typeID]) : to_string(typeID);
}

void printSymTable(){
    printf("\nSymbol table\n");	
    
    cout << setw(3) << "i" << setw(8) << "index" << setw(10) << "scope" << setw(15) << "token" 
             << setw(10) << "name" << setw(10) << "type" << setw(7) << "offset" 
             << setw(13) << "isReference" << setw(20)  << "parameters" << endl;
    
    for(int i=0;i<symTable.size(); i++){
        element e = symTable[i];
        stringstream params;
        for (auto i: e.parameters)
            params << i << ',';        
        
        cout << setw(3) << i << setw(8) << e.index << setw(10) << e.scope << setw(15) << getTokenString(e.token) 
             << setw(10) << e.name << setw(10) << getTokenString(e.type) << setw(7) << e.offset 
             << setw(13) << (e.isReference ? "REF" : "VAR")  << setw(20)  << params.str() << endl;
    }
}