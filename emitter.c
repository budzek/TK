#include "global.h"
#include "parser.h"
using namespace std;

vector<int> argsVector;  //lista argumentow przekazanych wywolaniu metody / procedury

std::stringstream ss(std::ios_base::out | std::ios_base::ate);        //this is a temporary string stream to which we write our code. Unless we set ate flag it would start writing from position 0 after string substitution

map<string, int> specialProcTokenMap = {
    {string(READ_PROC_NAME),    _READ},
    {string(WRITE_PROC_NAME),   _WRITE}
};

//TODO this must be string
map<string, int> operatorTokenMap = {
    {string("+"),   _PLUS},
    {string("-"),   _MINUS},
    {string("*"),   _MUL},
    {string("/"),   _IDIV},
    {string("div"),   _DIV},
    {string("mod"), _MOD},
    {string("and"), _AND},
    {string("="),   _EQ},
    {string(">="),  _GE},
    {string("<="),  _LE},
    {string("<>"),  _NE},
    {string(">"),   _G},
    {string("<"),   _L},
    {string("or"),   OR}
};

map<int, const char*> operationAssemblerMap = {
    {_EQ,   "je"},
    {_NE,   "jne"},
    {_LE,   "jle"},
    {_GE,   "jge"},
    {_G,   "jg"},
    {_L,   "jl"},
    {_REALTOINT,   "realtoint"},
    {_INTTOREAL,   "inttoreal"},
    {_PLUS,   "add"},
    {_MINUS,   "sub"},
    {_MUL,   "mul"},
    {_DIV,   "div"},
    {_IDIV,   "idiv"},
    {_MOD,   "mod"},
    {_AND,   "and"},
    {OR,   "or"},
    {_WRITE,   "write"},
    {_READ,   "read"}
};



/**
 * Return the result type, that the operation on two given variables produces.
 * @param leftVar
 * @param rightVar
 * @return 
 */
int getResultType(int leftVar, int rightVar){
    if(symTable[leftVar].type == REAL || symTable[rightVar].type==REAL) 
        return REAL;
    else 
        return INTEGER;
}

int getToken(string strVal){
    int result = operatorTokenMap.count(strVal) ? operatorTokenMap[strVal] : 0;
    DEBUG << "getToken(" << strVal << ") = " << result << endl;
    return result;
}

int getToken(const char* strVal){
    return getToken(string(strVal));
}


string getAsm(int operation){
    return operationAssemblerMap.count(operation) ? string(operationAssemblerMap[operation]) : "";
}

string getOperationType(int resultVar){
    if(resultVar == UNUSED)
        return "";
    
    if(symTable[resultVar].type==REAL) return ".r ";
    else return ".i";	
}



/**
 * Supposed to handle special procedure - which are handled differently than normal procedures. In our case it's read and write.
 * @param index
 */
void handleSpecialProcedureCall(int index){
    element e = symTable[index];
    if(e.name == WRITE_PROC_NAME || e.name == READ_PROC_NAME){
        for(auto &i : argsVector)
            genCode(specialProcTokenMap[e.name], i, VALUE);
    }
    argsVector.clear();
}

void checkArgs(int index){
    element e  = symTable[index];
    if(e.parameters.size() != argsVector.size()){
        stringstream tempSS; tempSS << "Wrong number of parameters in invocation " << e.name << " expected " << e.parameters.size() << ", found " << argsVector.size(); 
        yyerror(tempSS.str().c_str());
    }
    
    //TODO in fact I could implement type conversion in function invocation as well...
    for(int i = 0; i < argsVector.size(); i++){
        if(symTable[argsVector[i]].type != symTable[e.parameters[i]].type){
            stringstream tempSS; tempSS << "Wrong type of parameter in invocation " << e.name << " in position " << i << " expected " << getTypeString(symTable[e.parameters[i]].type) << ", found " << getTypeString(symTable[argsVector[i]].type); 
            yyerror(tempSS.str().c_str());
        }
    }
}

void pushArgs(){
    for(auto arg : argsVector){
        int indexToPush = arg;
        element e = symTable[arg];
        if(e.token == NUM){
            indexToPush = createNewVar(e.type);
            genCode(ASSIGN, indexToPush, VALUE, UNUSED, UNUSED, arg, VALUE); 
        }
        genCode(_PUSH, indexToPush, symTable[indexToPush].isReference ? AS_IS : ADDRESS);      
    }
}

void callAndCleanUp(int index){
    int incsp = argsVector.size()*REFERENCE_SIZE + (symTable[index].token == FUN ? REFERENCE_SIZE : 0);
    
    genCode(_CALL, index, VALUE);
    if(incsp != 0)
        genCode(_INCSP, incsp);

    argsVector.clear();
}

void handleCall(int index, int resultIndex = UNUSED){
    checkArgs(index);
    pushArgs();
    if(resultIndex != UNUSED) 
        genCode(_PUSH, resultIndex, ADDRESS);  
    callAndCleanUp(index); 
}

