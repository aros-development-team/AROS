#ifndef ENVOY_ERRORS_H
#define ENVOY_ERRORS_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Envoy error codes (as published in the Envoy developer kit).
*/

/* General errors, usable by every component */
#define ENVOYERR_NORESOURCES        501     /* memory or other resources exhausted              */
#define ENVOYERR_CMDUNKNOWN         502     /* server does not know this trans_Command           */
#define ENVOYERR_NOTINPLACE         503     /* server requires an in-place transaction           */
#define ENVOYERR_SMALLRESPBUFF      504     /* response does not fit the response buffer         */
#define ENVOYERR_NULLPTR            505     /* a required pointer was NULL                       */
#define ENVOYERR_SMALLREQBUFF       506     /* request shorter than the command requires         */
#define ENVOYERR_ILLEGALINPLACE     507     /* server does not accept in-place transactions      */

/* nipc.library */
#define ENVOYERR_UNKNOWNHOST        521     /* FindEntity: host name cannot be resolved          */
#define ENVOYERR_UNKNOWNENTITY      522     /* FindEntity: no such public entity                 */
#define ENVOYERR_NORESOLVER         523     /* FindEntity: the host's resolver did not answer    */
#define ENVOYERR_ABORTED            530     /* AbortTransaction() / LoseEntity() with pending    */
#define ENVOYERR_CANTDELIVER        531     /* destination unreachable or connection lost        */
#define ENVOYERR_ABOUTTOWAIT        532     /* (reserved: TRANSF_NOWAIT would have blocked)      */
#define ENVOYERR_REFUSED            533     /* (reserved)                                        */
#define ENVOYERR_TIMEOUT            534     /* trans_Timeout expired                             */

/* accounts.library */
#define ENVOYERR_UNKNOWNUSER        541
#define ENVOYERR_UNKNOWNGROUP       542
#define ENVOYERR_LASTUSER           543
#define ENVOYERR_LASTGROUP          544
#define ENVOYERR_LASTMEMBER         545
#define ENVOYERR_UNKNOWNMEMBER      546

/* services.library */
#define ENVOYERR_UNKNOWNSERVICE     560
#define ENVOYERR_OPENSERVICEFAIL    561
#define ENVOYERR_BADSTARTSERVICE    562

/* Application-private errors: ENVOYERR_APP + 0 ... + 99 */
#define ENVOYERR_APP                1000

#endif /* ENVOY_ERRORS_H */
