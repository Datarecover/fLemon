/*+@@file@@----------------------------------------------------------------*//*!
 \file		table.c
 \par Description 
            Code for processing tables in the LEMON parser generator.
 \par  Status: 
            
 \par Project: 
            Lemon parser
 \date		Created  on Sat Sep  1 21:45:15 2018
 \date		Modified on Sat Sep  1 21:45:15 2018
 \author	
\*//*-@@file@@----------------------------------------------------------------*/
#include "main.h"
#include <string.h>
#include "struct.h"
#include "table.h"
#include "i_o_fmt.h"

/*
 * All code in this file has been automatically generated
 * from a specification in the file
 *              "table.q"
 * by the associative array code building program "aagen".
 * Do not edit this file!
 * Instead, edit the specification file, then rerun aagen.
 */

PRIVATE unsigned strhash(const char *x)
{
	unsigned h = 0;
	while (*x)
		h = h * 13 + *(x++);
	return h;
}

/* Works like strdup, sort of.  Save a string in malloced memory, but
 ** keep strings in a table so that the same string is not in more
 ** than one place.
 */
const char *Strsafe(const char *y)
{
	const char *z;
	char *cpy;

	if (y == 0)
		return 0;
	z = Strsafe_find(y);
	if (z == 0 && (cpy = (char *)malloc(lemonStrlen(y) + 1)) != 0)
	{
		lemon_strcpy(cpy, y);
		z = cpy;
		Strsafe_insert(z);
	}
	MemoryCheck(z);
	return z;
}

/* There is one instance of the following structure for each
 ** associative array of type "x1".
 */
struct s_x1 {
	int size;	/* The number of available slots. */
	/*   Must be a power of 2 greater than or */
	/*   equal to 1 */
	int count;	/* Number of currently slots filled */
	struct s_x1node *tbl;	/* The data stored here */
	struct s_x1node **ht;	/* Hash table for lookups */
};

/* There is one instance of this structure for every data element
 ** in an associative array of type "x1".
 */
typedef struct s_x1node {
	const char *data;	/* The data */
	struct s_x1node *next;	/* Next entry with the same hash */
	struct s_x1node **from;	/* Previous link */
} x1node;

/* There is only one instance of the array, which is the following */
static struct s_x1 *x1a;

/* Allocate a new associative array */
void Strsafe_init(void)
{
	if (x1a)
		return;
	x1a = (struct s_x1 *)malloc(sizeof(struct s_x1));
	if (x1a)
	{
		x1a->size = 1024;
		x1a->count = 0;
		x1a->tbl = (x1node *)calloc(1024, sizeof(x1node) + sizeof(x1node *));
		if (x1a->tbl == 0)
		{
			free(x1a);
			x1a = 0;
		}
		else
		{
			int i;
			x1a->ht = (x1node **) & (x1a->tbl[1024]);
			for (i = 0; i < 1024; i++)
				x1a->ht[i] = 0;
		}
	}
}
/* Insert a new record into the array.  Return TRUE if successful.
 ** Prior data with the same key is NOT overwritten */
int Strsafe_insert(const char *data)
{
	x1node *np;
	unsigned h;
	unsigned ph;

	if (x1a == 0)
		return 0;
	ph = strhash(data);
	h = ph & (x1a->size - 1);
	np = x1a->ht[h];
	while (np)
	{
		if (strcmp(np->data, data) == 0)
		{
			/* An existing entry with the same key is found. */
			/* Fail because overwrite is not allows. */
			return 0;
		}
		np = np->next;
	}
	if (x1a->count >= x1a->size)
	{
		/* Need to make the hash table bigger */
		int i, arrSize;
		struct s_x1 array;
		array.size = arrSize = x1a->size * 2;
		array.count = x1a->count;
		array.tbl = (x1node *)calloc(arrSize, sizeof(x1node) + sizeof(x1node *));
		if (array.tbl == 0)
			return 0;	/* Fail due to malloc failure */
		array.ht = (x1node **) & (array.tbl[arrSize]);
		for (i = 0; i < arrSize; i++)
			array.ht[i] = 0;
		for (i = 0; i < x1a->count; i++)
		{
			x1node *oldnp, *newnp;
			oldnp = &(x1a->tbl[i]);
			h = strhash(oldnp->data) & (arrSize - 1);
			newnp = &(array.tbl[i]);
			if (array.ht[h])
				array.ht[h]->from = &(newnp->next);
			newnp->next = array.ht[h];
			newnp->data = oldnp->data;
			newnp->from = &(array.ht[h]);
			array.ht[h] = newnp;
		}
		free(x1a->tbl);
		*x1a = array;
	}
	/* Insert the new data */
	h = ph & (x1a->size - 1);
	np = &(x1a->tbl[x1a->count++]);
	np->data = data;
	if (x1a->ht[h])
		x1a->ht[h]->from = &(np->next);
	np->next = x1a->ht[h];
	x1a->ht[h] = np;
	np->from = &(x1a->ht[h]);
	return 1;
}

