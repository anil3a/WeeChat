/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * script-config.c: script configuration options (file script.conf)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-config.h"
#include "script-buffer.h"
#include "script-repo.h"


struct t_config_file *script_config_file = NULL;
struct t_config_section *script_config_section_look = NULL;
struct t_config_section *script_config_section_color = NULL;
struct t_config_section *script_config_section_scripts = NULL;

/* script config, look section */

struct t_config_option *script_config_look_columns;
struct t_config_option *script_config_look_sort;
struct t_config_option *script_config_look_translate_description;

/* script config, color section */

struct t_config_option *script_config_color_status_popular;
struct t_config_option *script_config_color_status_installed;
struct t_config_option *script_config_color_status_autoloaded;
struct t_config_option *script_config_color_status_held;
struct t_config_option *script_config_color_status_running;
struct t_config_option *script_config_color_status_obsolete;
struct t_config_option *script_config_color_status_unknown;
struct t_config_option *script_config_color_text;
struct t_config_option *script_config_color_text_date;
struct t_config_option *script_config_color_text_delimiters;
struct t_config_option *script_config_color_text_description;
struct t_config_option *script_config_color_text_extension;
struct t_config_option *script_config_color_text_name;
struct t_config_option *script_config_color_text_tags;
struct t_config_option *script_config_color_text_version;
struct t_config_option *script_config_color_text_version_loaded;
struct t_config_option *script_config_color_text_bg;
struct t_config_option *script_config_color_text_selected;
struct t_config_option *script_config_color_text_date_selected;
struct t_config_option *script_config_color_text_description_selected;
struct t_config_option *script_config_color_text_extension_selected;
struct t_config_option *script_config_color_text_name_selected;
struct t_config_option *script_config_color_text_tags_selected;
struct t_config_option *script_config_color_text_version_selected;
struct t_config_option *script_config_color_text_version_loaded_selected;
struct t_config_option *script_config_color_text_bg_selected;

/* script config, scripts section */

struct t_config_option *script_config_scripts_cache_expire;
struct t_config_option *script_config_scripts_dir;
struct t_config_option *script_config_scripts_hold;
struct t_config_option *script_config_scripts_url;


/*
 * script_config_get_dir: get local directory for script
 *                        Note: result must be freed after use
 */

char *
script_config_get_dir ()
{
    const char *weechat_home;
    char *path, *path2;

    weechat_home = weechat_info_get ("weechat_dir", NULL);

    path = weechat_string_expand_home (weechat_config_string (script_config_scripts_dir));
    path2 = weechat_string_replace ((path) ?
                                    path : weechat_config_string (script_config_scripts_dir),
                                    "%h", weechat_home);

    if (path && path2)
    {
        free (path);
        path = NULL;
    }

    if (path2)
        return path2;
    if (path)
        return path;
    return strdup (weechat_config_string (script_config_scripts_dir));
}

/*
 * script_config_get_xml_filename: get filename with scripts (by default
 *                                 "/home/xxx/.weechat/script/plugins.xml.gz")
 *                                 Note: result must be freed after use
 */

char *
script_config_get_xml_filename ()
{
    char *path, *filename;
    int length;

    path = script_config_get_dir ();
    length = strlen (path) + 64;
    filename = malloc (length);
    if (filename)
        snprintf (filename, length, "%s/plugins.xml.gz", path);
    free (path);
    return filename;
}

/*
 * script_config_get_script_download_filename: get filename for a script to
 *                                             download, for eample:
 *                                             "/home/xxx/.weechat/script/iset.pl"
 *                                             Note: result must be freed after
 *                                             use
 */

char *
script_config_get_script_download_filename (struct t_repo_script *script)
{
    char *path, *filename;
    int length;

    path = script_config_get_dir ();
    length = strlen (path) + 1 + strlen (script->name_with_extension) + 1;
    filename = malloc (length);
    if (filename)
        snprintf (filename, length, "%s/%s", path, script->name_with_extension);
    free (path);
    return filename;
}

/*
 * script_config_refresh_cb: callback called when script buffer needs to be
 *                           refreshed
 */

void
script_config_refresh_cb (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (script_buffer)
        script_buffer_refresh (0);
}

/*
 * script_config_reload_scripts_cb: callback called list of scripts must be
 *                                  reloaded from file (plugins.xml.gz)
 */

void
script_config_reload_scripts_cb (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (repo_scripts)
    {
        script_repo_remove_all ();
        script_repo_file_read (1);
        script_buffer_refresh (1);
    }
}

/*
 * script_config_change_hold_cb: callback called when list of scripts to "hold"
 *                               is changed
 */

void
script_config_change_hold_cb (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    script_repo_update_status_all ();
    if (script_buffer)
        script_buffer_refresh (0);
}

