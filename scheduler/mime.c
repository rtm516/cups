/*
 * "$Id: mime.c 4970 2006-01-24 14:05:45Z mike $"
 *
 *   MIME database file routines for the Common UNIX Printing System (CUPS).
 *
 *   Copyright 1997-2006 by Easy Software Products, all rights reserved.
 *
 *   These coded instructions, statements, and computer programs are the
 *   property of Easy Software Products and are protected by Federal
 *   copyright law.  Distribution and use rights are outlined in the file
 *   "LICENSE.txt" which should have been included with this file.  If this
 *   file is missing or damaged please contact Easy Software Products
 *   at:
 *
 *       Attn: CUPS Licensing Information
 *       Easy Software Products
 *       44141 Airport View Drive, Suite 204
 *       Hollywood, Maryland 20636 USA
 *
 *       Voice: (301) 373-9600
 *       EMail: cups-info@cups.org
 *         WWW: http://www.cups.org
 *
 * Contents:
 *
 *   mimeDelete()       - Delete (free) a MIME database.
 *   mimeDeleteFilter() - Delete a filter from the MIME database.
 *   mimeDeleteType()   - Delete a type from the MIME database.
 *   mimeFirstFilter()  - Get the first filter in the MIME database.
 *   mimeFirstType()    - Get the first type in the MIME database.
 *   mimeNextType()     - Get the next type in the MIME database.
 *   mimeLoad()         - Create a new MIME database from disk.
 *   mimeMerge()        - Merge a MIME database from disk with the current one.
 *   mimeNew()          - Create a new, empty MIME database.
 *   mimeNextFilter()   - Get the next filter in the MIME database.
 *   mimeNextType()     - Get the next type in the MIME database.
 *   mimeNumFilters()   - Get the number of filters in a MIME database.
 *   mimeNumTypes()     - Get the number of types in a MIME database.
 *   load_types()       - Load a xyz.types file...
 *   delete_rules()     - Free all memory for the given rule tree.
 *   load_convs()       - Load a xyz.convs file...
 */

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <cups/dir.h>
#include <cups/string.h>
#include "mime.h"


/*
 * Local functions...
 */

static void	load_types(mime_t *mime, const char *filename);
static void	load_convs(mime_t *mime, const char *filename,
		           const char *filterpath);
static void	delete_rules(mime_magic_t *rules);


/*
 * 'mimeDelete()' - Delete (free) a MIME database.
 */

void
mimeDelete(mime_t *mime)		/* I - MIME database */
{
  mime_type_t	*type;			/* Current type */
  mime_filter_t	*filter;		/* Current filter */


  if (!mime)
    return;

 /*
  * Loop through the file types and delete any rules...
  */

  for (type = (mime_type_t *)cupsArrayFirst(mime->types);
       type;
       type = (mime_type_t *)cupsArrayNext(mime->types))
    mimeDeleteType(mime, type);

 /*
  * Loop through filters and free them...
  */

  for (filter = (mime_filter_t *)cupsArrayFirst(mime->filters);
       filter;
       filter = (mime_filter_t *)cupsArrayNext(mime->filters))
    mimeDeleteFilter(mime, filter);

 /*
  * Free the types and filters arrays, and then the MIME database structure.
  */

  cupsArrayDelete(mime->types);
  cupsArrayDelete(mime->filters);
  free(mime);
}


/*
 * 'mimeDeleteFilter()' - Delete a filter from the MIME database.
 */

void
mimeDeleteFilter(mime_t        *mime,	/* I - MIME database */
		 mime_filter_t *filter)	/* I - Filter */
{
  if (!mime || !filter)
    return;

  cupsArrayRemove(mime->filters, filter);
  free(filter);
}


/*
 * 'mimeDeleteType()' - Delete a type from the MIME database.
 */

void
mimeDeleteType(mime_t      *mime,	/* I - MIME database */
	       mime_type_t *mt)		/* I - Type */
{
  if (!mime || !mt)
    return;

  cupsArrayRemove(mime->types, mt);

  delete_rules(mt->rules);
  free(mt);
}


