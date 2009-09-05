/* normal_parser.h  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007,2009  Free Software Foundation, Inc.
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

#ifndef GRUB_NORMAL_PARSER_HEADER
#define GRUB_NORMAL_PARSER_HEADER	1

#include <grub/types.h>
#include <grub/err.h>
#include <grub/parser.h>

struct grub_script_mem;

/* The generic header for each scripting command or structure.  */
struct grub_script_cmd
{
  /* This function is called to execute the command.  */
  grub_err_t (*exec) (struct grub_script_cmd *cmd);

  /* The next command.  This can be used by the parent to form a chain
     of commands.  */
  struct grub_script_cmd *next;
};

struct grub_script
{
  struct grub_script_mem *mem;
  struct grub_script_cmd *cmd;
};

typedef enum
{
  GRUB_SCRIPT_ARG_TYPE_STR,
  GRUB_SCRIPT_ARG_TYPE_VAR
} grub_script_arg_type_t;

/* A part of an argument.  */
struct grub_script_arg
{
  grub_script_arg_type_t type;

  char *str;

  /* Next argument part.  */
  struct grub_script_arg *next;
};

/* A complete argument.  It consists of a list of one or more `struct
   grub_script_arg's.  */
struct grub_script_arglist
{
  struct grub_script_arglist *next;
  struct grub_script_arg *arg;
  /* Only stored in the first link.  */
  int argcount;
};

/* A single command line.  */
struct grub_script_cmdline
{
  struct grub_script_cmd cmd;

  /* The arguments for this command.  */
  struct grub_script_arglist *arglist;
};

/* A block of commands, this can be used to group commands.  */
struct grub_script_cmdblock
{
  struct grub_script_cmd cmd;

  /* A chain of commands.  */
  struct grub_script_cmd *cmdlist;
};

/* An if statement.  */
struct grub_script_cmdif
{
  struct grub_script_cmd cmd;

  /* The command used to check if the 'if' is true or false.  */
  struct grub_script_cmd *exec_to_evaluate;

  /* The code executed in case the result of 'if' was true.  */
  struct grub_script_cmd *exec_on_true;

  /* The code executed in case the result of 'if' was false.  */
  struct grub_script_cmd *exec_on_false;
};

/* A menu entry generate statement.  */
struct grub_script_cmd_menuentry
{
  struct grub_script_cmd cmd;

  /* The arguments for this menu entry.  */
  struct grub_script_arglist *arglist;

  /* The sourcecode the entry will be generated from.  */
  const char *sourcecode;

  /* Options.  XXX: Not used yet.  */
  int options;
};

/* State of the lexer as passed to the lexer.  */
struct grub_lexer_param
{
  /* Set to 0 when the lexer is done.  */
  int done;

  /* State of the state machine.  */
  grub_parser_state_t state;

  /* Function used by the lexer to get a new line when more input is
     expected, but not available.  */
  grub_reader_getline_t getline;

  /* A reference counter.  If this is >0 it means that the parser
     expects more tokens and `getline' should be called to fetch more.
     Otherwise the lexer can stop processing if the current buffer is
     depleted.  */
  int refs;

  /* The character stream that has to be parsed.  */
  char *script;
  char *newscript; /* XXX */

  /* While walking through the databuffer, `record' the characters to
     this other buffer.  It can be used to edit the menu entry at a
     later moment.  */

  /* If true, recording is enabled.  */
  int record;

  /* Points to the recording.  */
  char *recording;

  /* index in the RECORDING.  */
  int recordpos;

  /* Size of RECORDING.  */
  int recordlen;

  /* The token that is already parsed but not yet returned. */
  int tokenonhold;

  /* Was the last token a newline? */
  int was_newline;
};

/* State of the parser as passes to the parser.  */
struct grub_parser_param
{
  /* Keep track of the memory allocated for this specific
     function.  */
  struct grub_script_mem *func_mem;

  /* When set to 0, no errors have occurred during parsing.  */
  int err;

  /* The memory that was used while parsing and scanning.  */
  struct grub_script_mem *memused;

  /* The result of the parser.  */
  struct grub_script_cmd *parsed;

