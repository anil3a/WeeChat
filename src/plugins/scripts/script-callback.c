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

/* script-callback.c: script callbacks */


#include <stdlib.h>
#include <unistd.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-callback.h"


/*
 * script_callback_alloc: allocate a new callback and initializes it
 */

struct t_script_callback *
script_callback_alloc ()
{
    struct t_script_callback *new_script_callback;
    
    new_script_callback = (struct t_script_callback *)malloc (sizeof (struct t_script_callback));
    if (new_script_callback)
    {
        new_script_callback->script = NULL;
        new_script_callback->function = NULL;
        new_script_callback->config_file = NULL;
        new_script_callback->config_section = NULL;
        new_script_callback->config_option = NULL;
        new_script_callback->hook = NULL;
        new_script_callback->buffer = NULL;
        return new_script_callback;
    }
    
    return NULL;
}

/*
 * script_callback_add: add a callback to list
 */

void
script_callback_add (struct t_plugin_script *script,
                     struct t_script_callback *callback)
{
    if (script->callbacks)
        script->callbacks->prev_callback = callback;
    callback->prev_callback = NULL;
    callback->next_callback = script->callbacks;
    script->callbacks = callback;
}

/*
 * script_callback_remove: remove a callback from a script
 */

void
script_callback_remove (struct t_plugin_script *script,
                        struct t_script_callback *script_callback)
{
    /* remove callback from list */
    if (script_callback->prev_callback)
        script_callback->prev_callback->next_callback =
            script_callback->next_callback;
    if (script_callback->next_callback)
        script_callback->next_callback->prev_callback =
            script_callback->prev_callback;
    if (script->callbacks == script_callback)
        script->callbacks = script_callback->next_callback;
    
    /* unhook and free data */
    if (script_callback->function)
        free (script_callback->function);
    
    free (script_callback);
}

/*
 * script_callback_remove_all: remove all callbacks from a script
 */

void
script_callback_remove_all (struct t_plugin_script *script)
{
    while (script->callbacks)
    {
        script_callback_remove (script, script->callbacks);
    }
}

/*
 * script_callback_print_log: print callback infos in log (usually for crash dump)
 */

void
script_callback_print_log (struct t_weechat_plugin *weechat_plugin,
                           struct t_script_callback *script_callback)
{
    weechat_log_printf ("");
    weechat_log_printf ("[callback (addr:0x%x)]",       script_callback);
    weechat_log_printf ("  script. . . . . . . : 0x%x", script_callback->script);
    weechat_log_printf ("  function. . . . . . : '%s'", script_callback->function);
    weechat_log_printf ("  hook. . . . . . . . : 0x%x", script_callback->hook);
    weechat_log_printf ("  buffer. . . . . . . : 0x%x", script_callback->buffer);
    weechat_log_printf ("  prev_callback . . . : 0x%x", script_callback->prev_callback);
    weechat_log_printf ("  next_callback . . . : 0x%x", script_callback->next_callback);
}