/* Return a pointer to data assigned to the given key.  Return NULL
 ** if no such key. */
const char *Strsafe_find(const char *key)
{
	unsigned h;
	x1node *np;

	if (x1a == 0)
		return 0;
	h = strhash(key) & (x1a->size - 1);
	np = x1a->ht[h];
	while (np)
	{
		if (strcmp(np->data, key) == 0)
			break;
		np = np->next;
	}
	return np ? np->data : 0;
}

/* Return a pointer to the (terminal or nonterminal) symbol "x".
 ** Create a new symbol if this is the first time "x" has been seen.
 */
struct symbol *Symbol_new(const char *x)
{
	struct symbol *sp;

	sp = Symbol_find(x);
	if (sp == 0)
	{
		sp = (struct symbol *)calloc(1, sizeof(struct symbol));
		MemoryCheck(sp);
		sp->name = Strsafe(x);
		sp->type = ISUPPER(*x) ? TERMINAL : NONTERMINAL;
		sp->rule = 0;
		sp->fallback = 0;
		sp->prec = -1;
		sp->assoc = UNK;
		sp->firstset = 0;
		sp->lambda = LEMON_FALSE;
		sp->destructor = 0;
		sp->destLineno = 0;
		sp->datatype = 0;
		sp->useCnt = 0;
		Symbol_insert(sp, sp->name);
	}
	sp->useCnt++;
	return sp;
}

/* Compare two symbols for sorting purposes.  Return negative,
 ** zero, or positive if a is less then, equal to, or greater
 ** than b.
 **
 ** Symbols that begin with upper case letters (terminals or tokens)
 ** must sort before symbols that begin with lower case letters
 ** (non-terminals).  And MULTITERMINAL symbols (created using the
 ** %token_class directive) must sort at the very end. Other than
 ** that, the order does not matter.
 **
 ** We find experimentally that leaving the symbols in their original
 ** order (the order they appeared in the grammar file) gives the
 ** smallest parser tables in SQLite.
 */
int Symbolcmpp(const void *_a, const void *_b)
{
	const struct symbol *a = *(const struct symbol **)_a;
	const struct symbol *b = *(const struct symbol **)_b;
	int i1 = a->type == MULTITERMINAL ? 3 : a->name[0] > 'Z' ? 2 : 1;
	int i2 = b->type == MULTITERMINAL ? 3 : b->name[0] > 'Z' ? 2 : 1;
	return i1 == i2 ? a->index - b->index : i1 - i2;
}

/* There is one instance of the following structure for each
 ** associative array of type "x2".
 */
struct s_x2 {
	int size;	/* The number of available slots. */
	/*   Must be a power of 2 greater than or */
	/*   equal to 1 */
	int count;	/* Number of currently slots filled */
	struct s_x2node *tbl;	/* The data stored here */
	struct s_x2node **ht;	/* Hash table for lookups */
};

/* There is one instance of this structure for every data element
 ** in an associative array of type "x2".
 */
typedef struct s_x2node {
	struct symbol *data;	/* The data */
	const char *key;	/* The key */
	struct s_x2node *next;	/* Next entry with the same hash */
	struct s_x2node **from;	/* Previous link */
} x2node;

/* There is only one instance of the array, which is the following */
static struct s_x2 *x2a;

/* Allocate a new associative array */
void Symbol_init(void)
{
	if (x2a)
		return;
	x2a = (struct s_x2 *)malloc(sizeof(struct s_x2));
	if (x2a)
	{
		x2a->size = 128;
		x2a->count = 0;
		x2a->tbl = (x2node *)calloc(128, sizeof(x2node) + sizeof(x2node *));
		if (x2a->tbl == 0)
		{
			free(x2a);
			x2a = 0;
		}
		else
		{
			int i;
			x2a->ht = (x2node **) & (x2a->tbl[128]);
			for (i = 0; i < 128; i++)
				x2a->ht[i] = 0;
		}
	}
}
/* Insert a new record into the array.  Return TRUE if successful.
 ** Prior data with the same key is NOT overwritten */