/*
 * script_config_hold: hold a script
 *                     Note: the option is changed, but the status "held"
 *                     in script is NOT updated by this function
 */

void
script_config_hold (const char *name_with_extension)
{
    char **items, *hold;
    int num_items, i, length;

    length = strlen (weechat_config_string (script_config_scripts_hold)) +
        1 + strlen (name_with_extension) + 1;
    hold = malloc (length);
    if (hold)
    {
        hold[0] = '\0';
        items = weechat_string_split (weechat_config_string (script_config_scripts_hold),
                                      ",", 0, 0, &num_items);
        if (items)
        {
            for (i = 0; i < num_items; i++)
            {
                if (strcmp (items[i], name_with_extension) != 0)
                {
                    if (hold[0])
                        strcat (hold, ",");
                    strcat (hold, items[i]);
                }
            }
            weechat_string_free_split (items);
        }
        if (hold[0])
            strcat (hold, ",");
        strcat (hold, name_with_extension);

        weechat_config_option_set (script_config_scripts_hold, hold, 0);

        free (hold);
    }
}

/*
 * script_config_unhold: unhold a script
 *                       Note: the option is changed, but the status "held"
 *                       in script is NOT updated by this function
 */

void
script_config_unhold (const char *name_with_extension)
{
    char **items, *hold;
    int num_items, i, length;

    length = strlen (weechat_config_string (script_config_scripts_hold)) + 1;
    hold = malloc (length);
    if (hold)
    {
        hold[0] = '\0';
        items = weechat_string_split (weechat_config_string (script_config_scripts_hold),
                                      ",", 0, 0, &num_items);
        if (items)
        {
            for (i = 0; i < num_items; i++)
            {
                if (strcmp (items[i], name_with_extension) != 0)
                {
                    if (hold[0])
                        strcat (hold, ",");
                    strcat (hold, items[i]);
                }
            }
            weechat_string_free_split (items);
        }

        weechat_config_option_set (script_config_scripts_hold, hold, 0);

        free (hold);
    }
}

/*
 * script_config_reaload: reload script configuration file
 */

int
script_config_reload (void *data, struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) data;

    return weechat_config_reload (config_file);
}

/*
 * script_config_init: init script configuration file
 *                     return: 1 if ok, 0 if error
 */

