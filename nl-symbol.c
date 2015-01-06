/* nl-symbol.c --- symbol handling routines for newLISP
//
// Copyright (C) 1992-2007 Lutz Mueller <lutz@nuevatec.com>
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2, 1991,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
*/


#include "newlisp.h"
#include "protos.h"


extern CELL * cellMemory;
extern SYMBOL * trueSymbol;
extern SYMBOL * orSymbol;

SYMBOL * findInsertSymbol(char * key, int forceCreation);
int deleteSymbol(char * key);
void deleteContextSymbols(CELL * cell);
CELL dumpSymbol(char * name);
void collectSymbols(SYMBOL * sPtr, CELL * symbolList, CELL * * nextSymbol);
void symbolReferences(SYMBOL * sPtr,  CELL * symbolList, CELL * * nextSymbol);
static SYMBOL * root;	/* root symbol derived from context */

/* --------- return a list of all symbols in a context -------------- */


CELL * p_symbols(CELL * params)
{
SYMBOL * context;
CELL * symbolList;
CELL * nextSymbol;

symbolList = getCell(CELL_EXPRESSION);
nextSymbol = NULL;

if(params->type == CELL_NIL) 
	context = currentContext;
else
	getContext(params, &context);

if(context) /* check in case we are in debug mode */
	collectSymbols((SYMBOL *)((CELL *)context->contents)->aux, symbolList, &nextSymbol);
return(symbolList);
}


void collectSymbols(SYMBOL * sPtr, CELL * symbolList, CELL * * nextSymbol)
{
if(sPtr != NIL_SYM && sPtr != NULL)
	{
	collectSymbols(sPtr->left, symbolList, nextSymbol);
	if(*nextSymbol == NULL)
		{
		*nextSymbol = getCell(CELL_SYMBOL);
		(*nextSymbol)->contents = (UINT)sPtr;
		symbolList->contents = (UINT)*nextSymbol;
		}
	else 
		{
		(*nextSymbol)->next = getCell(CELL_SYMBOL);
		*nextSymbol = (*nextSymbol)->next;
		(*nextSymbol)->contents = (UINT)sPtr;
		}
	collectSymbols(sPtr->right, symbolList, nextSymbol);
	}
}



/* iterate thru symbol tree for a specific context
*/

CELL * p_dotree(CELL * params)
{
SYMBOL * context;
SYMBOL * symbol;
CELL * symbolList;
CELL * nextSymbol;
CELL * cell;
CELL * list;
int resultIdxSave;

if(params->type != CELL_EXPRESSION)
	return(errorProcExt(ERR_LIST_EXPECTED, params));

list = (CELL *)params->contents;
if(list->type == CELL_SYMBOL)
	symbol = (SYMBOL *)list->contents;
else if(list->type == CELL_DYN_SYMBOL)
	symbol = getDynamicSymbol(list);
else
	return(errorProcExt(ERR_SYMBOL_EXPECTED, list));

if(isProtected(symbol->flags))
	return(errorProcExt2(ERR_SYMBOL_PROTECTED, stuffSymbol(symbol)));

pushEnvironment((CELL *)symbol->contents);
pushEnvironment((UINT)symbol);

symbol->contents = (UINT)copyCell(nilCell);

getContext(list->next, &context);
if(!context) return(nilCell); /* for debug mode */
cell = nilCell;

symbolList = getCell(CELL_EXPRESSION);
nextSymbol = NULL;
collectSymbols((SYMBOL *)((CELL *)context->contents)->aux, symbolList, &nextSymbol);

resultIdxSave = resultStackIdx;
list = (CELL *)symbolList->contents;
while(list != nilCell)
    {
    cleanupResults(resultIdxSave);
    deleteList((CELL *)symbol->contents);
    symbol->contents = (UINT)copyCell(list);
    cell = evaluateBlock(params->next);
    list = list->next;
    }

deleteList((CELL *)symbol->contents);

symbol = (SYMBOL*)popEnvironment();
symbol->contents = (UINT)popEnvironment();

deleteList(symbolList);

return(copyCell(cell));
}



