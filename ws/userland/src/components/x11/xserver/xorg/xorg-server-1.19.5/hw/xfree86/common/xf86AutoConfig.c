/*
 * Copyright 2003 by David H. Dawes.
 * Copyright 2003 by X-Oz Technologies.
 * Copyright (c) 2013, 2014 Oracle and/or its affiliates. All Rights Reserved.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 *
 * Author: David Dawes <dawes@XFree86.Org>.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86Parser.h"
#include "xf86tokens.h"
#include "xf86Config.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86platformBus.h"
#include "xf86pciBus.h"
#ifdef __sparc__
#include "xf86sbusBus.h"
#endif

#ifdef __sun
#include <sys/visual_io.h>
#include <ctype.h>
#endif

#if (defined(__sparc__) || defined(__sparc))
static Bool MultiSessionConfig (void);
extern int num_total_disp_dev;
#endif

/* Sections for the default built-in configuration. */

#define BUILTIN_DEVICE_NAME \
	"\"Builtin Default %s Device %d\""

#define BUILTIN_DEVICE_SECTION_PRE \
	"Section \"Device\"\n" \
	"\tIdentifier\t" BUILTIN_DEVICE_NAME "\n" \
	"\tDriver\t\"%s\"\n"

#ifdef sun
/* 
 * Allow setting an arbitrary number of options.
 * Each option should be indented with a tab and newline terminated.
 */ 
#define BUILTIN_DEVICE_SECTION_PRE_OPT \
	"Section \"Device\"\n" \
	"\tIdentifier\t" BUILTIN_DEVICE_NAME "\n" \
	"\tDriver\t\"%s\"\n" \
	"%s"
#endif

#define BUILTIN_DEVICE_SECTION_POST \
	"EndSection\n\n"

#define BUILTIN_DEVICE_SECTION \
	BUILTIN_DEVICE_SECTION_PRE \
	BUILTIN_DEVICE_SECTION_POST

#ifdef sun
/* Device section with options */
#define BUILTIN_DEVICE_SECTION_OPT \
	BUILTIN_DEVICE_SECTION_PRE_OPT \
	BUILTIN_DEVICE_SECTION_POST
#endif

#define BUILTIN_SCREEN_NAME \
	"\"Builtin Default %s Screen %d\""

#define BUILTIN_SCREEN_SECTION \
	"Section \"Screen\"\n" \
	"\tIdentifier\t" BUILTIN_SCREEN_NAME "\n" \
	"\tDevice\t" BUILTIN_DEVICE_NAME "\n" \
	"EndSection\n\n"

#define BUILTIN_LAYOUT_SECTION_PRE \
	"Section \"ServerLayout\"\n" \
	"\tIdentifier\t\"Builtin Default Layout\"\n"

#define BUILTIN_LAYOUT_SCREEN_LINE \
	"\tScreen\t" BUILTIN_SCREEN_NAME "\n"

#define BUILTIN_LAYOUT_SECTION_POST \
	"EndSection\n\n"

static const char **builtinConfig = NULL;
static int builtinLines = 0;

static void listPossibleVideoDrivers(char *matches[], int nmatches);

/*
 * A built-in config file is stored as an array of strings, with each string
 * representing a single line.  AppendToConfig() breaks up the string "s"
 * into lines, and appends those lines it to builtinConfig.
 */

static void
AppendToList(const char *s, const char ***list, int *lines)
{
    char *str, *newstr, *p;

    str = xnfstrdup(s);
    for (p = strtok(str, "\n"); p; p = strtok(NULL, "\n")) {
        (*lines)++;
        *list = xnfreallocarray(*list, *lines + 1, sizeof(**list));
        newstr = xnfalloc(strlen(p) + 2);
        strcpy(newstr, p);
        strcat(newstr, "\n");
        (*list)[*lines - 1] = newstr;
        (*list)[*lines] = NULL;
    }
    free(str);
}

static void
FreeList(const char ***list, int *lines)
{
    int i;

    for (i = 0; i < *lines; i++) {
        free((char *) ((*list)[i]));
    }
    free(*list);
    *list = NULL;
    *lines = 0;
}

static void
FreeConfig(void)
{
    FreeList(&builtinConfig, &builtinLines);
}

static void
AppendToConfig(const char *s)
{
    AppendToList(s, &builtinConfig, &builtinLines);
}

