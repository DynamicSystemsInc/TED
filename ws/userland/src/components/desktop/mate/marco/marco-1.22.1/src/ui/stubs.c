/* Marco stubs for libmarco-private */

/*
 * Copyright (C) 2020 Dynamic Systems, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <gtk/gtk.h>
/*
 * The following three dummy functions are needed
 * to resolve undefined functions in libmarco-private
 * that were introduced in marco's ui/theme.c
 */
int meta_workspace_index(void *workspace)
{
	return (0);
}
gboolean tsol_is_available()
{
	return FALSE;
}
char *meta_prefs_get_workspace_label(int i)
{
	return ("");
}