SYMBOL * lookupSymbol(char * token, SYMBOL * context)
{
root = (SYMBOL *)((CELL *)context->contents)->aux;

return(findInsertSymbol(token, LOOKUP_ONLY));
}



/* 
   if forceFlag is TRUE then 
       create the symbol, if not found in the context 
       specified in that context
   else
       if not found try to inherit from MAIN as a global
       or primitive, else create it in context specified
*/


SYMBOL * translateCreateSymbol
	(char * token, int type, SYMBOL * context, int forceFlag)
{
SYMBOL * sPtr;
CELL * cell = NULL;
size_t len;

/* for the first symbol (also a context) context is NULL */
if(context == NULL)
	root = NULL;
else
	{
	cell = (CELL *)context->contents;
	root = (SYMBOL *)cell->aux;
	}

if(forceFlag)
	sPtr = findInsertSymbol(token, FORCE_CREATION);
else /* try to inherit from MAIN, if not here create in current context */
	{
	sPtr = findInsertSymbol(token, LOOKUP_ONLY);
	if(sPtr == NULL)
		{
		if(context != mainContext)
			{
			root = (SYMBOL *)((CELL *)mainContext->contents)->aux;
			sPtr = findInsertSymbol(token, LOOKUP_ONLY);
			/* since 7.2.7 only inherit primitives and other globals */
			if(sPtr != NULL && !(sPtr->flags & SYMBOL_GLOBAL))
				{
				if(symbolType(sPtr) != CELL_CONTEXT
				    || (SYMBOL *)((CELL*)sPtr->contents)->contents != sPtr)
					sPtr = NULL;
				}
			root = (SYMBOL *)cell->aux;
			}
		if(sPtr == NULL)
			sPtr = findInsertSymbol(token, FORCE_CREATION);
		}
	}

/* root might have changed, if new symbol was inserted */
if(context != NULL)
	cell->aux = (UINT)root;

/* the symbol existed already, return */
if(sPtr->contents != 0) return(sPtr);
	
/* a new symbol has been allocated by findInsertSymbol() */
if(type != CELL_PRIMITIVE)
	{
	len = strlen(token);
	sPtr->name = (char *)allocMemory(len + 1);
	memcpy(sPtr->name, token, len + 1);
 	cell = copyCell(nilCell); 
	sPtr->contents = (UINT)cell;
	/* don't if this is a context variable (not in MAIN) 8.9.8 
	   but could this  protect/make global context vars in MAIN?
	   new and def-new inhibit targets to be MAIN, cant come from there 
    */
	if(type == CELL_CONTEXT && context == mainContext)
		{
		cell->type = CELL_CONTEXT;
		cell->contents = (UINT)sPtr;
		cell->aux = 0;
		sPtr->flags |= (SYMBOL_PROTECTED | SYMBOL_GLOBAL);
		}
	}
else
	sPtr->name = token;


sPtr->context = context;
return(sPtr);
}

/* ------------------------- dump RB tree info of a symbol -------------------- */

#ifdef SYMBOL_DEBUG
CELL * p_dumpSymbol(CELL * params)
{
char * name;
SYMBOL * sPtr;

getString(params, &name);

sPtr = findInsertSymbol(name, LOOKUP_ONLY);

if(sPtr == NULL)
	return(nilCell);

varPrintf(OUT_DEVICE, "name=%s color=%s parent=%s left=%s right=%s\n", 
	sPtr->name,
	(sPtr->color == RED) ? "red" : "black",
	(sPtr->parent) ? sPtr->parent->name : "ROOT",
	sPtr->left->name,
	sPtr->right->name);

return(trueCell);
}
#endif



/* ----------------------------- delete a symbol --------------------------- */
int references(SYMBOL * sPtr, int replaceFlag);

