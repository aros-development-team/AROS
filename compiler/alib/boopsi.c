/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

/*******************************************************************************

    MODUL
	boopsi.c

    DESCRIPTION
	Implementiert BOOPSI - Basic Object Oriented Programming System
	for Intuition. BOOPSI gestattet C++-Methoden in Standard ANSI
	bzw K&R-C zu verwenden. Die Vorgehensweise ist wie folgt:

	    BOOPSI arbeitet mit sog. Message-Dispatchern. Ein Message
	Dispatcher ist im Prinzip nichts anderes als eine Routine, die
	Nachrichten an ein Object auswertet. Diese Nachricht ist nichts
	anderes als eine Structur mit ID. Die ID bestimmt den Typ der
	Nachricht (z.B. Setzte Attribute des Objects).

	    Der Vorteil von BOOPSI liegt darin, dass die Zugriffsfunktionen
	alle extern zum Object und standardisiert sind. Der Benutzer eines
	Object muss sich nicht mehr viele verschiedene Funktionsnamen merken
	sondern kann mit fuenf Funktionen alle Operationen auf Objecte
	durchfuehren. Diese Operationen sind:

		- Erzeuge ein neues Object (eine neue Instanz des Objects)
		- Loese ein Object auf
		- Setze Attribute des Objects
		- Frage Attribute des Objects ab
		- Wende eine Methode auf das Object an.

    NOTES

    BUGS

    TODO

    EXAMPLES
	// Beispiel eines kompletten Line-Objects

	// Hier wird die Instance-Data definiert. Der Speicher fuer diese
	// Struktur wird vom Betriebssystem angelegt und verwaltet.
	struct LineData
	{
	    FLOAT coords[4];
	    ULONG width;
	    ULONG style;
	};

	// In einem Include-File sind folgende Tags definiert:
	//
	//	LINE_StartX, (FLOAT, CSG)
	//	LINE_StartY, (FLOAT, CSG)
	//	LINE_EndX, (FLOAT, CSG)
	//	LINE_EndY, (FLOAT, CSG)
	//	LINE_Width, (INT, CSG)
	//	LINE_Style, (INT, CSG)
	//
	// und folgende Methoden
	//
	//	LINEM_Display, (GC)
	//
	// Mit diesen lassen sich die einzelnen Attribute des Objects aendern

	// Initialisierung und Bekanntmachung des Objects
	Class * initLineClass P((void))
	{
	    Class * cl;

	    // Neue Klasse mit dem Namen "lineclass" bekanntgeben und
	    // verschiedene Default-Werte initialisieren.
	    if (cl = MakeClass (NULL,
			"lineclass", NULL,
			sizeof (struct LineData),
			0))
	    {
		// Dispatcher eintragen. Unter UNIX muss bei h_Entry immer
		// HookEntry stehen.
		cl->cl_Dispatcher.h_Entry    = HookEntry;   // always for UNIX
		cl->cl_Dispatcher.h_SubEntry = dispatchLineClass;
	    }
	} // initLineClass


	// Klasse wieder entfernen. Das muss man vor dem Verlassen
	// des Programms tun, weil zB. unter AmigaOS alle Public-Objecte
	// global bekannt sind und deshalb andere Programme u.U. noch
	// auf das Object zugreifen.
	BOOL freeLineClass (cl)
	Class * cl;
	{
	    return (FreeClass (cl));
	} // freeLineClass


	// Hier folgt jetzt der interessante Teil: Ein BOOSPI-Dispatcher.
	// GETA4 ist unter UNIX leer, unter AmigaOS wird damit die lokale
	// Umgebung (zB. Variablen initialisiert
	__geta4 ULONG dispatchLineClass (cl, o, msg)
	REG(A0) Class  * cl;
	REG(A2) Object * o;
	REG(A1) Msg      msg;
	{
	    struct LineData * inst;		// instance data
	    APTR	      retval = NULL;	// generic return value

	    // Die Bedeutung von retval haengt von der verwendeten Methode
	    // ab. ZB. ist es bei OM_GET ein Boolean-Wert, bei OM_NEW aber
	    // ein Zeiger auf das neue Object

	    switch (msg->MethodID)  // Welche Methode ?
	    {
	    case OM_NEW:    // Zuerst wird die Message nach "oben"
			    // weitergegeben damit die Superclass
			    // zuerst alles Einrichten kann (zB.
			    // Speicher besorgen).

		if (retval = (APTR)DoSuperMethodA (cl, o, msg))
		{
		    // Bei der OM_NEW-Methode zeigt der Object-Pointer nicht
		    // auf ein Object (wie auch ??). DoSuperMethod() gibt
		    // einen Zeiger auf das neu erzeugte Object zurueck.
		    // INST_DATA() ist ein Macro aus <intuition/classes.h>
		    // welches einen Zeiger auf die Instance-Data des Objects
		    // zurueckliefert. Dies ist hier die LineData-Struktur
		    // von oben.
		    //
		    //	  Beachten Sie, dass der Speicher in keiner Weise
		    // vorinitialisiert wird !

		    inst = INST_DATA(cl,retval);

		    // Jetzt koennen wir alle Felder initalisieren. Das muss
		    // so getan werden, dass man gefahrlos ALLE Methoden auf
		    // das Object anwenden kann.

		    inst->Width = 0;	// Breite = 0 -> Nichts tun

		    // Wenn wir hier etwas komplizierteres machen wollten
		    // (zB. Kind-Objecte erzeugen) waere das ohne weiteres
		    // moeglich. Aber dann muss bei einem Fehler
		    // DoMethod (retval, OM_DISPOSE); aufgerufen werden !
		}

		// Nur abbrechen, wenn kein Object erzeugt werden konnte.
		// Sonst sollten wir noch die Start-Attribute abfragen !
		if (!retval)
		    break;

		// Fuer weitere Schritte sollte <o> schon auf einen
		// sinnvollen Wert zeigen :-)

		o = (Object *)retval;

	    case OM_SET:    // Mit OM_SET kann man Attribute setzen.
	    case OM_UPDATE: // Wie OM_SET, nur kommt es von einem anderen
			    // BOOPSI-Object !

		// Zuerst wieder die SuperClass ranlassen (nur, wenn nicht
		// OM_NEW) damit diese zuerst alle ihr wichtigen Attribute
		// bearbeiten kann.

		if (msg->MethodID != OM_NEW)
		    retval = DoSuperMethodA (cl, o, msg);

		// Jetzt sind wir dran

		{   // Fuer lokale Variable
		    struct TagItem * ti, * tstate;

		    // Instance-Data raussuchen

		    inst = INST_DATA(cl, o);

		    // Tag-Liste untersuchen und Attribute kopieren
		    for (ti=tstate=((struct opSet *)msg)->ops_AttrList);
			    ti; ti = NextTagItem (&tstate))
		    {
			// Hier alle unbekannten Tags einfach ignorieren
			// weil sie wahrscheinlich zu einer der Superklassen
			// gehoeren
			switch (ti->ti_Tag)
			{
			case LINE_StartX:
			    *(&inst->coords[0]) = *(FLOAT *)ti->ti_Data;
			    break;

			case LINE_StartY:
			    *(&inst->coords[1]) = *(FLOAT *)ti->ti_Data;
			    break;

			case LINE_EndX:
			    *(&inst->coords[2]) = *(FLOAT *)ti->ti_Data;
			    break;

			case LINE_EndY:
			    *(&inst->coords[3]) = *(FLOAT *)ti->ti_Data;
			    break;

			case LINE_Width:
			    inst->Width = ti->ti_Data;
			    break;

			case LINE_Style:
			    inst->Style = ti->ti_Data;
			    break;

			}
		    } // for
		} // local block

		break; // OM_SET, OM_UPDATE

	    case OM_GET: // ein (1!) Attribut lesen
		inst = INST_DATA(cl, o);

		// Erst mal auf TRUE setzen -> Attribut gefunden.
		// Wenn es ein Line-Attribut ist, wird es auf alle
		// Faelle gefunden, wenn es ein Attribut der Superclass
		// ist, wird der Wert nochmals bei DoSuperMethodA()
		// gesetzt.

		retval = (APTR)TRUE;

		switch (((struct opGet *)msg)->opg_AttrID)
		{
		case LINE_StartX:
		    *((FLOAT *)(((struct opGet *)msg)->opg_Storage)) =
			    inst->coords[0];
		    break;

		// usw. usf.

		default:
		    // Kein Attribut von uns ?? Dann lassen wir mal die
		    // Superclass ran
		    retval = DoSuperMethodA (cl, o, msg);
		}

		break;	 // OM_GET

	    default: // Unbekannte Methode ? Vielleicht kann ja die
		     // Superclass was damit anfangen !

		// Hier wird in unserem Beispiel OM_DISPOSE behandelt.
		// Sollten wir eine kompliziertere Version von OM_DISPOSE
		// benoetigen, weil wir zB. Kinder freizugeben haben,
		// muessten wird das VOR dem Aufruf von DoSuperMethodA()
		// machen, weil danach der Zugriff auf das Object nicht
		// mehr erlaubt ist.

		retval = DoSuperMethodA (cl, o, msg);
		break;

	    } // switch

	    // Ergebnis zurueck
	    return (retval);
	} // dispatchLineClass

    SEE ALSO

    INDEX

    HISTORY
	14.09.93    ada created

*******************************************************************************/