/*
 * 'mimeFirstFilter()' - Get the first filter in the MIME database.
 */

mime_filter_t *				/* O - Filter or NULL */
mimeFirstFilter(mime_t *mime)		/* I - MIME database */
{
  if (!mime)
    return (NULL);
  else
    return ((mime_filter_t *)cupsArrayFirst(mime->filters));
}


/*
 * 'mimeFirstType()' - Get the first type in the MIME database.
 */

mime_type_t *				/* O - Type or NULL */
mimeFirstType(mime_t *mime)		/* I - MIME database */
{
  if (!mime)
    return (NULL);
  else
    return ((mime_type_t *)cupsArrayFirst(mime->types));
}


/*
 * 'mimeLoad()' - Create a new MIME database from disk.
 */

mime_t *				/* O - New MIME database */
mimeLoad(const char *pathname,		/* I - Directory to load */
         const char *filterpath)	/* I - Directory to load */
{
  return (mimeMerge(NULL, pathname, filterpath));
}


/*
 * 'mimeMerge()' - Merge a MIME database from disk with the current one.
 */

mime_t *				/* O - Updated MIME database */
mimeMerge(mime_t     *mime,		/* I - MIME database to add to */
          const char *pathname,		/* I - Directory to load */
          const char *filterpath)	/* I - Directory to load */
{
  cups_dir_t	*dir;			/* Directory */
  cups_dentry_t	*dent;			/* Directory entry */
  char		filename[1024];		/* Full filename of types/converts file */


 /*
  * First open the directory specified by pathname...  Return NULL if nothing
  * was read or if the pathname is NULL...
  */

  if (!pathname)
    return (NULL);

  if ((dir = cupsDirOpen(pathname)) == NULL)
    return (NULL);

 /*
  * If "mime" is NULL, make a new, blank database...
  */

  if (!mime)
    mime = mimeNew();
  if (!mime)
    return (NULL);

 /*
  * Read all the .types files...
  */

  while ((dent = cupsDirRead(dir)) != NULL)
  {
    if (strlen(dent->filename) > 6 &&
        !strcmp(dent->filename + strlen(dent->filename) - 6, ".types"))
    {
     /*
      * Load a mime.types file...
      */

      snprintf(filename, sizeof(filename), "%s/%s", pathname, dent->filename);
      load_types(mime, filename);
    }
  }

  cupsDirRewind(dir);

 /*
  * Read all the .convs files...
  */

  while ((dent = cupsDirRead(dir)) != NULL)
  {
    if (strlen(dent->filename) > 6 &&
        !strcmp(dent->filename + strlen(dent->filename) - 6, ".convs"))
    {
     /*
      * Load a mime.convs file...
      */

      snprintf(filename, sizeof(filename), "%s/%s", pathname, dent->filename);
      load_convs(mime, filename, filterpath);
    }
  }

  cupsDirClose(dir);

  return (mime);
}


/*
 * 'mimeNew()' - Create a new, empty MIME database.
 */

mime_t *				/* O - MIME database */
mimeNew(void)
{
  return ((mime_t *)calloc(1, sizeof(mime_t)));
}


/*
 * 'mimeNextFilter()' - Get the next filter in the MIME database.
 */

mime_filter_t *				/* O - Filter or NULL */
mimeNextFilter(mime_t *mime)		/* I - MIME database */
{
  if (!mime)
    return (NULL);
  else
    return ((mime_filter_t *)cupsArrayNext(mime->filters));
}


/*
 * 'mimeNextType()' - Get the next type in the MIME database.
 */

mime_type_t *				/* O - Type or NULL */
mimeNextType(mime_t *mime)		/* I - MIME database */
{
  if (!mime)
    return (NULL);
  else
    return ((mime_type_t *)cupsArrayNext(mime->types));
}


/*
 * 'mimeNumFilters()' - Get the number of filters in a MIME database.
 */

int
mimeNumFilters(mime_t *mime)		/* I - MIME database */
{
  if (!mime)
    return (0);
  else
    return (cupsArrayCount(mime->filters));
}


/*
 * 'mimeNumTypes()' - Get the number of types in a MIME database.
 */