CELL * p_deleteSymbol(CELL * params)
{
SYMBOL * sPtr;
CELL * cell;

cell = evaluateExpression(params);
if(cell->type == CELL_SYMBOL || cell->type == CELL_CONTEXT)
	sPtr = (SYMBOL*)cell->contents;
else if(cell->type == CELL_DYN_SYMBOL)
	sPtr = getDynamicSymbol(cell);
else return(errorProcExt(ERR_SYMBOL_OR_CONTEXT_EXPECTED, params));

if(sPtr == mainContext) return(nilCell);

if(symbolType(sPtr) == CELL_CONTEXT)
	{
 	if(cell->type == CELL_SYMBOL) 
		cell = (CELL*)sPtr->contents;
	sPtr->flags &= ~SYMBOL_PROTECTED;
	}

if(sPtr->flags & (SYMBOL_PROTECTED | SYMBOL_BUILTIN) )
	return(nilCell);

if(getFlag(params->next))
	{
	if(references(sPtr, FALSE) > 1)
		return(nilCell);
	}

if(cell->type == CELL_CONTEXT)
	deleteContextSymbols(cell);	

deleteFreeSymbol(sPtr);

return(trueCell);
}


void deleteContextSymbols(CELL * cell)
{
SYMBOL * context;
CELL * symbolList;
CELL * nextSymbol;

context = (SYMBOL *)cell->contents;

symbolList = getCell(CELL_EXPRESSION);
nextSymbol = NULL;
collectSymbols((SYMBOL *)((CELL *)context->contents)->aux, symbolList, &nextSymbol);

nextSymbol = (CELL *)symbolList->contents;
while(nextSymbol != nilCell)
	{
	deleteFreeSymbol((SYMBOL*)nextSymbol->contents);
	nextSymbol = nextSymbol->next;
	}
	
deleteList(symbolList);
}



void deleteFreeSymbol(SYMBOL * sPtr)
{
SYMBOL * context;

context = sPtr->context;
root = (SYMBOL *)((CELL *)context->contents)->aux;

if(!deleteSymbol(sPtr->name))
	return;

((CELL *)context->contents)->aux = (UINT)root; /* root may have changed */

deleteList((CELL *)sPtr->contents);

references(sPtr, TRUE);
freeMemory(sPtr->name);
freeMemory(sPtr);
}



void makeContextFromSymbol(SYMBOL * symbol, SYMBOL * treePtr)
{
CELL * contextCell;

contextCell = getCell(CELL_CONTEXT);
contextCell->contents = (UINT)symbol;
contextCell->aux = (UINT)treePtr;
symbol->contents = (UINT)contextCell;
symbol->context = mainContext;
symbol->flags |= (SYMBOL_PROTECTED | SYMBOL_GLOBAL);
}


int references(SYMBOL * sPtr, int replaceFlag)
{
CELL * blockPtr;
int i, count;

blockPtr = cellMemory;
count = 0;
while(blockPtr != NULL)
	{
	for(i = 0; i < MAX_BLOCK; i++)
		{
		if( ( *(UINT *)blockPtr == CELL_SYMBOL && blockPtr->contents == (UINT)sPtr) ||
		    ( *(UINT *)blockPtr == CELL_CONTEXT && blockPtr->contents == (UINT)sPtr) )
			{
			count++;
			if(replaceFlag) blockPtr->contents = (UINT)nilSymbol;
			}
		blockPtr++;
		}
	blockPtr = blockPtr->next;
	}

return(count);
}

CELL * p_name(CELL * params)
{
SYMBOL * sPtr;
CELL * cell;

cell = evaluateExpression(params);
if(cell->type == CELL_SYMBOL || cell->type == CELL_CONTEXT)
	sPtr = (SYMBOL *)cell->contents;
else
	return(errorProcExt(ERR_SYMBOL_OR_CONTEXT_EXPECTED, cell));

if(getFlag(params->next))
	return(stuffString(((SYMBOL*)sPtr->context)->name));
return(stuffString(sPtr->name));
}