/**************************************
		Includes
**************************************/
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef CLIB_ALIB_PROTOS_H
#   include <clib/alib_protos.h>
#endif
#include <stdarg.h>


static ULONG CallHookPkt (struct Hook * hook, APTR object, APTR paramPacket)
{
    return ((*(hook->h_Entry)) (hook, object, paramPacket));
}

/******************************************************************************

    NAME */
	ULONG DoMethodA (

/*  SYNOPSIS */
	Object * obj,
	Msg	 message)

/*  FUNCTION
	Wendet eine Methode auf ein BOOPSI-Object an. Dazu wird der Dispatcher
	fuer die Klasse, der das Object angehoert aufgerufen. Die Methoden,
	welche ein Object unterstuetzt, werden auf einer Klasse-fuer-Klasse
	Basis definiert.

    INPUTS
	obj - Das Object, auf welches sich die Operation bezieht.
	message - Die Method-Message. Das erste ULONG der Message definiert den
		Typ, der Rest haengt von der Klasse ab.

    RESULT
	Der Rueckgabewert haengt von der Methode ab. Bei OM_NEW ist es z.B. ein
	Zeiger auf das neu generierte Object; andere Methoden verwenden andere
	Ergebnis-Werte. Diese werden bei der Beschreibung der Klasse definiert
	und sind dort nachzulesen.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	NewObject(), SetAttrs(), GetAttr(), DisposeObject(), DoSuperMethod(),
	"Basic Object-Oriented Programming System for Intuition" und das
	"boopsi Class Reference" Dokument.

    HISTORY:
	14.09.93    ada created

******************************************************************************/
{
    return (CallHookPkt ((struct Hook *)OCLASS(obj), obj, message));
} /* DoMethodA */


