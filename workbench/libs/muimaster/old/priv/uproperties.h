/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __UPROPERTIES_H__
#define __UPROPERTIES_H__

#include <stdio.h>
#include <stringset.h>

/* Smell like java.util.Properties.
 * Store/retrieve key/value pairs, with file I/O
 */

struct UProperties;

typedef struct UProperties UProperties;

/* creation */
UProperties *uprop_new (ZStringSet *comments);
/* destroy obj, and free all keys and values */
void uprop_destroy (UProperties *prop);

/* file I/O */
int uprop_load (UProperties *prop, FILE *infile);
int uprop_save (UProperties *prop, FILE *outfile, const char *header);

/* access */
const char *uprop_get_property (UProperties *prop, const char *key);
const char *uprop_get_property_with_defaults (UProperties *prop,
					       const char *key,
					       const char *def_val);
/* note: both key and value are copied */
void uprop_set_property (UProperties *prop, const char *key,
			 const char *value);

/* note: try to init *res with your default value */
int uprop_get_property_uint   (UProperties *prop, const char *key, unsigned long *res);
int uprop_get_property_int    (UProperties *prop, const char *key, long   *res);
int uprop_get_property_double (UProperties *prop, const char *key, double *res);
int uprop_get_property_float  (UProperties *prop, const char *key, float  *res);


void uprop_set_property_uint   (UProperties *prop, const char *key, unsigned long value);
void uprop_set_property_int    (UProperties *prop, const char *key, long   value);
void uprop_set_property_double (UProperties *prop, const char *key, double value);
void uprop_set_property_float  (UProperties *prop, const char *key, float  value);

#endif