int
mimeNumTypes(mime_t *mime)		/* I - MIME database */
{
  if (!mime)
    return (0);
  else
    return (cupsArrayCount(mime->types));
}


/*
 * 'load_types()' - Load a xyz.types file...
 */

static void
load_types(mime_t     *mime,		/* I - MIME database */
           const char *filename)	/* I - Types file to load */
{
  cups_file_t	*fp;			/* Types file */
  int		linelen;		/* Length of line */
  char		line[65536],		/* Input line from file */
		*lineptr,		/* Current position in line */
		super[MIME_MAX_SUPER],	/* Super-type name */
		type[MIME_MAX_TYPE],	/* Type name */
		*temp;			/* Temporary pointer */
  mime_type_t	*typeptr;		/* New MIME type */


 /*
  * First try to open the file...
  */

  if ((fp = cupsFileOpen(filename, "r")) == NULL)
    return;

 /*
  * Then read each line from the file, skipping any comments in the file...
  */

  while (cupsFileGets(fp, line, sizeof(line)) != NULL)
  {
   /*
    * Skip blank lines and lines starting with a #...
    */

    if (!line[0] || line[0] == '#')
      continue;

   /*
    * While the last character in the line is a backslash, continue on to the
    * next line (and the next, etc.)
    */

    linelen = strlen(line);

    while (line[linelen - 1] == '\\')
    {
      linelen --;

      if (cupsFileGets(fp, line + linelen, sizeof(line) - linelen) == NULL)
        line[linelen] = '\0';
      else
        linelen += strlen(line + linelen);
    }

   /*
    * Extract the super-type and type names from the beginning of the line.
    */

    lineptr = line;
    temp    = super;

    while (*lineptr != '/' && *lineptr != '\n' && *lineptr != '\0' &&
           (temp - super + 1) < MIME_MAX_SUPER)
      *temp++ = tolower(*lineptr++ & 255);

    *temp = '\0';

    if (*lineptr != '/')
      continue;

    lineptr ++;
    temp = type;

    while (*lineptr != ' ' && *lineptr != '\t' && *lineptr != '\n' &&
           *lineptr != '\0' && (temp - type + 1) < MIME_MAX_TYPE)
      *temp++ = tolower(*lineptr++ & 255);

    *temp = '\0';

   /*
    * Add the type and rules to the MIME database...
    */

    typeptr = mimeAddType(mime, super, type);
    mimeAddTypeRule(typeptr, lineptr);
  }

  cupsFileClose(fp);
}


/*
 * 'load_convs()' - Load a xyz.convs file...
 */

