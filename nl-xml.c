/* nl-xml.c - newLISP XML interface 
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

#define XML_NONE 0
#define XML_TEXT 1
#define XML_CDATA 2
#define XML_COMMENT 3
#define XML_ELEMENT 4

int isWhiteSpaceStringN(char * source, int tagPos);
CELL * makeTagSymbolCell(char * tagStart, int tagLen);

char * typeNames[] =
  {
  "none",
  "TEXT",
  "CDATA",
  "COMMENT",
  "ELEMENT"
  };

CELL typeCells[5] = {{0}, {0}, {0}, {0}, {0}};

static char * xmlError;
static char xmlMsg[64];
static char * sourceOrg;
static char * source;

static SYMBOL * XMLcontext;

UINT optionsFlag;
#define OPTION_NO_OPTION 0
#define OPTION_NO_WHITESPACE 1
#define OPTION_NO_EMPTY_ATTRIBUTES 2
#define OPTION_NO_COMMENTS 4
#define OPTION_TAGS_TO_SYMBOLS 8
#define OPTION_SXML_ATTRIBUTES 16


typedef struct
	{
	char * name;
	void * next;
	} TAG_STACK;

TAG_STACK * tagStack = NULL;

/* setup type tage default cells, if done already just relink */
CELL * setupTypeTagCells(void)
{
int i;

/* if never done, initialize defaults */
if(typeCells[0].contents == 0)
    for(i = 0; i < 5; i++)
    {
    typeCells[i].type = (i == 0) ? CELL_EXPRESSION : CELL_STRING;
    typeCells[i].next = nilCell;
    typeCells[i].aux = (i == 0) ? (UINT)nilCell : strlen(typeNames[i]) + 1;
    typeCells[i].contents = (UINT)typeNames[i];
    }

/* link cells in a list */
typeCells[0].contents = (UINT)&typeCells[1];
for(i = 1; i < 4; i++)
    typeCells[i].next = &typeCells[i+1];

return(&typeCells[0]);
}

CELL * p_XMLparse(CELL * params)
{
CELL * result;

params = getString(params, &source);
if(params != nilCell)
    params = getInteger(params, &optionsFlag);
else optionsFlag = OPTION_NO_OPTION;

if(params != nilCell)
	XMLcontext = getCreateContext(params, TRUE);	
else
	XMLcontext = currentContext;
    
setupTypeTagCells();

xmlError = NULL;
sourceOrg = source;
deleteTagStack();

result = parseDoc();
deleteTagStack();

if(xmlError != NULL)
	return nilCell;
else
	return result;
}


CELL * p_XMLtypeTags(CELL * params)
{
int i;
CELL * cell;

if(params == nilCell) 
    return(copyCell(setupTypeTagCells()));

setupTypeTagCells();

for(i = 1; i < 5; i++)
    {
    cell = evaluateExpression(params);
    memcpy(&typeCells[i], cell, sizeof(CELL));
    params = params->next;
    }

return(copyCell(setupTypeTagCells()));
}


CELL * p_XMLerror(CELL * params)
{
CELL * errorCell;
CELL * cell;

if(xmlError == NULL)
	return(nilCell);

errorCell = getCell(CELL_EXPRESSION);
cell = stuffString(xmlError);
errorCell->contents = (UINT)cell;
cell->next = stuffInteger((UINT)(source - sourceOrg));

return errorCell;
}

void deleteTagStack(void)
{
TAG_STACK * oldTagStack;

while(tagStack != NULL)
	{
	oldTagStack = tagStack;
	freeMemory(tagStack->name);
	tagStack = tagStack->next;
	freeMemory(oldTagStack);
	}
}


CELL * parseDoc(void)
{
CELL * node;
CELL * lastNode;
int closingFlag = FALSE;
int tagPos;

lastNode = node = getCell(CELL_EXPRESSION);

while(!xmlError && !closingFlag)
	{
	if((tagPos = find("<", source)) == -1) break;
	if(tagPos > 0)
		{
		if( (tagStack != NULL) || (node->contents != (UINT)nilCell))
			{
			if((optionsFlag & OPTION_NO_WHITESPACE) && isWhiteSpaceStringN(source, tagPos))
                		{;}
			else lastNode = appendNode(lastNode, makeTextNode(XML_TEXT, stuffStringN(source, tagPos)));
			}
		source = source + tagPos;
		}

	if(strncmp(source, "<!DOCTYPE", 9) == 0)
		{
		parseDTD();
		continue;
		}

	if(*source == '<' && *(source + 1) == '?')
		{
		parseProcessingInstruction();
		continue;
		}

	if(memcmp(source, "<!--", 4) == 0)
		{
		if(optionsFlag & OPTION_NO_COMMENTS)
			parseTag("-->");
		else
			lastNode = appendNode(lastNode, parseTag("-->"));
		continue;
		}
	if(memcmp(source, "<![CDATA[", 9) == 0)
		{
		lastNode = appendNode(lastNode, parseTag("]]>"));
		continue;
		}

	if(*source == '<' && *(source + 1) == '/')
		{
		closingFlag = TRUE;
		parseClosing();
		continue;
		}

	lastNode = appendNode(lastNode, parseTag(">"));
	}


if(xmlError != NULL)
	{
	deleteList(node);
	return nilCell;
	}

return node;
}


