%{
#include "global.h"
#include "parser.h"
int lineno = 1;
%}

digit 		[0-9]
letter		[a-zA-Z]
relop		"="|"<>"|"<"|"<="|">="|">"
assign		":="
mulop		"*"|"mod"|"/"|"and"|"div"
sign		"+"|"-"	
array		"array"
of		"of"
function	"function"
procedure	"procedure"
integer		"integer"
realWord	"real"
id		{letter}({letter}|{digit})*
int		{digit}+
exponent 	[eE][+-]?{int}
real		{int}("."{int}){exponent}?

%%

[\t | " "]+	
[\n|\r\n]	{lineno++;} 
program		{return PROGRAM;}
begin		{return BEGINN;}
end		{return END;}
var 		{return VAR;}
{integer}	{yylval = INTEGER; return INTEGER;}
{realWord}	{yylval = REAL; return REAL;}
not		{return NOT;}
or		{yylval = getToken(yytext); return OR;}
if		{return IF;}
then		{return THEN;}
else		{return ELSE;}
do		{return DO;}
while		{return WHILE;}
{array}		{return ARRAY;}
{of}		{return OF;}
{function}	{return FUN;}
{procedure}	{return PROC;}
{relop}		{yylval = getToken(yytext); return RELOP;}
{mulop}		{yylval = getToken(yytext); return MULOP;}
{sign}		{yylval = getToken(yytext); return SIGN;}
{assign}	{return ASSIGN;}

{id}		{
                    //yylval = lookupOrInsert(yytext, ID, NONE);
                    yylval = lookupOrInsert(yytext, VAR, NONE); // not sure if it wont cause problems

                    return ID;
		}

{int}		{
                    yylval = lookupOrInsert(yytext, NUM, INTEGER);
                    return NUM;
		}

{real}		{
                    yylval = lookupOrInsert(yytext, NUM, REAL);
                    return NUM;
                }

<<EOF>>		{return DONE;}
.		{return *yytext;}
                /*.		{printf("\nyytext: %s \n", yytext); return *yytext;}*/
%%