Bool
xf86AutoConfig(void)
{
    char *deviceList[20];
    char **p;
    const char **cp;
    char buf[1024];
    ConfigStatus ret;

    /* Make sure config rec is there */
    if (xf86allocateConfig() != NULL) {
        ret = CONFIG_OK;    /* OK so far */
    }
    else {
        xf86Msg(X_ERROR, "Couldn't allocate Config record.\n");
        return FALSE;
    }

#if ((defined(__sparc__) || defined(__sparc)) && defined (sun))
    /* Do not do MultiSessionConfig() when invoked with -isolateDevice option */
    if ((num_total_disp_dev & 0x1000)|| !MultiSessionConfig()) {
#endif

    listPossibleVideoDrivers(deviceList, 20);

    for (p = deviceList; *p; p++) {
#ifdef sun
        const char *nvidia_string = "nvidia";
        const char *nvidia_opt_no_logo = "\tOption \"NoLogo\" \"True\"\n";
        if (!strncmp(*p, nvidia_string, sizeof(nvidia_string)))
            snprintf(buf, sizeof(buf), BUILTIN_DEVICE_SECTION_OPT,
                     *p, 0, *p, nvidia_opt_no_logo);
        else
#endif
        snprintf(buf, sizeof(buf), BUILTIN_DEVICE_SECTION, *p, 0, *p);
        AppendToConfig(buf);
        snprintf(buf, sizeof(buf), BUILTIN_SCREEN_SECTION, *p, 0, *p, 0);
        AppendToConfig(buf);
    }

    AppendToConfig(BUILTIN_LAYOUT_SECTION_PRE);
    for (p = deviceList; *p; p++) {
        snprintf(buf, sizeof(buf), BUILTIN_LAYOUT_SCREEN_LINE, *p, 0);
        AppendToConfig(buf);
    }
    AppendToConfig(BUILTIN_LAYOUT_SECTION_POST);

    for (p = deviceList; *p; p++) {
        free(*p);
    }

#if ((defined(__sparc__) || defined(__sparc)) && defined (sun))
    }
#endif

    xf86MsgVerb(X_DEFAULT, 0,
                "Using default built-in configuration (%d lines)\n",
                builtinLines);

    xf86MsgVerb(X_DEFAULT, 3, "--- Start of built-in configuration ---\n");
    for (cp = builtinConfig; *cp; cp++)
        xf86ErrorFVerb(3, "\t%s", *cp);
    xf86MsgVerb(X_DEFAULT, 3, "--- End of built-in configuration ---\n");

    xf86initConfigFiles();
    xf86setBuiltinConfig(builtinConfig);
    ret = xf86HandleConfigFile(TRUE);
    FreeConfig();

    if (ret != CONFIG_OK)
        xf86Msg(X_ERROR, "Error parsing the built-in default configuration.\n");

    return ret == CONFIG_OK;
}

static void
listPossibleVideoDrivers(char *matches[], int nmatches)
{
    int i;

    for (i = 0; i < nmatches; i++) {
        matches[i] = NULL;
    }
    i = 0;

#ifdef XSERVER_PLATFORM_BUS
    i = xf86PlatformMatchDriver(matches, nmatches);
#endif

#ifdef XSERVER_LIBPCIACCESS
    if (i < (nmatches - 1))
        i += xf86PciMatchDriver(&matches[i], nmatches - i);
#endif

#if defined(__linux__)
    matches[i++] = xnfstrdup("modesetting");
#endif

#if !defined(__sun)
    /* Fallback to platform default frame buffer driver */
    if (i < (nmatches - 1)) {
#if !defined(__linux__) && defined(__sparc__)
        matches[i++] = xnfstrdup("wsfb");
#else
        matches[i++] = xnfstrdup("fbdev");
#endif
    }
#endif                          /* !__sun */

    /* Fallback to platform default hardware */
    if (i < (nmatches - 1)) {
#if defined(__i386__) || defined(__amd64__) || defined(__hurd__)
        matches[i++] = xnfstrdup("vesa");
#elif defined(__sparc__) && !defined(__sun)
        matches[i++] = xnfstrdup("sunffb");
#endif
    }
}

/* copy a screen section and enter the desired driver
 * and insert it at i in the list of screens */
static Bool
copyScreen(confScreenPtr oscreen, GDevPtr odev, int i, char *driver)
{
    confScreenPtr nscreen;
    GDevPtr cptr = NULL;
    char *identifier;

    nscreen = malloc(sizeof(confScreenRec));
    if (!nscreen)
        return FALSE;
    memcpy(nscreen, oscreen, sizeof(confScreenRec));

    cptr = malloc(sizeof(GDevRec));
    if (!cptr) {
        free(nscreen);
        return FALSE;
    }
    memcpy(cptr, odev, sizeof(GDevRec));

    if (asprintf(&identifier, "Autoconfigured Video Device %s", driver)
        == -1) {
        free(cptr);
        free(nscreen);
        return FALSE;
    }
    cptr->driver = driver;
    cptr->identifier = identifier;

    xf86ConfigLayout.screens[i].screen = nscreen;

    /* now associate the new driver entry with the new screen entry */
    xf86ConfigLayout.screens[i].screen->device = cptr;
    cptr->myScreenSection = xf86ConfigLayout.screens[i].screen;

    return TRUE;
}

