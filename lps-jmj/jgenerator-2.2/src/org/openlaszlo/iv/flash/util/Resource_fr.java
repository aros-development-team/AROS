/*
 * $Id: Resource_fr.java,v 1.6 2002/06/25 15:31:36 ptalbot Exp $
 *
 * ===========================================================================
 *
 * The JGenerator Software License, Version 1.0
 *
 * Copyright (c) 2000 Dmitry Skavish (skavish@usa.net). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution, if
 *    any, must include the following acknowlegement:
 *    "This product includes software developed by Dmitry Skavish
 *     (skavish@usa.net, http://www.flashgap.com/)."
 *    Alternately, this acknowlegement may appear in the software itself,
 *    if and wherever such third-party acknowlegements normally appear.
 *
 * 4. The name "The JGenerator" must not be used to endorse or promote
 *    products derived from this software without prior written permission.
 *    For written permission, please contact skavish@usa.net.
 *
 * 5. Products derived from this software may not be called "The JGenerator"
 *    nor may "The JGenerator" appear in their names without prior written
 *    permission of Dmitry Skavish.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL DMITRY SKAVISH OR THE OTHER
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

package org.openlaszlo.iv.flash.util;

import java.util.*;

/**
 * Resources
 * <P>
 * Resource key has to have digit in first character from 0 to 4.
 * <ol start=0>
 * <li>FATAL
 * <li>ERROR
 * <li>WARN
 * <li>INFO
 * <li>DEBUG
 * </ol>
 *
 * @author Dmitry Skavish
 */
public class Resource_fr extends Resource {