/**
 * Handles calling procedure or function without assignment of it's result.
 * @param index
 */
void handleProcedureCall(int index){
    element e = symTable[index];
    if(e.token == _SPECIAL_PROC)
        handleSpecialProcedureCall(index);
    else if(e.token == PROC)
        handleCall(index);
    else if(e.token == FUN)
        handleFunctionCall(index);
}

/**
 * Handles calling function, return index of it's result variable.
 * @param index
 */
int handleFunctionCall(int index){
    element e = symTable[index];
    int result = createNewVar(e.type);  //create variable to store result
    handleCall(index, result);
    return result;
}

int handleBiOperator(int oper, int leftVar, int rightVar){
    handleBiOperator(oper, leftVar, rightVar, getResultType(leftVar, rightVar));
}

int handleBiOperator(int oper, int leftVar, int rightVar, int resultType){
    int resultVar = createNewVar(resultType);
    genCode(oper, resultVar, VALUE, leftVar, VALUE, rightVar, VALUE);
    return resultVar;
}

int handleUnarySign(int sign, int sourceVar){
    if(sign == _PLUS) 
        return sourceVar;
    else {
        return handleBiOperator(sign, lookupOrInsert("0", NUM, symTable[sourceVar].type), sourceVar);
    }
}

int handleRelop(int oper, int leftVar, int rightVar){
    int trueLabel = createLabel();
    int endLabel = createLabel();
    int falseVar = lookupOrInsert("0");
    int trueVar = lookupOrInsert("1");
    int resultVar = createNewVar(INTEGER);
    
    genCode(oper, trueLabel, VALUE, leftVar, VALUE, rightVar, VALUE);
    //condition not met, didn't jump, assign false
    genCode(ASSIGN, resultVar, UNUSED, UNUSED, VALUE, falseVar, VALUE);
    genCode(_JUMP, endLabel);
    //condition met, jumped, assign true
    genCode(_LABEL, trueLabel);
    genCode(ASSIGN, resultVar, UNUSED, UNUSED, VALUE, trueVar, VALUE);
    genCode(_LABEL, endLabel);

    return resultVar;
}

int handleNot(int sourceVar){
    int falseVar = lookupOrInsert("0");
    return handleRelop(_EQ, sourceVar, falseVar); //if x == 0 return 1 else return 0
}

/**
 * Return the formated access to variable. Puts either a value or reference depending on varMode. 
 * Calculates the proper place in memory (with respect to BP value) basing on offset.
 * @param index
 * @param varMode
 */
string getVar(int index, int varMode){
    stringstream tempSS;
    if(symTable[index].token==NUM) tempSS << "#" << symTable[index].name;
    else{
        if(symTable[index].isReference && varMode == VALUE)
            tempSS << "*";
        else if(symTable[index].token==VAR && varMode == ADDRESS)
            tempSS << "#";

        if(symTable[index].scope!=SCOPE_GLOBAL) {
            tempSS << "BP";
            if(symTable[index].offset >=0 )
                tempSS << "+";
        }
        tempSS << symTable[index].offset;
    }
    
    string result = tempSS.str();
    
    DEBUG << "getVar(" << index << ", " << varMode << ") returns " << result << endl;
    
    return result;
}

/**
 * Writes a variable straight to output. Puts either a value or reference depending on varMode. 
 * Calculates the proper place in memory (with respect to BP value) basing on offset.
 * @param index
 * @param varMode
 */
void writeVar(int index, int varMode){
    ss << getVar(index, varMode);
}

/**
 * Unify types, to more accurate type - to real.
 * @param leftVar
 * @param leftVarMode
 * @param rightVar
 * @param rightVarMode
 */
void unifyType(int &leftVar, int leftVarMode, int &rightVar, int rightVarMode){
    int leftType = getElementType(leftVar, leftVarMode);
    int rightType = getElementType(rightVar, rightVarMode);
    if(leftType!=rightType){
        int newVar = createNewVar(REAL);
        if(leftType==INTEGER && rightType==REAL){
            genCode(_INTTOREAL, newVar, leftVarMode, leftVar, leftVarMode, UNUSED, UNUSED);
            leftVar = newVar;
        }
        else if(leftType==REAL && rightType==INTEGER){
            genCode(_INTTOREAL, newVar, rightVarMode, rightVar, rightVarMode, UNUSED, UNUSED);
            rightVar = newVar;
        }
        else{
            yyerror("Type error");
        }
    }
}
/**
 * Unifies type to the type of variable being assigned
 * @param resultVar
 * @param resultVarMode
 * @param rightVar
 * @param rightVarMode
 */