ULONG DoMethod (Object * obj, ULONG MethodID, ...)
{
    va_list args;
    ULONG   retval;

    va_start (args, MethodID);

    retval = (CallHookPkt ((struct Hook *)OCLASS(obj), obj, (Msg)&MethodID));

    va_end (args);

    return (retval);
} /* DoMethod */


/******************************************************************************

    NAME
	DoSuperMethodA -- Sende eine Message an die SuperClass eines Objects

    SYNOPSIS
	retval = DoSuperMethodA (cl, obj, message)

	ULONG DoSuperMethodA (Class *, Object * obj, Msg message);

	retval = DoSuperMethod (class, obj, MethodID, ...)

	ULONG DoSuperMethod (Class *, Object * obj, ULONG MethodID, ...);

    FUNCTION
	Sendet eine BOOPSI-Message an ein BOOPSI-Object als ob dieses eine
	Instanz seiner SuperKlasse waere.

    INPUTS
	cl - Class des Objects.
	obj - Das Object, auf welches sich die Operation bezieht.
	message - Die Method-Message. Das erste ULONG der Message definiert den
		Typ, der Rest haengt von der Klasse ab.

    RESULT
	Der Rueckgabewert haengt von der Methode ab. Bei OM_NEW ist es z.B. ein
	Zeiger auf das neu generierte Object; andere Methoden verwenden andere
	Ergebnis-Werte. Diese werden bei der Beschreibung der Klasse definiert
	und sind dort nachzulesen.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	NewObject(), SetAttrs(), GetAttr(), DisposeObject(), DoMethod(),
	"Basic Object-Oriented Programming System for Intuition" und das
	"boopsi Class Reference" Dokument.

    HISTORY:
	14.09.93    ada created

******************************************************************************/