  struct grub_lexer_param *lexerstate;
};

struct grub_script_arglist *
grub_script_create_arglist (struct grub_parser_param *state);

struct grub_script_arglist *
grub_script_add_arglist (struct grub_parser_param *state,
			 struct grub_script_arglist *list,
			 struct grub_script_arg *arg);
struct grub_script_cmd *
grub_script_create_cmdline (struct grub_parser_param *state,
			    struct grub_script_arglist *arglist);
struct grub_script_cmd *
grub_script_create_cmdblock (struct grub_parser_param *state);

struct grub_script_cmd *
grub_script_create_cmdif (struct grub_parser_param *state,
			  struct grub_script_cmd *exec_to_evaluate,
			  struct grub_script_cmd *exec_on_true,
			  struct grub_script_cmd *exec_on_false);

struct grub_script_cmd *
grub_script_create_cmdmenu (struct grub_parser_param *state,
			    struct grub_script_arglist *arglist,
			    char *sourcecode,
			    int options);

struct grub_script_cmd *
grub_script_add_cmd (struct grub_parser_param *state,
		     struct grub_script_cmdblock *cmdblock,
		     struct grub_script_cmd *cmd);
struct grub_script_arg *
grub_script_arg_add (struct grub_parser_param *state,
		     struct grub_script_arg *arg,
		     grub_script_arg_type_t type, char *str);

struct grub_script *grub_script_parse (char *script,
				       grub_reader_getline_t getline);
void grub_script_free (struct grub_script *script);
struct grub_script *grub_script_create (struct grub_script_cmd *cmd,
					struct grub_script_mem *mem);

struct grub_lexer_param *grub_script_lexer_init (char *s,
						 grub_reader_getline_t getline);
void grub_script_lexer_ref (struct grub_lexer_param *);
void grub_script_lexer_deref (struct grub_lexer_param *);
void grub_script_lexer_record_start (struct grub_lexer_param *);
char *grub_script_lexer_record_stop (struct grub_lexer_param *);

/* Functions to track allocated memory.  */
struct grub_script_mem *grub_script_mem_record (struct grub_parser_param *state);
struct grub_script_mem *grub_script_mem_record_stop (struct grub_parser_param *state,
						     struct grub_script_mem *restore);
void *grub_script_malloc (struct grub_parser_param *state, grub_size_t size);

/* Functions used by bison.  */
union YYSTYPE;
int grub_script_yylex (union YYSTYPE *, struct grub_parser_param *);
int grub_script_yyparse (struct grub_parser_param *);
void grub_script_yyerror (struct grub_parser_param *, char const *);

/* Commands to execute, don't use these directly.  */
grub_err_t grub_script_execute_cmdline (struct grub_script_cmd *cmd);
grub_err_t grub_script_execute_cmdblock (struct grub_script_cmd *cmd);
grub_err_t grub_script_execute_cmdif (struct grub_script_cmd *cmd);
grub_err_t grub_script_execute_menuentry (struct grub_script_cmd *cmd);

/* Execute any GRUB pre-parsed command or script.  */
grub_err_t grub_script_execute (struct grub_script *script);

/* This variable points to the parsed command.  This is used to
   communicate with the bison code.  */
extern struct grub_script_cmd *grub_script_parsed;



/* The function description.  */
struct grub_script_function
{
  /* The name.  */
  char *name;

  /* The script function.  */
  struct grub_script *func;

  /* The flags.  */
  unsigned flags;

  /* The next element.  */
  struct grub_script_function *next;

  int references;
};
typedef struct grub_script_function *grub_script_function_t;

grub_script_function_t grub_script_function_create (struct grub_script_arg *functionname,
						    struct grub_script *cmd);
void grub_script_function_remove (const char *name);
grub_script_function_t grub_script_function_find (char *functionname);
int grub_script_function_iterate (int (*iterate) (grub_script_function_t));
int grub_script_function_call (grub_script_function_t func,
			       int argc, char **args);

char *
grub_script_execute_argument_to_string (struct grub_script_arg *arg);

#endif /* ! GRUB_NORMAL_PARSER_HEADER */