void unifyTypeAndAssign(int resultVar, int resultVarMode, int rightVar, int rightVarMode){
    int resultType = getElementType(resultVar, resultVarMode);
    int rightType = getElementType(rightVar, rightVarMode);
    if(resultType == rightType){
        ss << "\n\tmov" << getOperationType(resultVar) << " "
           << getVar(rightVar,rightVarMode) << "," << getVar(resultVar,resultVarMode);
    }
    else{
        if(resultType==INTEGER && rightType==REAL){
            genCode(_REALTOINT,resultVar, resultVarMode, rightVar, rightVarMode, UNUSED, UNUSED);
        }
        else if(resultType==REAL && rightType==INTEGER){
            genCode(_INTTOREAL,resultVar, resultVarMode, rightVar, rightVarMode, UNUSED, UNUSED);
        }
        else {
            yyerror("Type error");
        }
    }
}

void handleAssign(int resultVar, int sourceVar) {
    //TODO this may need more ensurance
//    if() 
//        symTable[resultVar].name == symTable[resultVar].scope && 
//    int assignMode = symTable[resultVar].isReference ? ADDRESS : VALUE; //if it's a reference it means it's the return value of a function ?? ensure
    int assignMode = VALUE; //if it's a reference it means it's the return value of a function ?? ensure
    genCode(ASSIGN, resultVar, assignMode, UNUSED, UNUSED, sourceVar, VALUE);
}


/**
 * Main function for code generation.
 * @param operand
 * @param resultVar
 * @param resultVarMode
 * @param leftVar
 * @param leftVarMode
 * @param rightVar
 * @param rightVarMode
 */
void genCode(int oper, int resultVar, int resultVarMode, int leftVar, int leftVarMode, int rightVar, int rightVarMode){
    string operationType = getOperationType(resultVar);
    
    if(oper==_LEAVE_RETURN){
        ss << "\n\tleave";
        ss << "\n\treturn";
        
        DEBUG << "Before enter markup substitution:" << endl << ss.str() << endl;
        
        string enterValue = to_string(generateEnterValue());
        string ssCpy = ss.str();
        ssCpy.replace(ssCpy.find(MARKUP_FOR_ENTER_VALUE), string(MARKUP_FOR_ENTER_VALUE).size(), enterValue);
        ss.str(ssCpy);
        
        DEBUG << "After enter markup substitution:" << endl << ss.str() << endl;
    }
    else if(oper>=_EQ && oper<= _L){
        unifyType(leftVar, leftVarMode, rightVar, rightVarMode);
        operationType = getOperationType(leftVar);
        ss << "\n\t" << getAsm(oper) << operationType << " " << getVar(leftVar, leftVarMode) << "," << getVar(rightVar, rightVarMode)  
           << "," << "#" << symTable[resultVar].name;
    }	
    else if(oper==PROC || oper==FUN){
        ss<< "\n" << symTable[resultVar].name << ":";
        ss<< "\n\tenter.i #" << MARKUP_FOR_ENTER_VALUE;
    }
    else if(oper==_LABEL){
        ss << "\n" << symTable[resultVar].name << ":";
    }
    else if(oper==_PUSH){
        ss << "\n\tpush.i " << getVar(resultVar,resultVarMode);
    }
    else if(oper==_INCSP){
        ss << "\n\tincsp" << operationType << " #" << resultVar;
    }
    else if(oper==_JUMP){
        ss <<"\n\tjump.i #" << symTable[resultVar].name;
    }
    else if(oper==_CALL){
        ss <<"\n\tcall.i #" << symTable[resultVar].name;
    }
    else if(oper==_WRITE || oper==_READ){
        ss << "\n\t" << getAsm(oper) << operationType << " " << getVar(resultVar,resultVarMode);
    }
    else if (oper==_REALTOINT || oper==_INTTOREAL){
        ss << "\n\t" << getAsm(oper) << operationType << " " << getVar(leftVar,leftVarMode) << "," << getVar(resultVar,resultVarMode);
    }
    else if(oper == ASSIGN){
        unifyTypeAndAssign(resultVar, resultVarMode, rightVar, rightVarMode);
    }
    else if (oper == _PLUS || oper == _MINUS || oper == _MUL || oper == _DIV ||
             oper == _IDIV || oper == _MOD || oper==_AND || oper == OR){
        unifyType(leftVar, leftVarMode, rightVar, rightVarMode);
        ss << "\n\t" << getAsm(oper) << operationType << " " << getVar(leftVar,leftVarMode) << "," << getVar(rightVar,rightVarMode) 
           << "," << getVar(resultVar,resultVarMode);
    } else {
        cout << "nieznany operator " << oper << endl;
    }
}


void writeFileHeader(){
    stream << "\tjump.i  #" << MAIN_LABEL << endl;
}

/*Writing straight to output*/
void writeToOutput(string str){
    ss << str;
}
void writeToOutput(const char* str){
    ss << str;
}
void writeToOutput(int i){
    ss << i;
}

void writeAllToFile(){
    writeFileHeader();
    stream.write(ss.str().c_str(), ss.str().size());
    ss.str(string());//czyści 
}


void printArrayError() {
    printf("Arrays are not supported\n");
}


void writeToOutputMainLabel(){
    stringstream ss;
    ss << "\n" << MAIN_LABEL << ":";
    writeToOutput(ss.str());
}