ULONG DoSuperMethodA (cl, obj, message)
Class  * cl;
Object * obj;
Msg	 message;
{
    return (CallHookPkt ((struct Hook *)cl->cl_Super, obj, message));
} /* DoSuperMethodA */


ULONG DoSuperMethod (Class * cl, Object * obj, ULONG MethodID, ...)
{
    va_list args;
    ULONG   retval;

    va_start (args,MethodID);

    retval = DoSuperMethodA (cl, obj, (Msg)&MethodID);

    va_end (args);

    return (retval);
} /* DoSuperMethod */


#ifdef TODO
/******************************************************************************

    NAME
	GetAttrsA

    SYNOPSIS
	GetAttr (object, tags)

	void GetAttr (Object *, struct TagItem *);

    FUNCTION
	Fragt bei dem angegebenen Objekt die angegebenen Attribut ab.

	Im ti_Data_feld der TagItem-Elemente werden Zeiger auf Langworte
	erwartet. In diese wird der Wert des Attributs geschrieben.

	Nicht alle Attribute werden auf diese Attribute reagieren. Welche
	das es tun steht bei der Dokumentation der Klasse.

    INPUTS
	object - Das Objekt ueber dessen Attribut wir uns informieren wollen
	tags - Gesuchtes Attribut und hier wird die Antwort reingeschrieben.

    RESULT
	Keines.

    NOTES
	Diese Funktion ruft die OM_GET-Methode fuer ein Objekt auf.

    EXAMPLE

    BUGS

    SEE ALSO
	NewObject(), DisposeObject(), SetAttr(), GetAttrs(), MakeClass(),
	"Basic Object-Oriented Programming System for Intuition" und das
	"boopsi Class Reference" Dokument.

    HISTORY:
	02.12.93    ada created

******************************************************************************/

void GetAttrsA (object, tags)
Object * object;
struct TagItem * tags;
{
    struct TagItem * ti, * tstate;

    /* Fuer alle Attribute in der Liste GetAttr() aufrufen */
    for (ti=tstate=tags; ti; ti = NextTagItem (&tstate))
    {
	GetAttr (ti->ti_Tag, object, (ULONG *) ti->ti_Data);
    }
} /* GetAttrsA */


void GetAttrs (Object * obj, ...)
{
    va_list	     args;

    va_start (args, obj);

    GetAttrsA (obj, (struct TagItem *)args);

    va_end (args);
} /* GetAttrs */
#endif


/*******************************************************************************
*****  ENDE boopsi.c
*******************************************************************************/