int Symbol_insert(struct symbol *data, const char *key)
{
	x2node *np;
	unsigned h;
	unsigned ph;

	if (x2a == 0)
		return 0;
	ph = strhash(key);
	h = ph & (x2a->size - 1);
	np = x2a->ht[h];
	while (np)
	{
		if (strcmp(np->key, key) == 0)
		{
			/* An existing entry with the same key is found. */
			/* Fail because overwrite is not allows. */
			return 0;
		}
		np = np->next;
	}
	if (x2a->count >= x2a->size)
	{
		/* Need to make the hash table bigger */
		int i, arrSize;
		struct s_x2 array;
		array.size = arrSize = x2a->size * 2;
		array.count = x2a->count;
		array.tbl = (x2node *)calloc(arrSize, sizeof(x2node) + sizeof(x2node *));
		if (array.tbl == 0)
			return 0;	/* Fail due to malloc failure */
		array.ht = (x2node **) & (array.tbl[arrSize]);
		for (i = 0; i < arrSize; i++)
			array.ht[i] = 0;
		for (i = 0; i < x2a->count; i++)
		{
			x2node *oldnp, *newnp;
			oldnp = &(x2a->tbl[i]);
			h = strhash(oldnp->key) & (arrSize - 1);
			newnp = &(array.tbl[i]);
			if (array.ht[h])
				array.ht[h]->from = &(newnp->next);
			newnp->next = array.ht[h];
			newnp->key = oldnp->key;
			newnp->data = oldnp->data;
			newnp->from = &(array.ht[h]);
			array.ht[h] = newnp;
		}
		free(x2a->tbl);
		*x2a = array;
	}
	/* Insert the new data */
	h = ph & (x2a->size - 1);
	np = &(x2a->tbl[x2a->count++]);
	np->key = key;
	np->data = data;
	if (x2a->ht[h])
		x2a->ht[h]->from = &(np->next);
	np->next = x2a->ht[h];
	x2a->ht[h] = np;
	np->from = &(x2a->ht[h]);
	return 1;
}

/* Return a pointer to data assigned to the given key.  Return NULL
 ** if no such key. */
struct symbol *Symbol_find(const char *key)
{
	unsigned h;
	x2node *np;

	if (x2a == 0)
		return 0;
	h = strhash(key) & (x2a->size - 1);
	np = x2a->ht[h];
	while (np)
	{
		if (strcmp(np->key, key) == 0)
			break;
		np = np->next;
	}
	return np ? np->data : 0;
}

/* Return the n-th data.  Return NULL if n is out of range. */
struct symbol *Symbol_Nth(int n)
{
	struct symbol *data;
	if (x2a && n > 0 && n <= x2a->count)
	{
		data = x2a->tbl[n - 1].data;
	}
	else
	{
		data = 0;
	}
	return data;
}

/* Return the size of the array */
int Symbol_count(void)
{
	return x2a ? x2a->count : 0;
}

/* Return an array of pointers to all data in the table.
 ** The array is obtained from malloc.  Return NULL if memory allocation
 ** problems, or if the array is empty. */
struct symbol **Symbol_arrayof(void)
{
	struct symbol **array;
	int i, arrSize;
	if (x2a == 0)
		return 0;
	arrSize = x2a->count;
	array = (struct symbol **)calloc(arrSize, sizeof(struct symbol *));
	if (array)
	{
		for (i = 0; i < arrSize; i++)
			array[i] = x2a->tbl[i].data;
	}
	return array;
}

/* Compare two configurations */
int Configcmp(const char *_a, const char *_b)
{
	const struct config *a = (struct config *)_a;
	const struct config *b = (struct config *)_b;
	int x;
	x = a->rp->index - b->rp->index;
	if (x == 0)
		x = a->dot - b->dot;
	return x;
}

/* Compare two states */
PRIVATE int statecmp(struct config *a, struct config *b)
{
	int rc;
	for (rc = 0; rc == 0 && a && b; a = a->bp, b = b->bp)
	{
		rc = a->rp->index - b->rp->index;
		if (rc == 0)
			rc = a->dot - b->dot;
	}
	if (rc == 0)
	{
		if (a)
			rc = 1;
		if (b)
			rc = -1;
	}
	return rc;
}

/* Hash a state */
PRIVATE unsigned statehash(struct config *a)
{
	unsigned h = 0;
	while (a)
	{
		h = h * 571 + a->rp->index * 37 + a->dot;
		a = a->bp;
	}
	return h;
}

/* Allocate a new state structure */
struct state *State_new(void)
{
	struct state *newstate;
	newstate = (struct state *)calloc(1, sizeof(struct state));
	MemoryCheck(newstate);
	return newstate;
}

/* There is one instance of the following structure for each
 ** associative array of type "x3".
 */
