/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* wee-upgrade-file.c: save/restore data for upgrading WeeChat */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "weechat.h"
#include "wee-upgrade-file.h"
#include "wee-infolist.h"
#include "wee-string.h"
#include "wee-utf8.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-main.h"
#include "../plugins/plugin.h"


struct t_upgrade_file *upgrade_files = NULL;
struct t_upgrade_file *last_upgrade_file = NULL;


/*
 * upgrade_file_error: display error
 */

void
upgrade_file_error (struct t_upgrade_file *upgrade_file, char *message,
                    char *file, int line)
{
    gui_chat_printf (NULL,
                     _("%sError upgrading WeeChat:"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     message, file, line);
    gui_chat_printf (NULL,
                     _("%s    error: %s"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     message);
    if ((upgrade_file->last_read_pos > 0)
        || (upgrade_file->last_read_length > 0))
    {
        gui_chat_printf (NULL,
                         _("%s    last read: position: %ld, length: %d"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         upgrade_file->last_read_pos,
                         upgrade_file->last_read_length);
    }
    gui_chat_printf (NULL,
                     _("%s    source: %s, line: %d"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     file, line);
    gui_chat_printf (NULL,
                     _("%s*** Please report above info to developers ***"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
}

/*
 * upgrade_file_write_integer: write int value in upgrade file
 *                             return 1 if ok, 0 if error
 */

int
upgrade_file_write_integer (struct t_upgrade_file *upgrade_file, int value)
{
    if (fwrite ((void *)(&value), sizeof (value), 1, upgrade_file->file) <= 0)
        return 0;
    
    return 1;
}

/*
 * upgrade_file_write_time: write int value in upgrade file
 *                          return 1 if ok, 0 if error
 */

int
upgrade_file_write_time (struct t_upgrade_file *upgrade_file, time_t date)
{
    if (fwrite ((void *)(&date), sizeof (date), 1, upgrade_file->file) <= 0)
        return 0;
    
    return 1;
}

/*
 * upgrade_file_write_string: write string in upgrade file
 *                            return 1 if ok, 0 if error
 */

int
upgrade_file_write_string (struct t_upgrade_file *upgrade_file, char *string)
{
    int length;
    
    if (string && string[0])
    {
        length = strlen (string);
        if (!upgrade_file_write_integer (upgrade_file, length))
            return 0;
        if (fwrite ((void *)string, length, 1, upgrade_file->file) <= 0)
            return 0;
    }
    else
    {
        if (!upgrade_file_write_integer (upgrade_file, 0))
            return 0;
    }
    
    return 1;
}

/*
 * upgrade_file_write_buffer: write buffer in upgrade file
 *                            return 1 if ok, 0 if error
 */

int
upgrade_file_write_buffer (struct t_upgrade_file *upgrade_file, void *pointer,
                           int size)
{
    if (pointer)
    {
        if (!upgrade_file_write_integer (upgrade_file, size))
            return 0;
        if (fwrite (pointer, size, 1, upgrade_file->file) <= 0)
            return 0;
    }
    else
    {
        if (!upgrade_file_write_integer (upgrade_file, 0))
            return 0;
    }
    
    return 1;
}

/*
 * upgrade_file_create: create an upgrade file
 *                      if write == 1 then it's open in write mode, otherwise
 *                      read mode
 */

struct t_upgrade_file *
upgrade_file_create (const char *filename, int write)
{
    int length;
    struct t_upgrade_file *new_upgrade_file;
    
    if (!filename)
        return NULL;
    
    new_upgrade_file = malloc (sizeof (*new_upgrade_file));
    if (new_upgrade_file)
    {
        /* build name of file */
        length = strlen (weechat_home) + 1 + strlen (filename) + 16 + 1;
        new_upgrade_file->filename = malloc (length + 1);
        if (!new_upgrade_file->filename)
        {
            free (new_upgrade_file);
            return NULL;
        }
        snprintf (new_upgrade_file->filename, length, "%s/%s.upgrade",
                  weechat_home, filename);
        
        /* open file in read or write mode */
        if (write)
            new_upgrade_file->file = fopen (new_upgrade_file->filename, "wb");
        else
            new_upgrade_file->file = fopen (new_upgrade_file->filename, "rb");
        
        if (!new_upgrade_file->file)
        {
            free (new_upgrade_file->filename);
            free (new_upgrade_file);
            return NULL;
        }
        
        /* change permissions if write mode */
        if (write)
        {
            chmod (new_upgrade_file->filename, 0600);
            
            /* write signature */
            upgrade_file_write_string (new_upgrade_file, UPGRADE_SIGNATURE);
        }
        
        /* init positions */
        new_upgrade_file->last_read_pos = 0;
        new_upgrade_file->last_read_length = 0;
        
        /* add upgrade file to list of upgrade files */
        new_upgrade_file->prev_upgrade = last_upgrade_file;
        new_upgrade_file->next_upgrade = NULL;
        if (upgrade_files)
            last_upgrade_file->next_upgrade = new_upgrade_file;
        else
            upgrade_files = new_upgrade_file;
        last_upgrade_file = new_upgrade_file;
    }
    return new_upgrade_file;
}

/*
 * upgrade_file_write_object: write object to file
 *                            return 1 if ok, 0 if error
 */

int
upgrade_file_write_object (struct t_upgrade_file *upgrade_file, int object_id,
                           struct t_infolist *infolist)
{
    int i, argc, length;
    char *fields, **argv;
    void *buf;
    
    /* write all infolist variables */
    infolist_reset_item_cursor (infolist);
    while (infolist_next (infolist))
    {
        /* write object start with id */
        if (!upgrade_file_write_integer (upgrade_file, UPGRADE_TYPE_OBJECT_START))
        {
            UPGRADE_ERROR("write - object type - 'object start'");
            return 0;
        }
        if (!upgrade_file_write_integer (upgrade_file, object_id))
        {
            UPGRADE_ERROR("write - object id");
            return 0;
        }
        
        fields = infolist_fields (infolist);
        if (fields)
        {
            argv = string_explode (fields, ",", 0, 0, &argc);
            if (argv && (argc > 0))
            {
                for (i = 0; i < argc; i++)
                {
                    switch (argv[i][0])
                    {
                        case 'i': /* integer */
                            if (!upgrade_file_write_integer (upgrade_file, UPGRADE_TYPE_OBJECT_VAR))
                            {
                                UPGRADE_ERROR("write - object type - 'object var'");
                                return 0;
                            }
                            if (!upgrade_file_write_string (upgrade_file, argv[i] + 2))
                            {
                                UPGRADE_ERROR("write - variable name");
                                return 0;
                            }
                            if (!upgrade_file_write_integer (upgrade_file, INFOLIST_INTEGER))
                            {
                                UPGRADE_ERROR("write - infolist type - 'integer'");
                                return 0;
                            }
                            if (!upgrade_file_write_integer (upgrade_file,
                                                             infolist_integer (infolist, argv[i] + 2)))
                            {
                                UPGRADE_ERROR("write - integer variable");
                                return 0;
                            }
                            break;
                        case 's': /* string */
                            if (!upgrade_file_write_integer (upgrade_file, UPGRADE_TYPE_OBJECT_VAR))
                            {
                                UPGRADE_ERROR("write - object type - 'object var'");
                                return 0;
                            }
                            if (!upgrade_file_write_string (upgrade_file, argv[i] + 2))
                            {
                                UPGRADE_ERROR("write - variable name");
                                return 0;
                            }
                            if (!upgrade_file_write_integer (upgrade_file, INFOLIST_STRING))
                            {
                                UPGRADE_ERROR("write - infolist type - 'string'");
                                return 0;
                            }
                            if (!upgrade_file_write_string (upgrade_file,
                                                            infolist_string (infolist, argv[i] + 2)))
                            {
                                UPGRADE_ERROR("write - string variable");
                                return 0;
                            }
                            break;
                        case 'p': /* pointer */
                            /* pointer in not used in upgrade files, only buffer is */
                            break;
                        case 'b': /* buffer */
                            buf = infolist_buffer (infolist, argv[i] + 2, &length);
                            if (buf && (length > 0))
                            {
                                if (!upgrade_file_write_integer (upgrade_file, UPGRADE_TYPE_OBJECT_VAR))
                                {
                                    UPGRADE_ERROR("write - object type - 'object var'");
                                    return 0;
                                }
                                if (!upgrade_file_write_string (upgrade_file, argv[i] + 2))
                                {
                                    UPGRADE_ERROR("write - variable name");
                                    return 0;
                                }
                                if (!upgrade_file_write_integer (upgrade_file, INFOLIST_BUFFER))
                                {
                                    UPGRADE_ERROR("write - infolist type - 'buffer'");
                                    return 0;
                                }
                                if (!upgrade_file_write_buffer (upgrade_file, buf, length))
                                {
                                    UPGRADE_ERROR("write - buffer variable");
                                    return 0;
                                }
                            }
                            break;
                        case 't': /* time */
                            if (!upgrade_file_write_integer (upgrade_file, UPGRADE_TYPE_OBJECT_VAR))
                            {
                                UPGRADE_ERROR("write - object type - 'object var'");
                                return 0;
                            }
                            if (!upgrade_file_write_string (upgrade_file, argv[i] + 2))
                            {
                                UPGRADE_ERROR("write - variable name");
                                return 0;
                            }
                            if (!upgrade_file_write_integer (upgrade_file, INFOLIST_TIME))
                            {
                                UPGRADE_ERROR("write - infolist type - 'time'");
                                return 0;
                            }
                            if (!upgrade_file_write_time (upgrade_file,
                                                          infolist_time (infolist, argv[i] + 2)))
                            {
                                UPGRADE_ERROR("write - time variable");
                                return 0;
                            }
                            break;
                    }
                }
            }
            if (argv)
                string_free_exploded (argv);
        }
        
        /* write object end */
        if (!upgrade_file_write_integer (upgrade_file, UPGRADE_TYPE_OBJECT_END))
            return 0;
    }
    
    return 1;
}

/*
 * upgrade_file_read_integer: read integer from upgrade file
 */

int
upgrade_file_read_integer (struct t_upgrade_file *upgrade_file, int *value)
{
    upgrade_file->last_read_pos = ftell (upgrade_file->file);
    upgrade_file->last_read_length = sizeof (*value);

    if (value)
    {
        if (fread ((void *)value, sizeof (*value), 1, upgrade_file->file) <= 0)
            return 0;
    }
    else
    {
        if (fseek (upgrade_file->file, sizeof (*value), SEEK_CUR) < 0)
            return 0;
    }
    return 1;
}

/*
 * upgrade_file_read_string: read string from upgrade file
 */

int
upgrade_file_read_string (struct t_upgrade_file *upgrade_file, char **string)
{
    int length;
    
    if (string && *string)
    {
        free (*string);
        *string = NULL;
    }
    
    if (!upgrade_file_read_integer (upgrade_file, &length))
        return 0;
    
    upgrade_file->last_read_pos = ftell (upgrade_file->file);
    upgrade_file->last_read_length = length;
    
    if (string)
    {
        if (length == 0)
            return 1;
        
        (*string) = malloc (length + 1);
        if (!(*string))
            return 0;
        
        if (fread ((void *)(*string), length, 1, upgrade_file->file) <= 0)
        {
            free (*string);
            *string = NULL;
            return 0;
        }
        (*string)[length] = '\0';
    }
    else
    {
        if (fseek (upgrade_file->file, length, SEEK_CUR) < 0)
            return 0;
    }
    return 1;
}

/*
 * upgrade_file_read_string_utf8: read string from file, then normalize UTF-8
 */

int
upgrade_file_read_string_utf8 (struct t_upgrade_file *upgrade_file,
                               char **string)
{
    if (!upgrade_file_read_string (upgrade_file, string))
        return 0;
    
    if (string && *string)
        utf8_normalize (*string, '?');
    
    return 1;
}

/*
 * upgrade_file_read_buffer: read buffer from upgrade file
 */

int
upgrade_file_read_buffer (struct t_upgrade_file *upgrade_file,
                          void **buffer, int *size)
{
    if (buffer && *buffer)
    {
        free (*buffer);
        *buffer = NULL;
    }
    
    if (!upgrade_file_read_integer (upgrade_file, size))
        return 0;
    
    if (*size > 0)
    {
        upgrade_file->last_read_pos = ftell (upgrade_file->file);
        upgrade_file->last_read_length = *size;
        
        *buffer = malloc (*size);
        
        if (buffer && *buffer)
        {
            if (fread (*buffer, *size, 1, upgrade_file->file) <= 0)
                return 0;
        }
        else
        {
            if (fseek (upgrade_file->file, *size, SEEK_CUR) < 0)
                return 0;
        }
    }
    
    return 1;
}

/*
 * upgrade_file_read_time: read time from upgrade file
 */

int
upgrade_file_read_time (struct t_upgrade_file *upgrade_file, time_t *time)
{
    upgrade_file->last_read_pos = ftell (upgrade_file->file);
    upgrade_file->last_read_length = sizeof (*time);
    
    if (time)
    {
        if (fread ((void *)time, sizeof (*time), 1, upgrade_file->file) <= 0)
            return 0;
    }
    else
    {
        if (fseek (upgrade_file->file, sizeof (*time), SEEK_CUR) < 0)
            return 0;
    }
    
    return 1;
}

/*
 * upgrade_file_read_object: read object from file, then call read callback
 *                           return 1 if ok, 0 if error
 */

int
upgrade_file_read_object (struct t_upgrade_file *upgrade_file)
{
    struct t_infolist *infolist;
    struct t_infolist_item *item;
    int rc, object_id, type, type_var, value, size;
    char *name, *value_str;
    void *buffer;
    time_t time;
    
    rc = 0;
    
    infolist = NULL;
    name = NULL;
    value_str = NULL;
    buffer = NULL;
    
    if (!upgrade_file_read_integer (upgrade_file, &type))
    {
        if (feof (upgrade_file->file))
            rc = 1;
        else
            UPGRADE_ERROR("read - object type");
        goto end;
    }
    
    if (type != UPGRADE_TYPE_OBJECT_START)
    {
        UPGRADE_ERROR("read - bad object type (not 'object start')");
        goto end;
    }
    
    if (!upgrade_file_read_integer (upgrade_file, &object_id))
    {
        UPGRADE_ERROR("read - object id");
        goto end;
    }
    
    infolist = infolist_new ();
    if (!infolist)
    {
        UPGRADE_ERROR("read - infolist creation");
        goto end;
    }
    item = infolist_new_item (infolist);
    if (!item)
    {
        UPGRADE_ERROR("read - infolist item creation");
        goto end;
    }
    
    while (1)
    {
        if (!upgrade_file_read_integer (upgrade_file, &type))
        {
            UPGRADE_ERROR("read - object type");
            goto end;
        }
        
        if (type == UPGRADE_TYPE_OBJECT_END)
            break;
        
        if (type == UPGRADE_TYPE_OBJECT_VAR)
        {
            if (!upgrade_file_read_string (upgrade_file, &name))
            {
                UPGRADE_ERROR("read - variable name");
                goto end;
            }
            if (!name)
            {
                UPGRADE_ERROR("read - variable name");
                goto end;
            }
            if (!upgrade_file_read_integer (upgrade_file, &type_var))
            {
                UPGRADE_ERROR("read - variable type");
                goto end;
            }
            
            switch (type_var)
            {
                case INFOLIST_INTEGER:
                    if (!upgrade_file_read_integer (upgrade_file, &value))
                    {
                        UPGRADE_ERROR("read - integer value");
                        goto end;
                    }
                    infolist_new_var_integer (item, name, value);
                    break;
                case INFOLIST_STRING:
                    if (!upgrade_file_read_string (upgrade_file, &value_str))
                    {
                        UPGRADE_ERROR("read - string value");
                        goto end;
                    }
                    infolist_new_var_string (item, name, value_str);
                    break;
                case INFOLIST_POINTER:
                    break;
                case INFOLIST_BUFFER:
                    if (!upgrade_file_read_buffer (upgrade_file, &buffer, &size))
                    {
                        UPGRADE_ERROR("read - buffer value");
                        goto end;
                    }
                    infolist_new_var_buffer (item, name, buffer, size);
                    break;
                case INFOLIST_TIME:
                    if (!upgrade_file_read_time (upgrade_file, &time))
                    {
                        UPGRADE_ERROR("read - time value");
                        goto end;
                    }
                    infolist_new_var_time (item, name, time);
                    break;
            }
        }
    }
    
    rc = 1;
    
    if (upgrade_file->callback_read)
    {
        if ((int)(upgrade_file->callback_read) (object_id, infolist) == WEECHAT_RC_ERROR)
            rc = 0;
    }
    
    end:
    if (infolist)
        infolist_free (infolist);
    if (name)
        free (name);
    if (value_str)
        free (value_str);
    if (buffer)
        free (buffer);
    
    return rc;
}

/*
 * upgrade_file_read: read an upgrade file
 */

int
upgrade_file_read (struct t_upgrade_file *upgrade_file,
                   int (*callback_read)(int object_id,
                                        struct t_infolist *infolist))
{
    char *signature;
    
    upgrade_file->callback_read = callback_read;
    
    signature = NULL;
    if (!upgrade_file_read_string (upgrade_file, &signature))
        return 0;
    
    if (!signature || (strcmp (signature, UPGRADE_SIGNATURE) != 0))
        return 0;
    
    free (signature);
    
    while (!feof (upgrade_file->file))
    {
        if (!upgrade_file_read_object (upgrade_file))
            return 0;
    }
    
    return 1;
}

/*
 * upgrade_file_close: close an upgrade file
 */

void
upgrade_file_close (struct t_upgrade_file *upgrade_file)
{
    fclose (upgrade_file->file);
}