void parseDTD(void)
{
int closeTag, squareTag;
int closePos = 0;
char * closeTagStr;

if((closeTag = find(">", source)) == -1)
	{
	xmlError = "error in DTD: expected '>'";
	return;
	}

squareTag = find("[", source);
if(squareTag != -1 && squareTag < closeTag)
	closeTagStr = "]>";
else
	closeTagStr = ">";

while(!xmlError)
	{
	if((closePos = find(closeTagStr, source)) == -1)
		{
		snprintf(xmlMsg, 63, "expected: %s", closeTagStr);
		xmlError = xmlMsg;
		return;
		}
	if(*(source + closePos - 1) != ']')
		break;
	source = source + closePos + strlen(closeTagStr);
	}

source = source + closePos + strlen(closeTagStr);
return;
}


void parseProcessingInstruction(void)
{
int closeTag;

if((closeTag = find("?>", source)) == -1)
	{
	xmlError = "expecting closing tag sequence '?>'";
	return;
	}

source = source + closeTag + 2;
}


void parseClosing(void)
{
int closeTag;
char * tagName;
TAG_STACK * oldTagStack;

if((closeTag = find(">", source)) == -1)
	{
	xmlError = "missing closing >";
	return;
	}

if(tagStack == NULL)
	{
	xmlError = "closing tag has no opening";
	return;
	}

tagName = tagStack->name;
if(strncmp(source + 2, tagName, strlen(tagName)) != 0)
	{
	xmlError = "closing tag doesn't match";
	return;
	}

/* pop tagStack */
freeMemory(tagName);
oldTagStack = tagStack;
tagStack = tagStack->next;

freeMemory(oldTagStack);

source = source + closeTag + 1;
}


CELL * parseTag(char * closeTagStr)
{
char * newSrc;
int closeTag;
CELL * cell;

cell = NULL;

closeTag = find(closeTagStr, source);
if(*(source + closeTag - 1) == '/')
	{
	if(memcmp(closeTagStr,"]]>",3) != 0)
		{
		--closeTag;
		closeTagStr = "/>";
		}
	}

if(closeTag == -1)
	{
	snprintf(xmlMsg, 63, "expected closing tag: %s", closeTagStr);
	xmlError = xmlMsg;
	return nilCell;
	}

if(memcmp(source, "<!--", 4) == 0)
	{
	if(optionsFlag & OPTION_NO_COMMENTS)
		cell = nilCell;
	else
		{
		cell = stuffStringN(source + 4, closeTag - 4);
		cell = makeTextNode(XML_COMMENT, cell);
		}
	}

if(memcmp(source, "<![CDATA[", 9) == 0)
	{
	cell = stuffStringN(source + 9, closeTag - 9);
	cell = makeTextNode(XML_CDATA, cell);
	}

if(*source == '<' && *(source + 1) == '/')
	{
	xmlError = "closing node has no opening";
	return nilCell;
	}

newSrc = source + closeTag + strlen(closeTagStr);

if(cell == NULL)
	cell = parseNormalTag(source + closeTag, newSrc);
else
	source = newSrc;


return(cell);
}


CELL * parseNormalTag(char * endSrc, char * newSrc)
{
char * tagStart;
int tagLen;
CELL * attributes;
CELL * childs;
CELL * tagCell;
TAG_STACK * tag;

++source; /* skip '/' */

while(*source <= ' ' && source < endSrc) ++source; /* skip whitespace */

tagStart = source;
tagLen = 0;
while(*source > ' ' && source < endSrc) ++source, ++tagLen; /* find tag end */

attributes = parseAttributes(endSrc);
if(optionsFlag & OPTION_SXML_ATTRIBUTES)
	{
	childs = (CELL*)attributes->contents;
	if(! (childs == nilCell && (optionsFlag & OPTION_NO_EMPTY_ATTRIBUTES)))
		{
		attributes->contents = (UINT)stuffSymbol(atSymbol);
		((CELL*)(attributes->contents))->next = childs;
		}
	}

if(xmlError) 
	return nilCell;

if(*source == '/' && *(source + 1) == '>')
	{
	source = newSrc;
	if(optionsFlag & OPTION_TAGS_TO_SYMBOLS)
		tagCell = makeTagSymbolCell(tagStart, tagLen);
	else
		tagCell = stuffStringN(tagStart, tagLen);
	return makeElementNode(tagCell, attributes, getCell(CELL_EXPRESSION));
	}

/* push tag on tagstack */
tag = (TAG_STACK*)allocMemory(sizeof(TAG_STACK));
tag->name = (char *)callocMemory(tagLen + 1);
memcpy(tag->name, tagStart, tagLen);
tag->next = tagStack;
tagStack = tag;

source = newSrc;
childs = parseDoc();

if(optionsFlag & OPTION_TAGS_TO_SYMBOLS)
    tagCell = makeTagSymbolCell(tagStart, tagLen);
else
    tagCell = stuffStringN(tagStart, tagLen);

return makeElementNode(tagCell, attributes, childs);
}