int
script_config_init ()
{
    struct t_config_section *ptr_section;

    script_config_file = weechat_config_new (SCRIPT_CONFIG_NAME,
                                             &script_config_reload, NULL);
    if (!script_config_file)
        return 0;

    /* look */
    ptr_section = weechat_config_new_section (script_config_file, "look",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (script_config_file);
        return 0;
    }

    script_config_look_columns = weechat_config_new_option (
        script_config_file, ptr_section,
        "columns", "string",
        N_("format of columns displayed in script buffer: following column "
           "identifiers are replaced by their value: %a=author, %d=description, "
           "%D=date added, %e=extension, %l=language, %L=license, %n=name with "
           "extension, %N=name, %r=requirements, %s=status, %t=tags, "
           "%u=date updated, %v=version, %V=version loaded, %w=min_weechat, "
           "%W=max_weechat)"),
        NULL, 0, 0, "%s %n %V %v %u | %d | %t", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_look_sort = weechat_config_new_option (
        script_config_file, ptr_section,
        "sort", "string",
        N_("default sort keys for scripts: comma-separated list of identifiers: "
           "a=author, A=autoloaded, d=date added, e=extension, i=installed, "
           "l=language, n=name, o=obsolete, p=popularity, r=running, "
           "u=date updated; char \"-\" can be used before identifier to reverse "
           "order; example: \"i,u\": installed scripts first, sorted by update "
           "date"),
        NULL, 0, 0, "p,n", NULL, 0,
        NULL, NULL, &script_config_reload_scripts_cb, NULL, NULL, NULL);
    script_config_look_translate_description = weechat_config_new_option (
        script_config_file, ptr_section,
        "translate_description", "boolean",
        N_("translate description of scripts (if translation is available in "
           "your language, otherwise english version is used)"),
        NULL, 0, 0, "on", NULL, 0,
        NULL, NULL, &script_config_reload_scripts_cb, NULL, NULL, NULL);

    /* color */
    ptr_section = weechat_config_new_section (script_config_file, "color",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (script_config_file);
        return 0;
    }

    script_config_color_status_popular = weechat_config_new_option (
        script_config_file, ptr_section,
        "status_popular", "color",
        N_("color for status \"popular\" (\"*\")"),
        NULL, 0, 0, "yellow", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_status_installed = weechat_config_new_option (
        script_config_file, ptr_section,
        "status_installed", "color",
        N_("color for status \"installed\" (\"i\")"),
        NULL, 0, 0, "lightcyan", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_status_autoloaded = weechat_config_new_option (
        script_config_file, ptr_section,
        "status_autoloaded", "color",
        N_("color for status \"autoloaded\" (\"a\")"),
        NULL, 0, 0, "cyan", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_status_held = weechat_config_new_option (
        script_config_file, ptr_section,
        "status_held", "color",
        N_("color for status \"held\" (\"H\")"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_status_running = weechat_config_new_option (
        script_config_file, ptr_section,
        "status_running", "color",
        N_("color for status \"running\" (\"r\")"),
        NULL, 0, 0, "lightgreen", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_status_obsolete = weechat_config_new_option (
        script_config_file, ptr_section,
        "status_obsolete", "color",
        N_("color for status \"obsolete\" (\"N\")"),
        NULL, 0, 0, "lightmagenta", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_status_unknown = weechat_config_new_option (
        script_config_file, ptr_section,
        "status_unknown", "color",
        N_("color for status \"unknown\" (\"?\")"),
        NULL, 0, 0, "lightred", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text = weechat_config_new_option (
        script_config_file, ptr_section,
        "text", "color",
        N_("text color in script buffer"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_date = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_date", "color",
        N_("text color of dates in script buffer"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_delimiters = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_delimiters", "color",
        N_("text color of delimiters in script buffer"),
        NULL, 0, 0, "darkgray", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_description = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_description", "color",
        N_("text color of description in script buffer"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_extension = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_extension", "color",
        N_("text color of extension in script buffer"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_name = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_name", "color",
        N_("text color of script name in script buffer"),
        NULL, 0, 0, "cyan", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_tags = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_tags", "color",
        N_("text color of tags in script buffer"),
        NULL, 0, 0, "brown", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_version = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_version", "color",
        N_("text color of version in script buffer"),
        NULL, 0, 0, "magenta", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_version_loaded = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_version_loaded", "color",
        N_("text color of version loaded in script buffer"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_bg = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_bg", "color",
        N_("background color in script buffer"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_selected = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_selected", "color",
        N_("text color for selected line in script buffer"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_date_selected = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_date_selected", "color",
        N_("text color of dates for selected line in script buffer"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_description_selected = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_description_selected", "color",
        N_("text color of description for selected line in script buffer"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_extension_selected = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_extension_selected", "color",
        N_("text color of extension for selected line in script buffer"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_name_selected = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_name_selected", "color",
        N_("text color of script name for selected line in script buffer"),
        NULL, 0, 0, "lightcyan", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_tags_selected = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_tags_selected", "color",
        N_("text color of tags for selected line in script buffer"),
        NULL, 0, 0, "yellow", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_version_selected = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_version_selected", "color",
        N_("text color of version for selected line in script buffer"),
        NULL, 0, 0, "lightmagenta", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_version_loaded_selected = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_version_loaded_selected", "color",
        N_("text color of version loaded for selected line in script buffer"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);
    script_config_color_text_bg_selected = weechat_config_new_option (
        script_config_file, ptr_section,
        "text_bg_selected", "color",
        N_("background color for selected line in script buffer"),
        NULL, 0, 0, "red", NULL, 0,
        NULL, NULL, &script_config_refresh_cb, NULL, NULL, NULL);

    /* scripts */
    ptr_section = weechat_config_new_section (script_config_file, "scripts",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (script_config_file);
        return 0;
    }

    script_config_scripts_cache_expire = weechat_config_new_option (
        script_config_file, ptr_section,
        "cache_expire", "integer",
        N_("local cache expiration time, in minutes (-1 = never expires, "
           "0 = always expire)"),
        NULL, -1, 525600, "60", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    script_config_scripts_dir = weechat_config_new_option (
        script_config_file, ptr_section,
        "dir", "string",
        N_("local cache directory for scripts"),
        NULL, 0, 0, "%h/script", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    script_config_scripts_hold = weechat_config_new_option (
        script_config_file, ptr_section,
        "hold", "string",
        N_("scripts to \"hold\": comma-separated list of scripts which will "
           "never been upgraded and can not be removed, for example: "
           "\"buffers.pl,iset.pl\""),
        NULL, 0, 0, "", NULL, 0, NULL, NULL,
        &script_config_change_hold_cb, NULL, NULL, NULL);
    script_config_scripts_url = weechat_config_new_option (
        script_config_file, ptr_section,
        "url", "string",
        N_("URL for file with list of scripts"),
        NULL, 0, 0, "http://www.weechat.org/files/plugins.xml.gz", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);

    return 1;
}

/*
 * script_config_read: read script configuration file
 */

int
script_config_read ()
{
    return weechat_config_read (script_config_file);
}

/*
 * script_config_write: write script configuration file
 */

int
script_config_write ()
{
    return weechat_config_write (script_config_file);
}

/*
 * script_config_free: free script configuration file
 */

void
script_config_free ()
{
    weechat_config_free (script_config_file);
}
