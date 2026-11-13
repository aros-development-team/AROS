/* $Id$ */
/* $Log: french_messages.c $
 * Revision 1.7  1999/05/14  11:31:34  Michiel
 * Long filename support implemented; bugfixes
 *
 * Revision 1.6  1999/03/09  10:31:35  Michiel
 * 00110: release number define
 *
 * Revision 1.5  1998/10/02  07:22:45  Michiel
 * final release 4.2 version
 *
 * Revision 1.3  1998/09/03  07:12:14  Michiel
 * versie 17.4
 * bugfixes 118, 121, 123 and superindexblocks and td64 support
 *
 * Revision 1.2  1998/08/05  00:30:35  Michiel
 * Update by Pierre
 *
 * Revision 1.2  1997/11/19  22:14:15  mark
 * Copyright transferred from FLD to GREED
 * Changed name from AFS to PFS-II
 * French Translation 01-Jun-98 by Philippe REUX
 * Revision 1.1  1997/03/03  22:04:04  Michiel
 * Initial revision
 * */

#include "versionhistory.doc"
#include <exec/types.h>

UBYTE AFS_WARNING_MEMORY_MASK[]         = "ATTENTION:\nLa mémoire allouée ne correspond pas au memorymask";

UBYTE AFS_ERROR_DNV_ALLOC_INFO[]        = "ALERTE:\nInformation d'allocation introuvable";
UBYTE AFS_ERROR_DNV_ALLOC_BLOCK[]       = "ALERTE:\nBloc d'allocation introuvable";
UBYTE AFS_ERROR_DNV_WRONG_ANID[]        = "ALERTE:\nMauvaise identification du bloc d'allocation";
UBYTE AFS_ERROR_DNV_WRONG_DIRID[]       = "ALERTE:\nMauvaise identification du bloc répertoire";
UBYTE AFS_ERROR_DNV_LOAD_DIRBLOCK[]     = "ALERTE:\nBloc répertoire illisible";
UBYTE AFS_ERROR_DNV_WRONG_BMID[]        = "ALERTE:\nMauvaise identification du bloc bitmap";
UBYTE AFS_ERROR_DNV_WRONG_INDID[]       = "ALERTE:\nMauvaise identification du bloc d'index";
UBYTE AFS_ERROR_CACHE_INCONSISTENCY[]   = "Incohérence du cache detectée\nAchevez toute activité du disque";
UBYTE AFS_ERROR_OUT_OF_BUFFERS[]        = "Tampons mémoire insuffisants";
UBYTE AFS_ERROR_MEMORY_POOL[]           = "Impossible d'allouer la mémoire";
UBYTE AFS_ERROR_PLEASE_FREE_MEM[]       = "Veuillez libérer un peu de mémoire";
UBYTE AFS_ERROR_LIBRARY_PROBLEM[]       = "Impossible d'ouvrir la librairie !";
UBYTE AFS_ERROR_INIT_FAILED[]           = "Echec de l'initialisation";
UBYTE AFS_ERROR_READ_OUTSIDE[]          = "Tentative de lecture hors de la partition !";
UBYTE AFS_ERROR_WRITE_OUTSIDE[]         = "Tentative d'écriture hors de la partition !";
UBYTE AFS_ERROR_SEEK_OUTSIDE[]          = "Tentative de recherche hors de la partition !";
UBYTE AFS_ERROR_READ_ERROR[]            = "Erreur de lecture %ld sur le bloc %ld\n"
                                          "Assurez-vous de l'insertion du disque";
UBYTE AFS_ERROR_WRITE_ERROR[]           = "Erreur d'écriture %ld sur le bloc %ld\n"
                                          "Assurez-vous de l'insertion du disque";
UBYTE AFS_ERROR_READ_DELDIR[]           = "Deldir illisible";
UBYTE AFS_ERROR_DELDIR_INVALID[]        = "Deldir invalide";
UBYTE AFS_ERROR_EXNEXT_FAIL[]           = "Echec de «ExamineNext»";
UBYTE AFS_ERROR_DOSLIST_ADD[]           = "Erreur d'ajoût à la DosList.\nVeuillez retirer le volume";
UBYTE AFS_ERROR_EX_NEXT_FAIL[]          = "Echec de «ExamineNext»";
UBYTE AFS_ERROR_NEWDIR_ADDLISTENTRY[]   = "Echec de «Newdir addlistentry»";
UBYTE AFS_ERROR_LOAD_DIRBLOCK_FAIL[]    = "Impossible de charger le bloc répertoire !!";
UBYTE AFS_ERROR_LRU_UPDATE_FAIL[]       = "Echec de la mise à jour du LRU\n"
                                          "LRU = Cache le moins récemment utilisé";
UBYTE AFS_ERROR_UPDATE_FAIL[]           = "La mise à jour du disque a échoué";
UBYTE AFS_ERROR_UNSLEEP[]               = "Erreur de la procédure «Unsleep»";
UBYTE AFS_ERROR_DISK_TOO_LARGE[]        = "Disque trop grand pour cette version de PFS3.\nVeuillez installer la version TD64 ou la version direct-SCSI";
UBYTE AFS_ERROR_ANODE_ERROR[]           = "Index de l'anode invalide";
UBYTE AFS_ERROR_ANODE_INIT[]            = "Echec de l'initialisation de l'anode";

#if VERSION23
UBYTE AFS_ERROR_READ_EXTENSION[]        = "Impossible de lire l'extension du bloc racine";
UBYTE AFS_ERROR_EXTENSION_INVALID[]     = "Extension du bloc racine invalide";
#endif

/* beta messages */

UBYTE AFS_BETA_WARNING_1[]              = "ALERTE BETA N°1";
UBYTE AFS_BETA_WARNING_2[]              = "ALERTE BETA N°2";

/*
 * Message shown when formatting a disk
 */

UBYTE FORMAT_MESSAGE[] =
                "Professional File System 3 V" RELEASE "\n\n"
                "      Version Open Source       \n\n"
                "        basé en fonction        \n"
                "              PFS3              \n\n"
                "            créé par            \n"
                "          Michiel Pelt          \n\n"
                "Cliquez la souris pour continuer";

#if DELDIR
UBYTE deldirname[] = "\007.DELDIR";
#endif