struct s_x3 {
	int size;	/* The number of available slots. */
	/*   Must be a power of 2 greater than or */
	/*   equal to 1 */
	int count;	/* Number of currently slots filled */
	struct s_x3node *tbl;	/* The data stored here */
	struct s_x3node **ht;	/* Hash table for lookups */
};

/* There is one instance of this structure for every data element
 ** in an associative array of type "x3".
 */
typedef struct s_x3node {
	struct state *data;	/* The data */
	struct config *key;	/* The key */
	struct s_x3node *next;	/* Next entry with the same hash */
	struct s_x3node **from;	/* Previous link */
} x3node;

/* There is only one instance of the array, which is the following */
static struct s_x3 *x3a;

/* Allocate a new associative array */
void State_init(void)
{
	if (x3a)
		return;
	x3a = (struct s_x3 *)malloc(sizeof(struct s_x3));
	if (x3a)
	{
		x3a->size = 128;
		x3a->count = 0;
		x3a->tbl = (x3node *)calloc(128, sizeof(x3node) + sizeof(x3node *));
		if (x3a->tbl == 0)
		{
			free(x3a);
			x3a = 0;
		}
		else
		{
			int i;
			x3a->ht = (x3node **) & (x3a->tbl[128]);
			for (i = 0; i < 128; i++)
				x3a->ht[i] = 0;
		}
	}
}
/* Insert a new record into the array.  Return TRUE if successful.
 ** Prior data with the same key is NOT overwritten */
int State_insert(struct state *data, struct config *key)
{
	x3node *np;
	unsigned h;
	unsigned ph;

	if (x3a == 0)
		return 0;
	ph = statehash(key);
	h = ph & (x3a->size - 1);
	np = x3a->ht[h];
	while (np)
	{
		if (statecmp(np->key, key) == 0)
		{
			/* An existing entry with the same key is found. */
			/* Fail because overwrite is not allows. */
			return 0;
		}
		np = np->next;
	}
	if (x3a->count >= x3a->size)
	{
		/* Need to make the hash table bigger */
		int i, arrSize;
		struct s_x3 array;
		array.size = arrSize = x3a->size * 2;
		array.count = x3a->count;
		array.tbl = (x3node *)calloc(arrSize, sizeof(x3node) + sizeof(x3node *));
		if (array.tbl == 0)
			return 0;	/* Fail due to malloc failure */
		array.ht = (x3node **) & (array.tbl[arrSize]);
		for (i = 0; i < arrSize; i++)
			array.ht[i] = 0;
		for (i = 0; i < x3a->count; i++)
		{
			x3node *oldnp, *newnp;
			oldnp = &(x3a->tbl[i]);
			h = statehash(oldnp->key) & (arrSize - 1);
			newnp = &(array.tbl[i]);
			if (array.ht[h])
				array.ht[h]->from = &(newnp->next);
			newnp->next = array.ht[h];
			newnp->key = oldnp->key;
			newnp->data = oldnp->data;
			newnp->from = &(array.ht[h]);
			array.ht[h] = newnp;
		}
		free(x3a->tbl);
		*x3a = array;
	}
	/* Insert the new data */
	h = ph & (x3a->size - 1);
	np = &(x3a->tbl[x3a->count++]);
	np->key = key;
	np->data = data;
	if (x3a->ht[h])
		x3a->ht[h]->from = &(np->next);
	np->next = x3a->ht[h];
	x3a->ht[h] = np;
	np->from = &(x3a->ht[h]);
	return 1;
}

/* Return a pointer to data assigned to the given key.  Return NULL
 ** if no such key. */
struct state *State_find(struct config *key)
{
	unsigned h;
	x3node *np;

	if (x3a == 0)
		return 0;
	h = statehash(key) & (x3a->size - 1);
	np = x3a->ht[h];
	while (np)
	{
		if (statecmp(np->key, key) == 0)
			break;
		np = np->next;
	}
	return np ? np->data : 0;
}

/* Return an array of pointers to all data in the table.
 ** The array is obtained from malloc.  Return NULL if memory allocation
 ** problems, or if the array is empty. */
struct state **State_arrayof(void)
{
	struct state **array;
	int i, arrSize;
	if (x3a == 0)
		return 0;
	arrSize = x3a->count;
	array = (struct state **)calloc(arrSize, sizeof(struct state *));
	if (array)
	{
		for (i = 0; i < arrSize; i++)
			array[i] = x3a->tbl[i].data;
	}
	return array;
}