GDevPtr
autoConfigDevice(GDevPtr preconf_device)
{
    GDevPtr ptr = NULL;
    char *matches[20];          /* If we have more than 20 drivers we're in trouble */
    int num_matches = 0, num_screens = 0, i;
    screenLayoutPtr slp;

    if (!xf86configptr) {
        return NULL;
    }

    /* If there's a configured section with no driver chosen, use it */
    if (preconf_device) {
        ptr = preconf_device;
    }
    else {
        ptr = calloc(1, sizeof(GDevRec));
        if (!ptr) {
            return NULL;
        }
        ptr->chipID = -1;
        ptr->chipRev = -1;
        ptr->irq = -1;

        ptr->active = TRUE;
        ptr->claimed = FALSE;
        ptr->identifier = "Autoconfigured Video Device";
        ptr->driver = NULL;
    }
    if (!ptr->driver) {
        /* get all possible video drivers and count them */
        listPossibleVideoDrivers(matches, 20);
        for (; matches[num_matches]; num_matches++) {
            xf86Msg(X_DEFAULT, "Matched %s as autoconfigured driver %d\n",
                    matches[num_matches], num_matches);
        }

        slp = xf86ConfigLayout.screens;
        if (slp) {
            /* count the number of screens and make space for
             * a new screen for each additional possible driver
             * minus one for the already existing first one
             * plus one for the terminating NULL */
            for (; slp[num_screens].screen; num_screens++);
            xf86ConfigLayout.screens = xnfcalloc(num_screens + num_matches,
                                                 sizeof(screenLayoutRec));
            xf86ConfigLayout.screens[0] = slp[0];

            /* do the first match and set that for the original first screen */
            ptr->driver = matches[0];
            if (!xf86ConfigLayout.screens[0].screen->device) {
                xf86ConfigLayout.screens[0].screen->device = ptr;
                ptr->myScreenSection = xf86ConfigLayout.screens[0].screen;
            }

            /* for each other driver found, copy the first screen, insert it
             * into the list of screens and set the driver */
            for (i = 1; i < num_matches; i++) {
                if (!copyScreen(slp[0].screen, ptr, i, matches[i]))
                    return NULL;
            }

            /* shift the rest of the original screen list
             * to the end of the current screen list
             *
             * TODO Handle rest of multiple screen sections */
            for (i = 1; i < num_screens; i++) {
                xf86ConfigLayout.screens[i + num_matches] = slp[i];
            }
            xf86ConfigLayout.screens[num_screens + num_matches - 1].screen =
                NULL;
            free(slp);
        }
        else {
            /* layout does not have any screens, not much to do */
            ptr->driver = matches[0];
            for (i = 1; matches[i]; i++) {
                if (matches[i] != matches[0]) {
                    free(matches[i]);
                }
            }
        }
    }

    xf86Msg(X_DEFAULT, "Assigned the driver to the xf86ConfigLayout\n");

    return ptr;
}

#if ((defined(__sparc__) || defined(__sparc)) && defined (sun))
#include <libdevinfo.h>
#include <xorg/xf86Pci.h>
#include "pciaccess.h"
#include <sys/utsname.h>

#define MAX_DEV                        16

#define IS_VGA(c) \
       (((c) & 0x00ffff00) \
       == ((PCI_CLASS_DISPLAY << 16) | (PCI_SUBCLASS_DISPLAY_VGA << 8)))


#define BUILTIN_MULTI_SERVERFLAGS_OPT1 \
       "\tOption\t\"DefaultServerLayout\"\t\"Xsession0\"\n"

#define BUILTIN_MULTI_SECTION_POST \
       "EndSection\n\n"

#define BUILTIN_MULTI_SERVERFLAGS_OPT2 \
       "\tOption\t\"AutoAddDevices\"\t\"true\"\n"

#define BUILTIN_MULTI_SERVERFLAGS_SECTION \
       "Section \"ServerFlags\"\n" \
       BUILTIN_MULTI_SERVERFLAGS_OPT1 \
       BUILTIN_MULTI_SERVERFLAGS_OPT2 \
       BUILTIN_MULTI_SECTION_POST

#define BUILTIN_MULTI_LAYOUT_SECTION_PRE \
       "Section \"ServerLayout\"\n" \
       "\tIdentifier\t\"Xsession%d\"\n"

#define BUILTIN_MULTI_LAYOUT_SECTION_LINE \
       "\tScreen\t\t\"Screen%d\"\n"

#define BUILTIN_MULTI_SCREEN_SECTION_PRE \
       "Section \"Screen\"\n" \
       "\tIdentifier\t\"Screen%d\"\n"