static void
load_convs(mime_t     *mime,		/* I - MIME database */
           const char *filename,	/* I - Convs file to load */
           const char *filterpath)	/* I - Path for filters */
{
  cups_file_t	*fp;			/* Convs file */
  char		line[1024],		/* Input line from file */
		*lineptr,		/* Current position in line */
		super[MIME_MAX_SUPER],	/* Super-type name */
		type[MIME_MAX_TYPE],	/* Type name */
		*temp,			/* Temporary pointer */
		*filter;		/* Filter program */
  mime_type_t	*temptype,		/* MIME type looping var */
		*dsttype;		/* Destination MIME type */
  int		cost;			/* Cost of filter */
#ifndef WIN32
  char		filterprog[1024];	/* Full path of filter... */
#endif /* !WIN32 */


 /*
  * First try to open the file...
  */

  if ((fp = cupsFileOpen(filename, "r")) == NULL)
    return;

 /*
  * Then read each line from the file, skipping any comments in the file...
  */

  while (cupsFileGets(fp, line, sizeof(line)) != NULL)
  {
   /*
    * Skip blank lines and lines starting with a #...
    */

    if (!line[0] || line[0] == '#')
      continue;

   /*
    * Strip trailing whitespace...
    */

    for (lineptr = line + strlen(line) - 1;
         lineptr >= line && isspace(*lineptr & 255);
	 lineptr --)
      *lineptr = '\0';

   /*
    * Extract the destination super-type and type names from the middle of
    * the line.
    */

    lineptr = line;
    while (*lineptr != ' ' && *lineptr != '\t' && *lineptr != '\0')
      lineptr ++;

    while (*lineptr == ' ' || *lineptr == '\t')
      lineptr ++;

    temp = super;

    while (*lineptr != '/' && *lineptr != '\n' && *lineptr != '\0' &&
           (temp - super + 1) < MIME_MAX_SUPER)
      *temp++ = tolower(*lineptr++ & 255);

    *temp = '\0';

    if (*lineptr != '/')
      continue;

    lineptr ++;
    temp = type;

    while (*lineptr != ' ' && *lineptr != '\t' && *lineptr != '\n' &&
           *lineptr != '\0' && (temp - type + 1) < MIME_MAX_TYPE)
      *temp++ = tolower(*lineptr++ & 255);

    *temp = '\0';

    if (*lineptr == '\0' || *lineptr == '\n')
      continue;

    if ((dsttype = mimeType(mime, super, type)) == NULL)
      continue;

   /*
    * Then get the cost and filter program...
    */

    while (*lineptr == ' ' || *lineptr == '\t')
      lineptr ++;

    if (*lineptr < '0' || *lineptr > '9')
      continue;

    cost = atoi(lineptr);

    while (*lineptr != ' ' && *lineptr != '\t' && *lineptr != '\0')
      lineptr ++;
    while (*lineptr == ' ' || *lineptr == '\t')
      lineptr ++;

    if (*lineptr == '\0' || *lineptr == '\n')
      continue;

    filter = lineptr;

#ifndef WIN32
    if (strcmp(filter, "-"))
    {
     /*
      * Verify that the filter exists and is executable...
      */

      if (filter[0] == '/')
	strlcpy(filterprog, filter, sizeof(filterprog));
      else if (!cupsFileFind(filter, filterpath, filterprog,
                             sizeof(filterprog)))
        continue;

      if (access(filterprog, X_OK))
	continue;
    }
#endif /* !WIN32 */

   /*
    * Finally, get the source super-type and type names from the beginning of
    * the line.  We do it here so we can support wildcards...
    */

    lineptr = line;
    temp    = super;

    while (*lineptr != '/' && *lineptr != '\n' && *lineptr != '\0' &&
           (temp - super + 1) < MIME_MAX_SUPER)
      *temp++ = tolower(*lineptr++ & 255);

    *temp = '\0';

    if (*lineptr != '/')
      continue;

    lineptr ++;
    temp = type;

    while (*lineptr != ' ' && *lineptr != '\t' && *lineptr != '\n' &&
           *lineptr != '\0' && (temp - type + 1) < MIME_MAX_TYPE)
      *temp++ = tolower(*lineptr++ & 255);

    *temp = '\0';

    if (!strcmp(super, "*") && !strcmp(type, "*"))
    {
     /*
      * Force * / * to be "application/octet-stream"...
      */

      strcpy(super, "application");
      strcpy(type, "octet-stream");
    }

   /*
    * Add the filter to the MIME database, supporting wildcards as needed...
    */

    for (temptype = (mime_type_t *)cupsArrayFirst(mime->types);
         temptype;
	 temptype = (mime_type_t *)cupsArrayNext(mime->types))
      if ((super[0] == '*' || !strcmp(temptype->super, super)) &&
          (type[0] == '*' || !strcmp(temptype->type, type)))
	mimeAddFilter(mime, temptype, dsttype, cost, filter);
  }

  cupsFileClose(fp);
}


/*
 * 'delete_rules()' - Free all memory for the given rule tree.
 */

static void
delete_rules(mime_magic_t *rules)	/* I - Rules to free */
{
  mime_magic_t	*next;			/* Next rule to free */


 /*
  * Free the rules list, descending recursively to free any child rules.
  */

  while (rules != NULL)
  {
    next = rules->next;

    if (rules->child != NULL)
      delete_rules(rules->child);

    free(rules);
    rules = next;
  }
}


/*
 * End of "$Id: mime.c 4970 2006-01-24 14:05:45Z mike $".
 */
