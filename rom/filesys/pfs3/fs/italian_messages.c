/* $Id$ */
/* $Log: messages.c $
 * Revision 1.9  1999/03/09  10:31:35  Michiel
 * 00110: definizione numero di release
 *
 * Revisione 1.8  1998/10/02  07:22:45  Michiel
 * release finale versione 4.2
 *
 * Revision 1.7  1998/09/27  11:26:37  Michiel
 * aggiunti ERRORE_ANODE e ANODE_INIT
 *
 * Revision 1.6  1998/09/03  07:12:14  Michiel
 * versione 17.4
 * risoluzione errori 118, 121, 123 e supporto di superindexblocks td64
 *
 * Revision 1.5  1998/05/31  16:30:38  Michiel
 * versione numero
 *
 * Revision 1.4  1998/05/30  21:52:34  Michiel
 * Aggiunte in ritardo
 *
 * Revision 1.3  1998/05/27  20:16:13  Michiel
 * AFS --> PFS2
 *
 * Revision 1.2  1997/11/19  22:14:15  mark
 * Copyright trasferito da FLD a GREED
 * Cambio nome da AFS in PFS-II
 *
 * Revision 1.1  1997/03/03  22:04:04  Michiel
 * Revisione Iniziale
 * */

#include <exec/types.h>
#include "versionhistory.doc"

UBYTE AFS_WARNING_MEMORY_MASK[]         = "AVVERTIMENTO:\nLa memoria allocata non corrisponde alla memoria mask";
UBYTE AFS_ERROR_DNV_ALLOC_INFO[]        = "AVVISO:\nInfo allocato non trovato";
UBYTE AFS_ERROR_DNV_ALLOC_BLOCK[]       = "AVVISO:\nBlocco allocato non trovato";
UBYTE AFS_ERROR_DNV_WRONG_ANID[]        = "AVVISO:\nAblock id errato";
UBYTE AFS_ERROR_DNV_WRONG_DIRID[]       = "AVVISO:\nDirblock id errato";
UBYTE AFS_ERROR_DNV_LOAD_DIRBLOCK[]     = "AVVISO:\nNon  possibile leggere il directoryblock";
UBYTE AFS_ERROR_DNV_WRONG_BMID[]        = "AVVISO:\nId del bitmap block id errato";
UBYTE AFS_ERROR_DNV_WRONG_INDID[]       = "AVVISO:\nId indice block errato";
UBYTE AFS_ERROR_CACHE_INCONSISTENCY[]   = "Incoerenza dati rilevata nella Cache\nBlocco di tutte le attivit del disco";
UBYTE AFS_ERROR_OUT_OF_BUFFERS[]        = "Mancanza buffers";
UBYTE AFS_ERROR_MEMORY_POOL[]           = "Non si possono allocare gruppi di memoria";
UBYTE AFS_ERROR_PLEASE_FREE_MEM[]       = "Per piacere liberare pi memoria";
UBYTE AFS_ERROR_LIBRARY_PROBLEM[]       = "Non  possibile aprire le librerie!";
UBYTE AFS_ERROR_INIT_FAILED[]           = "Fallimento nell'inizializzazione";
UBYTE AFS_ERROR_READ_OUTSIDE[]          = "Tentativo di lettura oltre il bordo della partizione!";
UBYTE AFS_ERROR_WRITE_OUTSIDE[]         = "Tentativo di scrittura oltre il bordo ella partizione!";
UBYTE AFS_ERROR_SEEK_OUTSIDE[]          = "Tentativo di ricerca oltre il bordo della partizione!";
UBYTE AFS_ERROR_READ_ERROR[]            = "Errore di lettura %ld nel blocco %ld\n"
                                          "Assicurarsi che il disco sia inserito";
UBYTE AFS_ERROR_WRITE_ERROR[]           = "Errore di scrittura %ld nel blocco %ld\n"
                                          "Assicurarsi che il disco sia inserito";
UBYTE AFS_ERROR_READ_DELDIR[]           = "Non  possibile leggere la deldir";
UBYTE AFS_ERROR_DELDIR_INVALID[]        = "Deldir non valida";
UBYTE AFS_ERROR_EXNEXT_FAIL[]           = "ExamineNext fallito";
UBYTE AFS_ERROR_DOSLIST_ADD[]           = "Aggiunto errore DosList.\nPer favore rimuovere il volume";
UBYTE AFS_ERROR_EX_NEXT_FAIL[]          = "ExamineNext fallito";
UBYTE AFS_ERROR_NEWDIR_ADDLISTENTRY[]   = "Fallimento con newdir e addlistentry";
UBYTE AFS_ERROR_LOAD_DIRBLOCK_FAIL[]    = "Non  possibile caricare il dirblock!!";
UBYTE AFS_ERROR_LRU_UPDATE_FAIL[]       = "Aggiornamento di LRU fallito";
UBYTE AFS_ERROR_UPDATE_FAIL[]           = "Aggiornamento del disco fallito";
UBYTE AFS_ERROR_UNSLEEP[]               = "Risveglio fallito";
UBYTE AFS_ERROR_DISK_TOO_LARGE[]        = "Disco troppo grande per questa versione di PFS3.\nPer favore installare TD64 o la versione direct-scsi";
UBYTE AFS_ERROR_ANODE_ERROR[]           = "Indice Anode non valido";
UBYTE AFS_ERROR_ANODE_INIT[]            = "Inizializzazione di Anode fallita";
UBYTE AFS_ERROR_READ_EXTENSION[]        = "Non  possibile leggere le estensioni del rootblock";
UBYTE AFS_ERROR_EXTENSION_INVALID[]     = "Estensioni del Rootblock non valide";

/* beta messages */

UBYTE AFS_BETA_WARNING_1[]              = "AVVERTIMENTO BETA NR 1";
UBYTE AFS_BETA_WARNING_2[]              = "AVVERTIMENTO BETA NR 2";

/*
 * Message shown when formatting a disk
 */

UBYTE FORMAT_MESSAGE[] =
                "File System Professionale 3 V" RELEASE "\n\n"
                "      Versione Open Source      \n\n"
                "         basato sopra           \n"
                "             PFS3               \n\n"
                "           scritto da           \n"
                "          Michiel Pelt          \n\n"
                "  Premere il mouse per continuare    ";

#if DELDIR
UBYTE deldirname[] = "\007.DELDIR";
#endif