#define BUILTIN_MULTI_SCREEN_SECTION_LINE \
       "\tDevice\t\t\"Card%d\"\n"

#define BUILTIN_MULTI_DEVICE_SECTION_PRE \
       "Section \"Device\"\n" \
       "\tIdentifier\t\"Card%d\"\n"

#define BUILTIN_MULTI_DEVICE_SECTION_LINE \
       "\tBusID\t\t\"%s\"\n\tDriver\t\t\"%s\"\n"

static Bool
MultiSessionConfig (void)
{
    di_node_t node;
    struct pci_device_iterator * iter;
    struct pci_device * dev;
    typedef struct {
       char            busid[64];
       char            dev_path[PATH_MAX];
    } disp_dev_type;
    disp_dev_type disp_dev[MAX_DEV];
    int num = 0;
    char buf[1024];
    int i;
    struct utsname sys;
    struct sol_device_private {
        struct pci_device  base;
        const char * device_string;
    };
#define DEV_PATH(dev)    (((struct sol_device_private *) dev)->device_string)

    if (uname(&sys) < 0) {
       xf86Msg(X_ERROR, "Error in uname call\n");
       return FALSE;
    }

    if (strcmp(sys.machine, "sun4v"))
       return FALSE;

    iter = pci_slot_match_iterator_create( NULL );

    while ( (dev = pci_device_next( iter )) != NULL ) {
       if (IS_VGA(dev->device_class)) {
           if (num > MAX_DEV) {
               xf86Msg(X_ERROR, "Too many display devices: %d\n", num);
               return FALSE;
           }

           memset(&disp_dev[num], 0, sizeof (disp_dev_type));

           snprintf(disp_dev[num].busid, sizeof(disp_dev[num].busid),
               "PCI:%d@%d:%d:%d", dev->bus, dev->domain,
               dev->dev, dev->func);

           strcpy(disp_dev[num].dev_path, "/devices");
           strncat(disp_dev[num].dev_path, DEV_PATH(dev),
               sizeof(disp_dev_type) - strlen("/devices"));

           num++;
       }
    }

    pci_iterator_destroy(iter);

    /* Do not do multi-session configuration if dev number is 0 or 1 */
    if (num <= 1)
       return FALSE;

    snprintf(buf, sizeof(buf), BUILTIN_MULTI_SERVERFLAGS_SECTION);
    AppendToConfig(buf);

    for (i = 0; i < num; i++) {
       char drv[32];

       snprintf(buf, sizeof(buf), BUILTIN_MULTI_LAYOUT_SECTION_PRE, i);
       AppendToConfig(buf);
       snprintf(buf, sizeof(buf), BUILTIN_MULTI_LAYOUT_SECTION_LINE, i);
       AppendToConfig(buf);
       snprintf(buf, sizeof(buf), BUILTIN_MULTI_SECTION_POST);
       AppendToConfig(buf);

       snprintf(buf, sizeof(buf), BUILTIN_MULTI_SCREEN_SECTION_PRE, i);
       AppendToConfig(buf);
       snprintf(buf, sizeof(buf), BUILTIN_MULTI_SCREEN_SECTION_LINE, i);
       AppendToConfig(buf);
       snprintf(buf, sizeof(buf), BUILTIN_MULTI_SECTION_POST);
       AppendToConfig(buf);

       snprintf(buf, sizeof(buf), BUILTIN_MULTI_DEVICE_SECTION_PRE, i);
       AppendToConfig(buf);

       drv[0] = 0;
       if (disp_dev[i].dev_path) {
           int iret;
           int fd;


           fd = open(disp_dev[i].dev_path, O_RDONLY);
           if (fd >= 0) {
               struct vis_identifier visid;
               const char *cp;

               SYSCALL(iret = ioctl(fd, VIS_GETIDENTIFIER, &visid));
               close (fd);

               if (iret >= 0) {
                   if (strcmp(visid.name, "SUNWtext") != 0) {
                       for (cp = visid.name; (*cp != '\0') && isupper((uchar_t)*cp); cp++);
                       /* find end of all uppercase vendor section */
                       if ((cp != visid.name) && (*cp != '\0'))
                           strncpy (drv, cp, sizeof(drv));
                   }
               }
           }
        }

       if (drv[0] == 0) {
           xf86Msg(X_ERROR, "Can't find driver for session %d\n", i);
           FreeConfig();
           return FALSE;
       }

       snprintf(buf, sizeof(buf), BUILTIN_MULTI_DEVICE_SECTION_LINE,
               disp_dev[i].busid, drv);
       AppendToConfig(buf);
       snprintf(buf, sizeof(buf), BUILTIN_MULTI_SECTION_POST);
      AppendToConfig(buf);
    }

    return TRUE;
}
#endif

