#include <unistd.h>
#include <toollib/error.h>
#include <toollib/stdiocb.h>
#include <toollib/stringcb.h>
#include "html.h"
#include "parse.h"
#include "var.h"

int
HTML_Filter (HTMLTag * tag, MyStream * in, MyStream * out, CBD data)
{
    HTMLTagArg * cmdarg;
    String	 cmdstr;
    char       * fn;
    char       * body;
    FILE       * fh;
    FILE       * cmdoutput;
    char       * cmdoutputname;
    int ProcessInput;
    int ExpandInput;
    int InputFile;
    int ProcessOutput;

    cmdarg = (HTMLTagArg *) FindNodeNC (&tag->args, "CMD");

    if (!cmdarg)
    {
	Str_PushError (in, "Missing argument CMD in FILTER");
	return T_ERROR;
    }

    if (!cmdarg->value)
    {
	Str_PushError (in, "Missing value for argument CMD in FILTER");
	return T_ERROR;
    }

    Var_PushLevel ();

    ProcessInput  = (FindNodeNC (&tag->args, "PROCESSINPUT")  != NULL);
    ExpandInput   = (FindNodeNC (&tag->args, "EXPANDINPUT")   != NULL);
    InputFile	  = (FindNodeNC (&tag->args, "INPUTFILE")     != NULL);
    ProcessOutput = (FindNodeNC (&tag->args, "PROCESSOUTPUT") != NULL);

    fn = xstrdup (tmpnam (NULL));

    if (!ProcessInput)
    {
	fh = fopen (fn, "w");

	if (!fh)
	{
	    PushStdError ("Can open %s for writing", fn);
	    xfree (fn);
	    Var_FreeLevel (Var_PopLevel ());
	    return T_ERROR;
	}

	body = Var_Get ("@body");

	if (body)
	{
	    if (ExpandInput)
	    {
		String ebody = Var_Subst (body);

		if (!ebody)
		{
		    Str_PushError (in, "Can't expand body");
		    return T_ERROR;
		}

		fputs (ebody->buffer, fh);
		VS_Delete (ebody);
	    }
	    else
		fputs (body, fh);
	}

	fclose (fh);
    }
    else
    {
	StdioStream  * ppout = StdStr_New (fn, "w");
	StringStream * ppin;
	int rc;

	if (!ppout)
	{
	    PushStdError ("Can open %s for writing", fn);
	    xfree (fn);
	    Var_FreeLevel (Var_PopLevel ());
	    return T_ERROR;
	}

	body = Var_Get ("@body");

	if (body)
	{
	    String ebody = NULL;

	    if (ExpandInput)
	    {
		ebody = Var_Subst (body);

		if (!ebody)
		{
		    Str_PushError (in, "Can't expand body");
		    return T_ERROR;
		}

		ppin = StrStr_New (ebody->buffer);
	    }
	    else
		ppin = StrStr_New (body);

	    rc = HTML_Parse ((MyStream *)ppin, (MyStream *)ppout, data);

	    StrStr_Delete (ppin);
	    if (ebody)
		VS_Delete (ebody);
	}
	else
	    rc = T_OK;

	StdStr_Delete (ppout);

	if (rc != T_OK)
	{
	    PushStdError ("Failed to prepare the input for the filter", cmdarg->value);
	    xfree (fn);
	    Var_FreeLevel (Var_PopLevel ());
	    return T_ERROR;
	}
    }

    if (InputFile)
    {
	Var_Set ("infilename", fn);
    }

    cmdoutputname = xstrdup (tmpnam (NULL));

    cmdstr = Var_Subst (cmdarg->value);

    Var_FreeLevel (Var_PopLevel ());

    if (!cmdstr)
    {
	Str_PushError (in, "Can expand CMD argument for FILTER");
	xfree (fn);
	xfree (cmdoutputname);
	return T_ERROR;
    }

    if (!execute (cmdstr->buffer, "", InputFile ? "-" : fn, cmdoutputname))
    {
	Str_PushError (in, "Error running filter %s", cmdarg->value);
	unlink (fn);
	unlink (cmdoutputname);
	xfree (fn);
	xfree (cmdoutputname);
	VS_Delete (cmdstr);
	return T_ERROR;
    }

    unlink (fn);
    xfree (fn);
    VS_Delete (cmdstr);

    if (ProcessOutput)
    {
	StdioStream * ss = StdStr_New (cmdoutputname, "r");
	int rc;

	if (!ss)
	{
	    PushStdError ("Can't open file %s", cmdoutputname);
	    unlink (cmdoutputname);
	    xfree (cmdoutputname);
	    return T_ERROR;
	}

	rc = HTML_Parse ((MyStream *)ss, out, data);

	StdStr_Delete (ss);

	if (rc != T_OK)
	{
	    PushStdError ("Failed to process the output of the filter", cmdarg->value);
	    unlink (cmdoutputname);
	    xfree (cmdoutputname);
	    return T_ERROR;
	}
    }
    else
    {
	int c;

	cmdoutput = fopen (cmdoutputname, "r");

	if (!cmdoutput)
	{
	    PushStdError ("Can open %s for reading", cmdoutputname);
	    unlink (cmdoutputname);
	    xfree (cmdoutputname);
	    return T_ERROR;
	}

	while ((c = getc (cmdoutput)) != EOF)
	    Str_Put (out, c, data);
    }

    unlink (cmdoutputname);
    xfree (cmdoutputname);

    return T_OK;
}