/* -------------------------------------------------------------------------

   Red-Black Balanced Binary Tree Algorithm adapted from:

   Thomas Niemann thomasn@epaperpress.com

   See also:
   Thomas H. Cormen, et al
   Introduction to Algorithms
   (MIT Electrical Engineering and Computer Science)
   (C) 1990 MIT Press

*/


#define compLT(a,b) (a < b)
#define compEQ(a,b) (a == b)

#define BLACK 0
#define RED 1

#define NIL_SYM &sentinel	/* all leafs are sentinels */

SYMBOL sentinel = {
	0, 		/* pretty print */
	BLACK,		/* color */
	"NIL",		/* name */
	0,		/* contents */
	NULL,		/* context */
	NULL,		/* parent */
	NIL_SYM,	/* left */
 	NIL_SYM 	/* right */
	};

void rotateLeft(SYMBOL* x);
void rotateRight(SYMBOL * x);
static void insertFixup(SYMBOL * x);
void deleteFixup(SYMBOL *x);

/* --------------------------------------------------------------------

   lookup the symbol with name key, if it does not exist and the
   forceCreation flag is set, create and insert the symbol and
   return a pointer to the new symbol. If the context passed is empty
   then it's treePtr (root) will be the new symbol.

*/


SYMBOL * findInsertSymbol(char * key, int forceCreation) 
{
SYMBOL *current, *parent, *x;

/* find future parent */
current = (root == NULL) ? NIL_SYM : root;
parent = 0;

while (current != NIL_SYM)
	{
	if(strcmp(key, current->name) == 0) /* already exists */
            return(current);

	parent = current;
	current = (strcmp(key, current->name) < 0) ? 
		current->left : current->right;
	}

/* if forceCreation not specified just return */
if(forceCreation == LOOKUP_ONLY) return(NULL);

/* allocate new symbol */
x = (SYMBOL *)callocMemory(sizeof(SYMBOL));

x->parent = parent;
x->left = NIL_SYM;
x->right = NIL_SYM;
x->color = RED;

/* insert node in tree */
if(parent) 
	{
      if(strcmp(key, parent->name) < 0)
            parent->left = x;
      else
            parent->right = x;
	} 
else 
	root = x;

insertFixup(x);


/* return new node */

++symbolCount;
return(x);
}


/* --------------------------------------------------------------------
   extract symbol in context from tree, return 1 if deleted or 0 if it 
   couldn't be found.

*/

int deleteSymbol(char * key)
{
SYMBOL *x, *y, *z;
int color;

/* find node in tree */
z = (root == NULL) ? NIL_SYM : root;

while(z != NIL_SYM)
	{
	if(strcmp(key, z->name) == 0) 
		break;
	else
		z = (strcmp(key, z->name) < 0) ? z->left : z->right;
	}

if (z == NIL_SYM) return(0); /* key to delete not found */


if (z->left == NIL_SYM || z->right == NIL_SYM)
	{
	/* y has a NIL_SYM node as a child */
	y = z;
	}
else 
	{
	/* find tree successor with a NIL_SYM node as a child */
	y = z->right;
	while (y->left != NIL_SYM) y = y->left;
	}

/* x is y's only child */
if (y->left != NIL_SYM)
	x = y->left;
else
	x = y->right;

/* remove y from the parent chain */
x->parent = y->parent;
if (y->parent)
	{
	if (y == y->parent->left)
		y->parent->left = x;
        else
            y->parent->right = x;
	}
else
	root = x;


color = y->color;
if (y != z)
	{
	/* swap y and z */
	y->left = z->left;
	y->right = z->right;
	y->parent = z->parent;

	if(z->parent)
		{
		if(z->parent->left == z)
			z->parent->left = y;
		else
			z->parent->right = y;
		}
	else root = y;

	y->right->parent = y;
	y->left->parent = y;

	y->color = z->color;
	}

if (color == BLACK)
	deleteFixup (x);

--symbolCount;
return TRUE;
}



/* -------------------------------------------------------------------- */

