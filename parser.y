%{  
    #define YYDEBUG 1
    #include "global.h"
    using namespace std;







%}

%token 	PROGRAM
%token 	BEGINN
%token 	END
%token 	VAR
%token 	INTEGER
%token  REAL
%token	ARRAY
%token 	OF
%token	FUN
%token 	PROC
%token	IF
%token	THEN
%token	ELSE
%token	DO
%token	WHILE
%token 	RELOP
%token 	MULOP
%token 	SIGN
%token 	ASSIGN
%token	OR
%token 	NOT
%token 	ID
%token 	NUM
%token 	NONE
%token 	DONE

%%
start:      PROGRAM{
                isProgramHeaderDeclaration = true;
            } 
            ID '(' start_params ')' ';'{
                isProgramHeaderDeclaration = false;
                isGlobalDeclaration = true;
            }
            declarations {
                processGlobalDeclarations();
                isGlobalDeclaration = false;
            }
            subprogram_declarations {writeToOutputMainLabel();}
            compound_statement '.'  {writeToOutput("\n\texit\n");}	
            eof
    ;

start_params:   ID
            |   start_params ',' ID
    ;

identifier_list:    ID {
                        processIdentifier($1);
                    }
               |    identifier_list  ',' ID  {
                        processIdentifier($3);
                    }
    ;


declarations:   declarations VAR identifier_list ':' type {
                    processIdentifiersListInDeclaration($5);
                }
                ';' 
            | /*EMPTY SYMBOL*/
    ;

type:   standard_type
    |   ARRAY '[' NUM '.' '.' NUM ']' OF standard_type {printArrayError();YYERROR;}
    ;

standard_type:  INTEGER 
             |  REAL
    ;

subprogram_declarations: /*EMPTY SYMBOL*/
                        |    subprogram_declarations subprogram_declaration ';'
    ;

subprogram_declaration:     subprogram_head{
                                isLocalDeclaration = true;
                            } 
                            declarations{
                                processLocalDeclarations();
                                isLocalDeclaration = false;
                            }
                            compound_statement {
                                genCode(_LEAVE_RETURN);
                                setCurrentScope(SCOPE_GLOBAL);
                            }
    ;

subprogram_head:    FUN {
                        isSubprogramNameDeclaration = true;
                    }
                    ID {
                        isSubprogramNameDeclaration = false;
                        symTable[$3].token = FUN;
                        setCurrentScope($3); 
                        genCode(FUN, $3, VALUE);
                    }
                    arguments {
                        setArgumentsOffset($3, FUNC_OFFSET);
                    }
                    ':' standard_type {
                        symTable[$3].type = $8;
                        insertReturnValueVariable($3, $8);
                    }
                    ';' 
               |    PROC {
                        isSubprogramNameDeclaration = true;
                    }
                    ID {
                        isSubprogramNameDeclaration = false;
                        symTable[$3].token = PROC;
                        setCurrentScope($3); 
                        genCode(PROC, $3, VALUE);
                    }
                    arguments {
                        setArgumentsOffset($3, PROC_OFFSET);
                    }  ';'
    ;

arguments:  '('{
                isArgumentList = true;
            }
            parameter_list
            ')'{
                isArgumentList = false;
            }
        | /*EMPTY SYMBOL*/
    ;   

parameter_list:     identifier_list  ':' type {
                        processIdentifiersListInFuncProcArguments($3);
                    }
                |   parameter_list ';' identifier_list  ':' type {
                        processIdentifiersListInFuncProcArguments($5);
                    }
    ;

compound_statement: BEGINN optional_statement END
    ;

optional_statement: statement_list
                  | /*EMPTY SYMBOL*/
    ;

statement_list: statement
              | statement_list ';' statement
    ;

statement:  variable {
                $1 = trySubstituteForFunctionReturnAddress($1); 
            }
            ASSIGN simple_expression {
                handleAssign($1, $4);     
            }
         |  procedure_statement
         |  compound_statement
         |  IF expression {
                int elseLabel = createLabel();
                genCode(_EQ, elseLabel, VALUE, $2, VALUE, lookupOrInsert("0"), VALUE);
                $$ = elseLabel;
            }
            THEN statement {
                int endLabel = createLabel();
                genCode(_JUMP, endLabel);
                $$ = endLabel;
            }
            ELSE {
                genCode(_LABEL, $3); // elseLabel
            }
            statement {
                genCode(_LABEL, $6); // endLabel
            }
         |  WHILE {
                int checkLabel = createLabel();
                genCode(_LABEL, checkLabel); // checkLabel
                $$ = checkLabel;
            }
            expression {
                int endLabel = createLabel();
                genCode(_EQ, endLabel, VALUE, $3, VALUE, lookupOrInsert("0"), VALUE); // if x == 0 -> jump endLabel
                $$ = endLabel;
            }
            DO statement {
                genCode(_JUMP, $2); // jump checkLabel
                genCode(_LABEL, $4); // endLabel
            }
    ;

variable:   ID 
        |   ID '[' simple_expression ']' {printArrayError();YYERROR;}
    ;


procedure_statement:    ID { 
                            handleProcedureCall($1);
                        }
                    |   ID '(' expression_list ')' {
                            handleProcedureCall($1);
                        }
    ;

expression_list:    expression {
                        argsVector.push_back($1);
                    }
               |    expression_list ',' expression{
                        argsVector.push_back($3);
                    }  
    ;

expression:     simple_expression 
          |     simple_expression RELOP simple_expression {
                    $$ = handleRelop($2, $1, $3);
                }
    ;

simple_expression:  term
                 |  SIGN term {
                        $$ = handleUnarySign($1, $2);
                    }
                 |  simple_expression SIGN term {
                        $$ = handleBiOperator($2, $1, $3);
                    }
                 |  simple_expression OR term {
                        $$ = handleBiOperator($2, $1, $3, INTEGER);
                    }
    ;

term:   factor
    |   term MULOP factor {
            $$ = handleBiOperator($2, $1, $3);
        }
    ;

factor:     variable {
                if(symTable[$1].token == FUN)
                    yyerror("function on the RHS must be used with parenthesis");
            }
        |   ID '(' expression_list ')' {
                $$ = handleFunctionCall($1);
            } 	
        |   NUM
        |   '(' expression ')' {
                $$=$2;
            }
        |   NOT factor {
                $$ = handleNot($2);
            }
    ;

eof:    DONE{return 0;}
    ;   

%%
void yyerror(char const *s) {
    printf("Błąd w linii %d: %s \n",lineno, s);
}

void prinArrayError() {
    printf("Arrays are not supported\n");
}