/* Hash a configuration */
PRIVATE unsigned confighash(struct config *a)
{
	unsigned h = 0;
	h = h * 571 + a->rp->index * 37 + a->dot;
	return h;
}

/* There is one instance of the following structure for each
 ** associative array of type "x4".
 */
struct s_x4 {
	int size;	/* The number of available slots. */
	/*   Must be a power of 2 greater than or */
	/*   equal to 1 */
	int count;	/* Number of currently slots filled */
	struct s_x4node *tbl;	/* The data stored here */
	struct s_x4node **ht;	/* Hash table for lookups */
};

/* There is one instance of this structure for every data element
 ** in an associative array of type "x4".
 */
typedef struct s_x4node {
	struct config *data;	/* The data */
	struct s_x4node *next;	/* Next entry with the same hash */
	struct s_x4node **from;	/* Previous link */
} x4node;

/* There is only one instance of the array, which is the following */
static struct s_x4 *x4a;

/* Allocate a new associative array */
void Configtable_init(void)
{
	if (x4a)
		return;
	x4a = (struct s_x4 *)malloc(sizeof(struct s_x4));
	if (x4a)
	{
		x4a->size = 64;
		x4a->count = 0;
		x4a->tbl = (x4node *)calloc(64, sizeof(x4node) + sizeof(x4node *));
		if (x4a->tbl == 0)
		{
			free(x4a);
			x4a = 0;
		}
		else
		{
			int i;
			x4a->ht = (x4node **) & (x4a->tbl[64]);
			for (i = 0; i < 64; i++)
				x4a->ht[i] = 0;
		}
	}
}
/* Insert a new record into the array.  Return TRUE if successful.
 ** Prior data with the same key is NOT overwritten */
int Configtable_insert(struct config *data)
{
	x4node *np;
	unsigned h;
	unsigned ph;

	if (x4a == 0)
		return 0;
	ph = confighash(data);
	h = ph & (x4a->size - 1);
	np = x4a->ht[h];
	while (np)
	{
		if (Configcmp((const char *)np->data, (const char *)data) == 0)
		{
			/* An existing entry with the same key is found. */
			/* Fail because overwrite is not allows. */
			return 0;
		}
		np = np->next;
	}
	if (x4a->count >= x4a->size)
	{
		/* Need to make the hash table bigger */
		int i, arrSize;
		struct s_x4 array;
		array.size = arrSize = x4a->size * 2;
		array.count = x4a->count;
		array.tbl = (x4node *)calloc(arrSize, sizeof(x4node) + sizeof(x4node *));
		if (array.tbl == 0)
			return 0;	/* Fail due to malloc failure */
		array.ht = (x4node **) & (array.tbl[arrSize]);
		for (i = 0; i < arrSize; i++)
			array.ht[i] = 0;
		for (i = 0; i < x4a->count; i++)
		{
			x4node *oldnp, *newnp;
			oldnp = &(x4a->tbl[i]);
			h = confighash(oldnp->data) & (arrSize - 1);
			newnp = &(array.tbl[i]);
			if (array.ht[h])
				array.ht[h]->from = &(newnp->next);
			newnp->next = array.ht[h];
			newnp->data = oldnp->data;
			newnp->from = &(array.ht[h]);
			array.ht[h] = newnp;
		}
		free(x4a->tbl);
		*x4a = array;
	}
	/* Insert the new data */
	h = ph & (x4a->size - 1);
	np = &(x4a->tbl[x4a->count++]);
	np->data = data;
	if (x4a->ht[h])
		x4a->ht[h]->from = &(np->next);
	np->next = x4a->ht[h];
	x4a->ht[h] = np;
	np->from = &(x4a->ht[h]);
	return 1;
}

/* Return a pointer to data assigned to the given key.  Return NULL
 ** if no such key. */
struct config *Configtable_find(struct config *key)
{
	int h;
	x4node *np;

	if (x4a == 0)
		return 0;
	h = confighash(key) & (x4a->size - 1);
	np = x4a->ht[h];
	while (np)
	{
		if (Configcmp((const char *)np->data, (const char *)key) == 0)
			break;
		np = np->next;
	}
	return np ? np->data : 0;
}

/* Remove all data from the table.  Pass each data to the function "f"
 ** as it is removed.  ("f" may be null to avoid this step.) */
void Configtable_clear(int (*f) (struct config *))
{
	int i;
	if (x4a == 0 || x4a->count == 0)
		return;
	if (f)
		for (i = 0; i < x4a->count; i++)
			(*f) (x4a->tbl[i].data);
	for (i = 0; i < x4a->size; i++)
		x4a->ht[i] = 0;
	x4a->count = 0;
	return;
}