    private static final Object[][] contents = {
        {INFINITELOOP        , "Boucle sans fin pendant le traitement du script"},
        {GENCMDERR           , "La commande Generator ''{0}'' ne peut \u00EAtre appliqu\u00E9e qu'\u00E0 une instance"},
        {UNKNOWNTAG          , "Tag inconnu ''{0}'' rencontr\u00E9"},
        {UNKNOWNGENTEXT      , "Subtag texte Generator inconnu {0} rencontr\u00E9"},
        {CMDNOTFOUND         , "Commande Generator ''{0}'' non reconnue"},
        {CMDNOTCREATED       , "Erreur \u00E0 la cr\u00E9ation de la commande Generator ''{0}''"},
        {ERRCMDFILEREAD      , "Erreur \u00E0 la lecture du fichier ''{0}'' dans la commande {1}"},
        {CMDSCRIPTNOTFOUND   , "Script ''{0}'' non trouv\u00E9 dans la commande {1}"},
        {INVALRATEVALUE      , "Cadence invalide {0} dans la commande 'SetMovieParameters'"},
        {INVALURL            , "Url invalide ''{0}''"},
        {ERRDATAREAD         , "Erreur \u00E0 la lecture des donn\u00E9es depuis la source ''{0}'' dans la commande {1}"},
        {INVALDATASOURCE     , "Source de donn\u00E9es invalide ''{0}'' dans la commande {1}"},
        {INVALALPHA          , "Valeur de pourcentage invalide {0} dans la commande {1}"},
        {COLNOTFOUNDCMD      , "Commande {0} : Colonne {1} non trouv\u00E9e dans la source de donn\u00E9es {2}"},
        {COLNOTFOUNDCMD1     , "Commande {0} : L'une des colonnes suivantes est requise dans la source de donn\u00E9es : {1}"},
        {ERRPARSETEMPLATE    , "Erreur pendant le d\u00E9codage du mod\u00E8le ''{0}''"},
        {CANTREADHEADER      , "Impossible de lire l'ent\u00EAte du mod\u00E8le ''{0}''"},
        {ILLEGALHEADER       , "Le mod\u00E8le ''{0}'' a une ent\u00EAte ill\u00E9gale - ce n'est pas un fichier Shockwave Flash"},
        {FILETOOSHORT        , "La taille du fichier mod\u00E8le ''{0}'' est trop r\u00E9duite"},
        {FILETOOBIG          , "Le fichier mod\u00E8le ''{0}'' est trop volumineux pour pouvoir \u00EAtre charg\u00E9 en m\u00E9moire"},
        {ERRWRITINGFILE      , "Erreur d'\u00E9criture du fichier ''{0}''"},
        {ERRREADINGFILE      , "Erreur de lecture du fichier ''{0}''"},
        {FILENOTFOUND        , "Fichier ''{0}'' non trouv\u00E9"},
        {UNKNOWNERROR        , "Une exception inattendue est survenue, veuillez envoyer le fichier log aux d\u00E9veloppeurs"},
        {REENCODINGJPEG      , "R\u00E9-Encodage de l'image JPEG avec une qualit\u00E9 de {0}"},
        {RESCALINGJPEG       , "Redimensionnement de l'image JPEG avec une qualit\u00E9 de {2}, nouvelle largeur = {0}, hauteur = {1}"},
        {INVLEXTERNALFONT    , "Impossible de trouver la moindre police dans le fichier ''{0}''"},
        {CLIPORDEFSYM        , "L'objet Table doit comporter soit une colonne 'Clip' soit un 'DefaultSymbol'"},
        {ROWSORCOLS          , "Nombre invalide de lignes ou de colonnes dans la commande {0}"},
        {BORDERTOOTHICK      , "La bordure est trop \u00E0paisse dans la commande Table"},
        {PROCESSREQUEST      , "Fichier trait\u00E9 ''{0}'', taille {1} octets, temps de traitement {2} ms, temps total {3} ms"},
        {ERRREADINGGIF       , "Erreur de lecture du flux GIF"},
        {ERRREADINGJPG       , "Erreur de lecture du flux JPEG"},
        {ERRDOCMD            , "Une erreur est survenue dans le fichier ''{0}'', clip ''{1}'', image ''{2}'', commande ''{3}''"},
        {ERRREADINGPNG       , "Erreur de lecture du flux PNG"},
        {UNSUPMEDIA          , "Fichier m\u00E9dia non support\u00E9 ''{0}''"},
        {NOTEXT              , "Aucun texte trouv\u00E9 pour la commande 'InsertText'"},
        {CONNECTINGTO        , "Connexion \u00E0 {0} ..."},
        {RETRIEVINGCONTENT   , "R\u00E9cup\u00E9ration du contenu de {0} ..."},
        {INVLMP3LAYER        , "MP3 invalide - seul le MPEG Audio Layer 3 est support\u00E9"},
        {INVLMP3             , "Ent\u00EAte MP3 invalide"},
        {INVLMP3FREQUENCY    , "Fr\u00E9quence MP3 invalide - valeurs accept\u00E9es : 11025Hz, 22050Hz ou 44100Hz"},
        {NOGRAPHCONTEXT      , "Le contexte courant n'a pas d'anc\u00EAtre 'GraphContext'"},
        {NOMATCHINGCONTEXTS  , "Aucun anc\u00EAtre n'a pu \u00EAtre d\u00E9termin\u00E9 pour ''{0}''"},
        {EXPECTSTDCONTEXT    , "Un contexte de type Texte (tabul\u00E9) \u00E9tait attendu"},
        {ERRSETPROPERTY      , "Veuillez param\u00E9trer la propri\u00E9t\u00E9 'org.openlaszlo.iv.flash.installDir' avec le r\u00E9pertoire o\u00F9 JGenerator est install\u00E9"},
        {CANTLOADPROPERTIES  , "Impossible de charger le fichier de propri\u00E9t\u00E9s"},
        {CANTLOADSYSPROPS    , "Impossible de charger les propri\u00E9t\u00E9s syst\u00E8me, utilisation des valeurs par d\u00E9faut"},
        {SQLQUERY            , "Requ\u00EAte SQL \n   pilote : {0}\n   url : {1}\n   utilisateur/mot de passe : {2}/{3}\n   requ\u00EAte : {4}"},
        {BADHIGHORLOW        , "Commande {0} : Soit 'HIGH' soit 'LOW' n'est pas une valeur limite dans la ligne {1} de la source de donn\u00E9es {2}"},
        {PROCESSERROR        , "Erreur de traitement du mod\u00E8le ''{0}''"},
        {SENDERROR           , "Erreur d'\u00E9criture du flux de sortie pendant le traitement du mod\u00E9le ''{0}''"},
        {REQUESTFROM         , "Traitement de la requ\u00EAte depuis ''{0}''"},
        {JSERROR             , "JavaScript: {0}"},
        {INLINECMDNOTFOUND   , "Commande interne {0} non trouv\u00E9e"},
        {INLINECMDERROR      , "Erreur lors de l'ex\u00E9cution de la commande interne {0}"},
        {CMDINSTNOTFOUND     , "Impossible de trouver une instance pour la commande {0}. Saisissez le param\u00E8tre 'instancename'."},
        {SERVERSTARTED       , "Demarrage du servlet JGenerator."},
    };

    public Object[][] getContents() {
        return contents;
    }

}

