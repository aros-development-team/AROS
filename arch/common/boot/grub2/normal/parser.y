/* parser.y - The scripting parser.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

%{
#include <grub/script.h>
#include <grub/mm.h>

#define YYFREE		grub_free
#define YYMALLOC	grub_malloc
#define YYLTYPE_IS_TRIVIAL      0
#define YYENABLE_NLS	0

%}

%union {
  struct grub_script_cmd *cmd;
  struct grub_script_arglist *arglist;
  struct grub_script_arg *arg;
  char *string;
}

%token GRUB_PARSER_TOKEN_IF		"if"
%token GRUB_PARSER_TOKEN_WHILE		"while"
%token GRUB_PARSER_TOKEN_FUNCTION	"function"
%token GRUB_PARSER_TOKEN_MENUENTRY	"menuentry"
%token GRUB_PARSER_TOKEN_ELSE		"else"
%token GRUB_PARSER_TOKEN_THEN		"then"
%token GRUB_PARSER_TOKEN_FI		"fi"
%token GRUB_PARSER_TOKEN_NAME
%token GRUB_PARSER_TOKEN_VAR
%type <cmd> script_init script grubcmd command commands commandblock menuentry if
%type <arglist> arguments;
%type <arg> argument;
%type <string> "if" "while" "function" "else" "then" "fi"
%type <string> text GRUB_PARSER_TOKEN_NAME GRUB_PARSER_TOKEN_VAR

%pure-parser
%lex-param { struct grub_parser_param *state };
%parse-param { struct grub_parser_param *state };

%%
/* It should be possible to do this in a clean way...  */
script_init:	{ state->err = 0} script
		  {
		    state->parsed = $2;
		  }
;

script:		commands { $$ = $1; }
		| function '\n' { $$ = 0; }
		| menuentry '\n' { $$ = $1; }
;

delimiter:	'\n'
		| ';'
		| delimiter '\n'
;

newlines:	/* Empty */
		| newlines '\n'
;

/* Some tokens are both used as token or as plain text.  XXX: Add all
   tokens without causing conflicts.  */
text:		GRUB_PARSER_TOKEN_NAME
		  {
		    $$ = $1;
		  }
		| "if"
		  {
		    $$ = $1;
		  }
		| "while"
		  {
		    $$ = $1;
		  }
;

/* An argument can consist of some static text mixed with variables,
   for example: `foo${bar}baz'.  */
argument:	GRUB_PARSER_TOKEN_VAR
		  {
		    $$ = grub_script_arg_add (state, 0, GRUB_SCRIPT_ARG_TYPE_VAR, $1);
		  }
		| text
		  {
		    $$ = grub_script_arg_add (state, 0, GRUB_SCRIPT_ARG_TYPE_STR, $1);
		  }
/* XXX: Currently disabled to simplify the parser.  This should be
   parsed by yet another parser for readability.  */
/* 		| argument GRUB_PARSER_TOKEN_VAR */
/* 		  { */
/* 		    $$ = grub_script_arg_add ($1, GRUB_SCRIPT_ARG_TYPE_VAR, $2); */
/* 		  } */
/* 		| argument text */
/* 		  { */
/* 		    $$ = grub_script_arg_add ($1, GRUB_SCRIPT_ARG_TYPE_STR, $2); */
/* 		  } */
;

arguments:	argument
		  {
		    $$ = grub_script_add_arglist (state, 0, $1);
		  }
		| arguments argument
		  {
		    $$ = grub_script_add_arglist (state, $1, $2);
		  }
;

grubcmd:	GRUB_PARSER_TOKEN_NAME arguments
		  {
		    $$ = grub_script_create_cmdline (state, $1, $2);
		  }
		| GRUB_PARSER_TOKEN_NAME
		  {
		    $$ = grub_script_create_cmdline (state, $1, 0);
		  }
;

/* A single command.  */
command:	grubcmd delimiter { $$ = $1; }
		| if delimiter 	{ $$ = $1; }
		| commandblock delimiter { $$ = $1; }
		| error delimiter
		  {
		    $$ = 0;
		    yyerror (state, "Incorrect command");
		    state->err = 1;
		    yyerrok;
		  }
;

/* A block of commands.  */
commands:	command
		  {
		    $$ = grub_script_add_cmd (state, 0, $1);
		  }
		| command commands
		  {
		    struct grub_script_cmdblock *cmd;
		    cmd = (struct grub_script_cmdblock *) $2;
		    $$ = grub_script_add_cmd (state, cmd, $1);
		  }
;

/* A function.  Carefully save the memory that is allocated.  Don't
   change any stuff because it might seem like a fun thing to do!
   Special care was take to make sure the mid-rule actions are
   executed on the right moment.  So the `commands' rule should be
   recognized after executing the `grub_script_mem_record; and before
   `grub_script_mem_record_stop'.  */
function:	"function" GRUB_PARSER_TOKEN_NAME
		  { 
		    grub_script_lexer_ref (state->lexerstate);
		  } newlines '{'
		  { 
		    /* The first part of the function was recognized.
		       Now start recording the memory usage to store
		       this function.  */
		    state->func_mem = grub_script_mem_record (state);
		  } newlines commands '}'
		  {
		    struct grub_script *script;

		    /* All the memory usage for parsing this function
		       was recorded.  */
		    state->func_mem = grub_script_mem_record_stop (state,
								   state->func_mem);
		    script = grub_script_create ($8, state->func_mem);
		    if (script)
		      grub_script_function_create ($2, script);
		    grub_script_lexer_deref (state->lexerstate);
		  }
;

/* Carefully designed, together with `menuentry' so everything happens
   just in the expected order.  */
commandblock:	'{'
		  {
		    grub_script_lexer_ref (state->lexerstate);
		  }
                newlines commands '}'
                  {
		    grub_script_lexer_deref (state->lexerstate);
		    $$ = $4;
		  }
;

/* A menu entry.  Carefully save the memory that is allocated.  */
menuentry:	"menuentry" argument
		  {
		    grub_script_lexer_ref (state->lexerstate);
		  } newlines '{'
		  {
		    grub_script_lexer_record_start (state->lexerstate);
		  } newlines commands '}'
		  {
		    char *menu_entry;
		    menu_entry = grub_script_lexer_record_stop (state->lexerstate);
		    grub_script_lexer_deref (state->lexerstate);
		    $$ = grub_script_create_cmdmenu (state, $2, menu_entry, 0);
		  }
;

/* The first part of the if statement.  It's used to switch the lexer
   to a state in which it demands more tokens.  */
if_statement:	"if" { grub_script_lexer_ref (state->lexerstate); }
;

/* The if statement.  */
if:		 if_statement commands "then" newlines commands "fi"
		  {
		    $$ = grub_script_create_cmdif (state, $2, $5, 0);
		    grub_script_lexer_deref (state->lexerstate);
		  }
		 | if_statement commands "then" newlines commands "else" newlines commands  "fi"
		  {
		    $$ = grub_script_create_cmdif (state, $2, $5, $8);
		    grub_script_lexer_deref (state->lexerstate);
		  }
;