void rotateLeft(SYMBOL* x) 
{
SYMBOL* y;

y = x->right;

/* establish x->right link */
x->right = y->left;
if (y->left != NIL_SYM) 
	y->left->parent = x;

/* establish y->parent link */
if(y != NIL_SYM) 
	y->parent = x->parent;

if (x->parent)
	{
	if (x == x->parent->left)
      	x->parent->left = y;
      else
      	x->parent->right = y;
	} 
else 
	root = y;


/* link x and y */
y->left = x;
if (x != NIL_SYM) 
	x->parent = y;
}


void rotateRight(SYMBOL * x)
{
SYMBOL * y;

y = x->left;

/* establish x->left link */
x->left = y->right;
if (y->right != NIL_SYM)
	y->right->parent = x;

/* establish y->parent link */
if (y != NIL_SYM) 
	y->parent = x->parent;

if (x->parent) 
	{
      if (x == x->parent->right)
            x->parent->right = y;
      else
            x->parent->left = y;
	}
else
	root = y;

/* link x and y */
y->right = x;
if (x != NIL_SYM) 
	x->parent = y;
}


static void insertFixup(SYMBOL * x)
{
SYMBOL * y;

/* check Red-Black properties */
while (x != root && x->parent->color == RED)
	{
	/* we have a violation */
	if (x->parent == x->parent->parent->left)
		{
        y = x->parent->parent->right;
        if (y->color == RED) 
			{
			/* uncle is RED */
			x->parent->color = BLACK;
			y->color = BLACK;
			x->parent->parent->color = RED;
			x = x->parent->parent;
			} 
		else 
			{
           	/* uncle is BLACK */
           	if (x == x->parent->right)
				{
           		/* make x a left child */
           		x = x->parent;
           		rotateLeft(x);
				}

			/* recolor and rotate */
			x->parent->color = BLACK;
			x->parent->parent->color = RED;
			rotateRight(x->parent->parent);
           }
		} 
	else 
		{

		/* mirror image of above code */
		y = x->parent->parent->left;
		if (y->color == RED) 
			{
			/* uncle is RED */
			x->parent->color = BLACK;
			y->color = BLACK;
			x->parent->parent->color = RED;
			x = x->parent->parent;
			} 
		else 
			{
			/* uncle is BLACK */
			if (x == x->parent->left) 
				{
				x = x->parent;
				rotateRight(x);
				}
			x->parent->color = BLACK;
			x->parent->parent->color = RED;
			rotateLeft(x->parent->parent);
			}
		}
	}

root->color = BLACK;
}


void deleteFixup(SYMBOL *x)
{
SYMBOL * w;

while (x != root && x->color == BLACK)
	{
	if (x == x->parent->left)
		{
            w = x->parent->right;
            if (w->color == RED)
			{
			w->color = BLACK;
			x->parent->color = RED;
			rotateLeft (x->parent);
			w = x->parent->right;
			}
            if (w->left->color == BLACK && w->right->color == BLACK)
			{
			w->color = RED;
			x = x->parent;
			} 
		else 
			{
			if (w->right->color == BLACK)
				{
				w->left->color = BLACK;
				w->color = RED;
				rotateRight (w);
				w = x->parent->right;
				}
			w->color = x->parent->color;
			x->parent->color = BLACK;
			w->right->color = BLACK;
			rotateLeft (x->parent);
			x = root;
			}
		} 
	else 
		{
            w = x->parent->left;
            if (w->color == RED)
			{
			w->color = BLACK;
			x->parent->color = RED;
			rotateRight (x->parent);
			w = x->parent->left;
			}
            if (w->right->color == BLACK && w->left->color == BLACK)
			{
			w->color = RED;
			x = x->parent;
			} 
		else 
			{
			if (w->left->color == BLACK)
				{
				w->right->color = BLACK;
				w->color = RED;
				rotateLeft (w);
				w = x->parent->left;
				}
			w->color = x->parent->color;
			x->parent->color = BLACK;
			w->left->color = BLACK;
			rotateRight (x->parent);
			x = root;
			}
		}
	}

x->color = BLACK;
}


/* eof */


