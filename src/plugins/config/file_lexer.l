/***************************************************************************
 *   Copyright (C) 2005 by Robert Timothy Stewart                          *
 *   tims@cc.gatech.edu                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

%{
#undef yywrap

#include <string.h>
#include <netvizd.h>
#include "file_parser.h"

int lineno = 1;
%}

%option stack

%x string
%x generic

ws		[ \t]+
word	[A-Za-z_][A-Za-z0-9_]*
int		[0-9]+
text	[^"\n]*

crap	[^A-Za-z_0-9;#\{\}\" \t\n]+

%%

{ws}			{ }
\n				{ lineno++; }
#.*				{ }

global			{ return GLOBAL; }
plugin			{ return PLUGIN; }
type			{ return TYPE; }
file			{ return FILEE; }
storage			{ return STORAGE; }
sensor			{ return SENSOR; }
proto			{ return PROTO; }
auth			{ return AUTH; }
data_set		{ return DATA_SET; }
counter			{ return COUNTER; }
derive			{ return DERIVE; }
absolute		{ return ABSOLUTE; }
gauge			{ return GAUGE; }
yes				{ return YES; }
no				{ return NO; }
system			{ return SYSTEM; }
description		{ return DESC; }
cf				{ return CF; }
average			{ return AVERAGE; }
min				{ return MIN; }
max				{ return MAX; }
last			{ return LAST; }

;				{ return SEMI; }
\{				{ return LBRACE; }
\}				{ return RBRACE; }

\"				{ yy_push_state(string); }

{int}			{ yylval.ch = strdup(yytext); return INT; }
{word}			{ yylval.ch = strdup(yytext); return WORD; }

{crap}			{ return CRAP; }

<string>\"		{ yy_pop_state(); }
<string>{text}	{ yylval.ch = strdup(yytext); return STRING; }

<generic>{ws}	{ }
<generic>\n		{ lineno++; }
<generic>#.*	{ }
<generic>yes	{ yylval.ch = strdup(yytext); return YES; }
<generic>no		{ yylval.ch = strdup(yytext); return NO; }
<generic>;		{ return SEMI; }
<generic>\"		{ yy_push_state(string); }
<generic>{word}	{ yylval.ch = strdup(yytext); return WORD; }
<generic>{int}	{ yylval.ch = strdup(yytext); return INT; }
<generic>\}		{ yy_pop_state(); }
<generic>{crap}	{ return CRAP; }

%%

int yywrap() {
	return 1;
}

void push_generic() {
	yy_push_state(generic);
}

void pop_generic() {
	yy_pop_state();
}

void yyerror(const char *msg) {
	fflush(stdout);
	nv_log(LOG_INFO, "line %d: %s at \"%s\"", lineno, msg, yytext);
}

/* vim: set ts=4 sw=4: */