CELL * makeTagSymbolCell(char * tagStart, int tagLen)
{
char * name;
CELL * cell;

name = (char *)callocMemory(tagLen + 1);
memcpy(name, tagStart, tagLen);
cell = stuffSymbol(translateCreateSymbol(name, CELL_NIL, XMLcontext, 0));
freeMemory(name);
return(cell);
}


CELL * parseAttributes(char * endSrc)
{
CELL * attributes;
CELL * att;
CELL * cell;
CELL * lastAtt;
char * namePos;
char * valPos;
char quoteChar;
int  nameLen, valLen;

attributes = getCell(CELL_EXPRESSION);
lastAtt = NULL;

while(!xmlError && source < endSrc)
	{
	while(*source <= ' ' && source < endSrc) source++; /* strip leading space */
	namePos = source;
	nameLen = 0;
	while(*source > ' ' && *source != '=' && source < endSrc) source++, nameLen++; /* get end */
	if(nameLen == 0) break;
	while(*source <= ' ' && source < endSrc) source++; /* strip leading space */
	if(*source != '=')
		{
		xmlError = "expected '=' in attributes";
		deleteList(attributes);
		return nilCell;
		}
	else source++;
	while(*source <= ' ' && source < endSrc) source++; /* strip spaces */
	if(*source != '\"' && *source != '\'')
		{
		xmlError = "attribute values must be delimited by \" or \' ";
		deleteList(attributes);
		return nilCell;
		}
	quoteChar = *source;
	source++;
	valPos = source;
	valLen = 0;
	while(*source != quoteChar && source < endSrc) source++, valLen++;
	if(*source != quoteChar) valLen = -1;
	else source++;
	if(nameLen == 0 || valLen == -1)
		{
		xmlError = "incorrect attribute";
		deleteList(attributes);
		return nilCell;
		}
	att = getCell(CELL_EXPRESSION);
	if(optionsFlag & OPTION_TAGS_TO_SYMBOLS)
        	cell = makeTagSymbolCell(namePos, nameLen);
	else
		cell = stuffStringN(namePos, nameLen);
	cell->next = stuffStringN(valPos, valLen);
	att->contents = (UINT)cell;
	if(lastAtt == NULL)
		attributes->contents = (UINT)att;
	else 
		lastAtt->next = att;
	lastAtt = att;
	}

return attributes;
}


CELL * appendNode(CELL * node, CELL * newNode)
{
if(node->contents == (UINT)nilCell)
	node->contents = (UINT)newNode;
else
	node->next = newNode;

return newNode;
}


CELL * makeTextNode(int type, CELL * contents)
{
CELL * newNode;
CELL * cell;

/* unwrap text node if nil xml-type-tag */
if(typeCells[type].type == CELL_NIL)
    return(contents);

newNode = getCell(CELL_EXPRESSION);
cell = copyCell(&typeCells[type]);
newNode->contents = (UINT)cell;
cell->next = contents;

return newNode;
}


CELL * makeElementNode(CELL * tagNode, CELL * attributesNode, CELL * childrenNode)
{
CELL * newNode;
CELL * cell;

/* unwrap children node, if nil in xml-type-tag */
if(typeCells[XML_ELEMENT].type == CELL_NIL)
    {
    cell = childrenNode;
    childrenNode = (CELL *)childrenNode->contents;
    cell->contents = (UINT)nilCell;
    deleteList(cell);
    }

newNode = getCell(CELL_EXPRESSION);
if(typeCells[XML_ELEMENT].type == CELL_NIL)
    newNode->contents = (UINT)tagNode;
else
    {
    cell = copyCell(&typeCells[XML_ELEMENT]);
    newNode->contents = (UINT)cell;
    cell->next = tagNode;
    }

if( (attributesNode->contents == (UINT)nilCell) && 
    (optionsFlag & OPTION_NO_EMPTY_ATTRIBUTES))
    {
    tagNode->next = childrenNode;
    deleteList(attributesNode);
    }
else
    {
    tagNode->next = attributesNode;
    attributesNode->next = childrenNode;
    }

return newNode;
}
	

int find(char * key, char * source)
{
char * ptr;

ptr = strstr(source, key);
if(ptr == NULL) return -1;

return(ptr - source);
}
	

int isWhiteSpaceStringN(char * source, int tagPos)
{
while(tagPos--) if((unsigned char)*source++ > 32) return(FALSE);
return(TRUE);
}

/* eof */





