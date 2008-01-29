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

/* weechat-perl-api.c: Perl API functions */

#undef _

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "../script-api.h"
#include "../script-callback.h"
#include "weechat-perl.h"


#define PERL_RETURN_OK XSRETURN_YES
#define PERL_RETURN_ERROR XSRETURN_NO
#define PERL_RETURN_EMPTY XSRETURN_EMPTY
#define PERL_RETURN_STRING(__string)              \
    if (__string)                                 \
    {                                             \
        XST_mPV (0, __string);                    \
        XSRETURN (1);                             \
    }                                             \
    XST_mPV (0, "");                              \
    XSRETURN (1)
#define PERL_RETURN_STRING_FREE(__string)         \
    if (__string)                                 \
    {                                             \
        XST_mPV (0, __string);                    \
        free (__string);                          \
        XSRETURN (1);                             \
    }                                             \
    XST_mPV (0, "");                              \
    XSRETURN (1)
#define PERL_RETURN_INT(__int)                  \
    XST_mIV (0, __int);                         \
    XSRETURN (1);


extern void boot_DynaLoader (pTHX_ CV* cv);


/*
 * weechat::register: startup function for all WeeChat Perl scripts
 */

static XS (XS_weechat_register)
{
    char *name, *author, *version, *license, *description, *shutdown_func;
    char *charset;
    dXSARGS;
    
    /* make C compiler happy */
    (void) items;
    (void) cv;
    
    perl_current_script = NULL;
    
    if (items < 7)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("register");
        PERL_RETURN_ERROR;
    }
    
    name = SvPV (ST (0), PL_na);
    author = SvPV (ST (1), PL_na);
    version = SvPV (ST (2), PL_na);
    license = SvPV (ST (3), PL_na);
    description = SvPV (ST (4), PL_na);
    shutdown_func = SvPV (ST (5), PL_na);
    charset = SvPV (ST (6), PL_na);
    
    if (script_search (weechat_perl_plugin, perl_scripts, name))
    {
        /* error: another script already exists with this name! */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), "perl", name);
        PERL_RETURN_ERROR;
    }
    
    /* register script */
    perl_current_script = script_add (weechat_perl_plugin,
                                      &perl_scripts,
                                      (perl_current_script_filename) ?
                                      perl_current_script_filename : "",
                                      name, author, version, license,
                                      description, shutdown_func, charset);
    if (perl_current_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: registered script \"%s\", "
                                         "version %s (%s)"),
                        weechat_prefix ("info"), "perl",
                        name, version, description);
    }
    else
    {
        PERL_RETURN_ERROR;
    }
    
    PERL_RETURN_OK;
}

/*
 * weechat::charser_set: set script charset
 */

static XS (XS_weechat_charset_set)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("charset_set");
        PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("charset_set");
        PERL_RETURN_ERROR;
    }
    
    script_api_charset_set (perl_current_script,
                            SvPV (ST (0), PL_na)); /* charset */
    
    PERL_RETURN_OK;
}

/*
 * weechat::iconv_to_internal: convert string to internal WeeChat charset
 */

static XS (XS_weechat_iconv_to_internal)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("iconv_to_internal");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("iconv_to_internal");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_iconv_to_internal (SvPV (ST (0), PL_na), /* charset */
                                        SvPV (ST (1), PL_na)); /* string */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::iconv_from_internal: convert string from WeeChat inernal charset
 *                               to another one
 */

static XS (XS_weechat_iconv_from_internal)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("iconv_from_internal");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("iconv_from_internal");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_iconv_from_internal (SvPV (ST (0), PL_na), /* charset */
                                          SvPV (ST (1), PL_na)); /* string */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::gettext: get translated string
 */

static XS (XS_weechat_gettext)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("gettext");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("gettext");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_gettext (SvPV (ST (0), PL_na)); /* string */
    PERL_RETURN_STRING(result);
}

/*
 * weechat::ngettext: get translated string with plural form
 */

static XS (XS_weechat_ngettext)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("ngettext");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("ngettext");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_ngettext (SvPV (ST (0), PL_na), /* single */
                               SvPV (ST (1), PL_na), /* plural */
                               SvIV (ST (2))); /* count */
    PERL_RETURN_STRING(result);
}

/*
 * weechat::mkdir_home: create a directory in WeeChat home
 */

static XS (XS_weechat_mkdir_home)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("mkdir_home");
        PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir_home");
        PERL_RETURN_ERROR;
    }
    
    if (weechat_mkdir_home (SvPV (ST (0), PL_na), /* directory */
                            SvIV (ST (1)))) /* mode */
        PERL_RETURN_OK;
    
    PERL_RETURN_ERROR;
}

/*
 * weechat::mkdir: create a directory
 */

static XS (XS_weechat_mkdir)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("mkdir");
        PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir");
        PERL_RETURN_ERROR;
    }
    
    if (weechat_mkdir (SvPV (ST (0), PL_na), /* directory */
                       SvIV (ST (1)))) /* mode */
        PERL_RETURN_OK;
    
    PERL_RETURN_ERROR;
}

/*
 * weechat::list_new: create a new list
 */

static XS (XS_weechat_list_new)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) items;
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_new");
	PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_new ());
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::list_add: add a string to list
 */

static XS (XS_weechat_list_add)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_add");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_add");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_add (script_str2ptr (SvPV (ST (0), PL_na)), /* weelist */
                                               SvPV (ST (1), PL_na), /* data */
                                               SvPV (ST (2), PL_na))); /* where */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::list_search: search a string in list
 */

static XS (XS_weechat_list_search)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_search");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_search");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_search (script_str2ptr (SvPV (ST (0), PL_na)), /* weelist */
                                                  SvPV (ST (1), PL_na))); /* data */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::list_casesearch: search a string in list (ignore case)
 */

static XS (XS_weechat_list_casesearch)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_casesearch");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_casesearch");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_casesearch (script_str2ptr (SvPV (ST (0), PL_na)), /* weelist */
                                                      SvPV (ST (1), PL_na))); /* data */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::list_get: get item by position
 */

static XS (XS_weechat_list_get)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_get");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_get");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_get (script_str2ptr (SvPV (ST (0), PL_na)), /* weelist */
                                               SvIV (ST (1)))); /* position */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::list_set: set new value for item
 */

static XS (XS_weechat_list_set)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_set");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_set");
        PERL_RETURN_ERROR;
    }
    
    weechat_list_set (script_str2ptr (SvPV (ST (0), PL_na)), /* item */
                      SvPV (ST (1), PL_na)); /* new_value */
    
    PERL_RETURN_OK;
}

/*
 * weechat::list_next: get next item
 */

static XS (XS_weechat_list_next)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_next");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_next");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_next (script_str2ptr (SvPV (ST (0), PL_na)))); /* item */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::list_prev: get previous item
 */

static XS (XS_weechat_list_prev)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_prev");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_prev");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_prev (script_str2ptr (SvPV (ST (0), PL_na)))); /* item */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::list_string: get string value of item
 */

static XS (XS_weechat_list_string)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_string");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_string");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_list_string (script_str2ptr (SvPV (ST (0), PL_na))); /* item */
    PERL_RETURN_STRING(result);
}

/*
 * weechat::list_size: get number of elements in list
 */

static XS (XS_weechat_list_size)
{
    int size;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_size");
	PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_size");
        PERL_RETURN_INT(0);
    }
    
    size = weechat_list_size (script_str2ptr (SvPV (ST (0), PL_na))); /* weelist */
    PERL_RETURN_INT(size);
}

/*
 * weechat::list_remove: remove item from list
 */

static XS (XS_weechat_list_remove)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_remove");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_remove");
        PERL_RETURN_ERROR;
    }
    
    weechat_list_remove (script_str2ptr (SvPV (ST (0), PL_na)), /* weelist */
                         script_str2ptr (SvPV (ST (1), PL_na))); /* item */
    
    PERL_RETURN_OK;
}

/*
 * weechat::list_remove_all: remove all items from list
 */

static XS (XS_weechat_list_remove_all)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_remove_all");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_remove_all");
        PERL_RETURN_ERROR;
    }
    
    weechat_list_remove_all (script_str2ptr (SvPV (ST (0), PL_na))); /* weelist */
    
    PERL_RETURN_OK;
}

/*
 * weechat::list_free: free list
 */

static XS (XS_weechat_list_free)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_free");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_free");
        PERL_RETURN_ERROR;
    }
    
    weechat_list_free (script_str2ptr (SvPV (ST (0), PL_na))); /* weelist */
    
    PERL_RETURN_OK;
}

/*
 * weechat_perl_api_config_reload_cb: callback for config reload
 */

int
weechat_perl_api_config_reload_cb (void *data,
                                   struct t_config_file *config_file)
{
    struct t_script_callback *script_callback;
    char *perl_argv[2];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = script_ptr2str (config_file);
        perl_argv[1] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (perl_argv[0])
            free (perl_argv[0]);

        return ret;
    }
    
    return 0;
}

/*
 * weechat::config_new: create a new configuration file
 */

static XS (XS_weechat_config_new)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_new");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_new");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_config_new (weechat_perl_plugin,
                                                    perl_current_script,
                                                    SvPV (ST (0), PL_na), /* filename */
                                                    &weechat_perl_api_config_reload_cb,
                                                    SvPV (ST (1), PL_na))); /* perl function */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_config_section_read_cb: callback for reading option in section
 */

void
weechat_perl_api_config_section_read_cb (void *data,
                                         struct t_config_file *config_file,
                                         char *option_name, char *value)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = script_ptr2str (config_file);
        perl_argv[1] = option_name;
        perl_argv[2] = value;
        perl_argv[3] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (rc)
            free (rc);
        if (perl_argv[0])
            free (perl_argv[0]);
    }
}

/*
 * weechat_perl_api_config_section_write_cb: callback for writing section
 */

void
weechat_perl_api_config_section_write_cb (void *data,
                                          struct t_config_file *config_file,
                                          char *section_name)
{
    struct t_script_callback *script_callback;
    char *perl_argv[3];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = script_ptr2str (config_file);
        perl_argv[1] = section_name;
        perl_argv[2] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);

        if (rc)
            free (rc);
        if (perl_argv[0])
            free (perl_argv[0]);
    }
}

/*
 * weechat_perl_api_config_section_write_default_cb: callback for writing
 *                                                   default values for section
 */

void
weechat_perl_api_config_section_write_default_cb (void *data,
                                                  struct t_config_file *config_file,
                                                  char *section_name)
{
    struct t_script_callback *script_callback;
    char *perl_argv[3];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = script_ptr2str (config_file);
        perl_argv[1] = section_name;
        perl_argv[2] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (rc)
            free (rc);
        if (perl_argv[0])
            free (perl_argv[0]);
    }
}

/*
 * weechat::config_new_section: create a new section in configuration file
 */

static XS (XS_weechat_config_new_section)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_new_section");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_new_section");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_config_new_section (weechat_perl_plugin,
                                                            perl_current_script,
                                                            script_str2ptr (SvPV (ST (0), PL_na)), /* config_file */
                                                            SvPV (ST (1), PL_na), /* name */
                                                            &weechat_perl_api_config_section_read_cb,
                                                            SvPV (ST (2), PL_na), /* perl function (read cb) */
                                                            &weechat_perl_api_config_section_write_cb,
                                                            SvPV (ST (3), PL_na), /* perl function (write cb) */
                                                            &weechat_perl_api_config_section_write_default_cb,
                                                            SvPV (ST (4), PL_na))); /* perl function (write default cb) */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::config_search_section: search section in configuration file
 */

static XS (XS_weechat_config_search_section)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_search_section");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_search_section");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_config_search_section (script_str2ptr (SvPV (ST (0), PL_na)), /* config_file */
                                                            SvPV (ST (1), PL_na))); /* section_name */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_config_option_change_cb: callback for option changed
 */

void
weechat_perl_api_config_option_change_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *perl_argv[1];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (rc)
            free (rc);
    }
}

/*
 * weechat::config_new_option: create a new option in section
 */

static XS (XS_weechat_config_new_option)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_new_option");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 10)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_new_option");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_config_new_option (weechat_perl_plugin,
                                                           perl_current_script,
                                                           script_str2ptr (SvPV (ST (0), PL_na)), /* config_file */
                                                           script_str2ptr (SvPV (ST (1), PL_na)), /* section */
                                                           SvPV (ST (2), PL_na), /* name */
                                                           SvPV (ST (3), PL_na), /* type */
                                                           SvPV (ST (4), PL_na), /* description */
                                                           SvPV (ST (5), PL_na), /* string_values */
                                                           SvIV (ST (6)), /* min */
                                                           SvIV (ST (7)), /* max */
                                                           SvPV (ST (8), PL_na), /* default_value */
                                                           &weechat_perl_api_config_option_change_cb,
                                                           SvPV (ST (9), PL_na))); /* perl function */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::config_search_option: search option in configuration file or section
 */

static XS (XS_weechat_config_search_option)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_search_option");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_search_option");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_config_search_option (script_str2ptr (SvPV (ST (0), PL_na)), /* config_file */
                                                           script_str2ptr (SvPV (ST (1), PL_na)), /* section */
                                                           SvPV (ST (2), PL_na))); /* option_name */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::config_string_to_boolean: return boolean value of a string
 */

static XS (XS_weechat_config_string_to_boolean)
{
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_string_to_boolean");
	PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_string_to_boolean");
        PERL_RETURN_INT(0);
    }
    
    value = weechat_config_string_to_boolean (SvPV (ST (0), PL_na)); /* text */
    PERL_RETURN_INT(value);
}

/*
 * weechat::config_option_set: set new value for option
 */

static XS (XS_weechat_config_option_set)
{
    int rc;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_set");
	PERL_RETURN_INT(0);
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_set");
        PERL_RETURN_INT(0);
    }
    
    rc = weechat_config_option_set (script_str2ptr (SvPV (ST (0), PL_na)), /* option */
                                    SvPV (ST (1), PL_na), /* new_value */
                                    SvIV (ST (2))); /* run_callback */
    PERL_RETURN_INT(rc);
}

/*
 * weechat::config_boolean: return boolean value of option
 */

static XS (XS_weechat_config_boolean)
{
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_boolean");
	PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_boolean");
        PERL_RETURN_INT(0);
    }
    
    value = weechat_config_boolean (script_str2ptr (SvPV (ST (0), PL_na))); /* option */
    PERL_RETURN_INT(value);
}

/*
 * weechat::config_integer: return integer value of option
 */

static XS (XS_weechat_config_integer)
{
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_integer");
	PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_integer");
        PERL_RETURN_INT(0);
    }
    
    value = weechat_config_integer (script_str2ptr (SvPV (ST (0), PL_na))); /* option */
    PERL_RETURN_INT(value);
}

/*
 * weechat::config_string: return string value of option
 */

static XS (XS_weechat_config_string)
{
    char *value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_string");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_string");
        PERL_RETURN_EMPTY;
    }
    
    value = weechat_config_string (script_str2ptr (SvPV (ST (0), PL_na))); /* option */
    PERL_RETURN_STRING(value);
}

/*
 * weechat::config_color: return color value of option
 */

static XS (XS_weechat_config_color)
{
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_color");
	PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_color");
        PERL_RETURN_INT(0);
    }
    
    value = weechat_config_color (script_str2ptr (SvPV (ST (0), PL_na))); /* option */
    PERL_RETURN_INT(value);
}

/*
 * weechat::config_write_line: write a line in configuration file
 */

static XS (XS_weechat_config_write_line)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_write_line");
	PERL_RETURN_ERROR;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_write_line");
        PERL_RETURN_ERROR;
    }
    
    weechat_config_write_line (script_str2ptr (SvPV (ST (0), PL_na)), /* config_file */
                               SvPV (ST (1), PL_na), /* option_name */
                               "%s",
                               SvPV (ST (2), PL_na)); /* value */
    
    PERL_RETURN_OK;
}

/*
 * weechat::config_write: write configuration file
 */

static XS (XS_weechat_config_write)
{
    int rc;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_write");
	PERL_RETURN_INT(-1);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_write");
        PERL_RETURN_INT(-1);
    }
    
    rc = weechat_config_write (script_str2ptr (SvPV (ST (0), PL_na))); /* config_file */
    PERL_RETURN_INT(rc);
}

/*
 * weechat::config_read: read configuration file
 */

static XS (XS_weechat_config_read)
{
    int rc;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_read");
	PERL_RETURN_INT(-1);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_read");
        PERL_RETURN_INT(-1);
    }
    
    rc = weechat_config_read (script_str2ptr (SvPV (ST (0), PL_na))); /* config_file */
    PERL_RETURN_INT(rc);
}

/*
 * weechat::config_reload: reload configuration file
 */

static XS (XS_weechat_config_reload)
{
    int rc;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_reload");
	PERL_RETURN_INT(-1);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_reload");
        PERL_RETURN_INT(-1);
    }
    
    rc = weechat_config_reload (script_str2ptr (SvPV (ST (0), PL_na))); /* config_file */
    PERL_RETURN_INT(rc);
}

/*
 * weechat::config_free: free configuration file
 */

static XS (XS_weechat_config_free)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_free");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_free");
        PERL_RETURN_ERROR;
    }
    
    script_api_config_free (weechat_perl_plugin,
                            perl_current_script,
                            script_str2ptr (SvPV (ST (0), PL_na))); /* config_file */
    
    PERL_RETURN_OK;
}

/*
 * weechat::prefix: get a prefix, used for display
 */

static XS (XS_weechat_prefix)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("prefix");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("prefix");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_prefix (SvPV (ST (0), PL_na));
    PERL_RETURN_STRING(result);
}

/*
 * weechat::color: get a color code, used for display
 */

static XS (XS_weechat_color)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("color");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("color");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_color (SvPV (ST (0), PL_na));
    PERL_RETURN_STRING(result);
}

/*
 * weechat::print: print message in a buffer
 */

static XS (XS_weechat_print)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("print");
        PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("print");
        PERL_RETURN_ERROR;
    }
    
    script_api_printf (weechat_perl_plugin,
                       perl_current_script,
                       script_str2ptr (SvPV (ST (0), PL_na)), /* buffer */
                       "%s", SvPV (ST (1), PL_na)); /* message */
    
    PERL_RETURN_OK;
}

/*
 * weechat::infobar_print: print message to infobar
 */

static XS (XS_weechat_infobar_print)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        PERL_RETURN_ERROR;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infobar_print");
        PERL_RETURN_ERROR;
    }
    
    script_api_infobar_printf (weechat_perl_plugin,
                               perl_current_script,
                               SvIV (ST (0)), /* delay */
                               SvPV (ST (1), PL_na), /* color */
                               "%s",
                               SvPV (ST (1), PL_na)); /* message */
    
    PERL_RETURN_OK;
}

/*
 * weechat::infobar_remove: remove message(s) from infobar
 */

static XS (XS_weechat_infobar_remove)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_remove");
        PERL_RETURN_ERROR;
    }
    
    weechat_infobar_remove ((items >= 1) ? SvIV (ST (0)) : 0);
    
    PERL_RETURN_OK;
}

/*
 * weechat::log_print: print message in WeeChat log file
 */

static XS (XS_weechat_log_print)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;

    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("log_print");
        PERL_RETURN_ERROR;
    }

    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("log_print");
        PERL_RETURN_ERROR;
    }
    
    script_api_log_printf (weechat_perl_plugin,
                           perl_current_script,
                           "%s", SvPV (ST (0), PL_na)); /* message */
    
    PERL_RETURN_OK;
}

/*
 * weechat_perl_api_hook_command_cb: callback for command hooked
 */

int
weechat_perl_api_hook_command_cb (void *data, struct t_gui_buffer *buffer,
                                  int argc, char **argv, char **argv_eol)
{
    struct t_script_callback *script_callback;
    char *perl_argv[3], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    /* make C compiler happy */
    (void) argv;
    
    script_callback = (struct t_script_callback *)data;
    
    perl_argv[0] = script_ptr2str (buffer);
    perl_argv[1] = (argc > 1) ? argv_eol[1] : empty_arg;
    perl_argv[2] = NULL;
    
    rc = (int *) weechat_perl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    perl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (perl_argv[0])
        free (perl_argv[0]);
    
    return ret;
}

/*
 * weechat::hook_command: hook a command
 */

static XS (XS_weechat_hook_command)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_command");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 6)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_command");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_command (weechat_perl_plugin,
                                                      perl_current_script,
                                                      SvPV (ST (0), PL_na), /* command */
                                                      SvPV (ST (1), PL_na), /* description */
                                                      SvPV (ST (2), PL_na), /* args */
                                                      SvPV (ST (3), PL_na), /* args_description */
                                                      SvPV (ST (4), PL_na), /* completion */
                                                      &weechat_perl_api_hook_command_cb,
                                                      SvPV (ST (5), PL_na))); /* perl function */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_timer_cb: callback for timer hooked
 */

int
weechat_perl_api_hook_timer_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *perl_argv[1] = { NULL };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    rc = (int *) weechat_perl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    perl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    
    return ret;
}

/*
 * weechat::hook_timer: hook a timer
 */

static XS (XS_weechat_hook_timer)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_timer");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_timer");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_timer (weechat_perl_plugin,
                                                    perl_current_script,
                                                    SvIV (ST (0)), /* interval */
                                                    SvIV (ST (1)), /* align_second */
                                                    SvIV (ST (2)), /* max_calls */
                                                    &weechat_perl_api_hook_timer_cb,
                                                    SvPV (ST (3), PL_na))); /* perl function */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_fd_cb: callback for fd hooked
 */

int
weechat_perl_api_hook_fd_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *perl_argv[1] = { NULL };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    rc = (int *) weechat_perl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    perl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    
    return ret;
}

/*
 * weechat::hook_fd: hook a fd
 */

static XS (XS_weechat_hook_fd)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_fd");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_fd");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_fd (weechat_perl_plugin,
                                                 perl_current_script,
                                                 SvIV (ST (0)), /* fd */
                                                 SvIV (ST (1)), /* read */
                                                 SvIV (ST (2)), /* write */
                                                 SvIV (ST (3)), /* exception */
                                                 &weechat_perl_api_hook_fd_cb,
                                                 SvPV (ST (4), PL_na))); /* perl function */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_print_cb: callback for print hooked
 */

int
weechat_perl_api_hook_print_cb (void *data, struct t_gui_buffer *buffer,
                                time_t date, char *prefix, char *message)
{
    struct t_script_callback *script_callback;
    char *perl_argv[5];
    static char timebuffer[64];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    snprintf (timebuffer, sizeof (timebuffer) - 1, "%ld", date);
    
    perl_argv[0] = script_ptr2str (buffer);
    perl_argv[1] = timebuffer;
    perl_argv[2] = prefix;
    perl_argv[3] = message;
    perl_argv[4] = NULL;
    
    rc = (int *) weechat_perl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    perl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (perl_argv[0])
        free (perl_argv[0]);
    
    return ret;
}

/*
 * weechat::hook_print: hook a print
 */

static XS (XS_weechat_hook_print)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_print");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_print");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_print (weechat_perl_plugin,
                                                    perl_current_script,
                                                    script_str2ptr (SvPV (ST (0), PL_na)), /* buffer */
                                                    SvPV (ST (1), PL_na), /* message */
                                                    SvIV (ST (2)), /* strip_colors */
                                                    &weechat_perl_api_hook_print_cb,
                                                    SvPV (ST (3), PL_na))); /* perl function */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_signal_cb: callback for signal hooked
 */

int
weechat_perl_api_hook_signal_cb (void *data, char *signal, char *type_data,
                                 void *signal_data)
{
    struct t_script_callback *script_callback;
    char *perl_argv[3];
    static char value_str[64];
    int *rc, ret, free_needed;
    
    script_callback = (struct t_script_callback *)data;
    
    perl_argv[0] = signal;
    free_needed = 0;
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        perl_argv[1] = (char *)signal_data;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        snprintf (value_str, sizeof (value_str) - 1,
                  "%d", *((int *)signal_data));
        perl_argv[1] = value_str;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        perl_argv[1] = script_ptr2str (signal_data);
        free_needed = 1;
    }
    else
        perl_argv[1] = NULL;
    perl_argv[2] = NULL;
    
    rc = (int *) weechat_perl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    perl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (free_needed && perl_argv[1])
        free (perl_argv[1]);
    
    return ret;
}

/*
 * weechat::hook_signal: hook a signal
 */

static XS (XS_weechat_hook_signal)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_signal");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_signal");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_signal (weechat_perl_plugin,
                                                     perl_current_script,
                                                     SvPV (ST (0), PL_na), /* signal */
                                                     &weechat_perl_api_hook_signal_cb,
                                                     SvPV (ST (1), PL_na))); /* perl function */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::hook_signal_send: send a signal
 */

static XS (XS_weechat_hook_signal_send)
{
    char *type_data;
    int number;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_signal_send");
	PERL_RETURN_ERROR;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_signal_send");
        PERL_RETURN_ERROR;
    }
    
    type_data = SvPV (ST (1), PL_na);
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        weechat_hook_signal_send (SvPV (ST (0), PL_na), /* signal */
                                  type_data,
                                  SvPV (ST (2), PL_na)); /* signal_data */
        PERL_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        number = SvIV(ST (2));
        weechat_hook_signal_send (SvPV (ST (0), PL_na), /* signal */
                                  type_data,
                                  &number); /* signal_data */
        PERL_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        weechat_hook_signal_send (SvPV (ST (0), PL_na), /* signal */
                                  type_data,
                                  script_str2ptr (SvPV (ST (2), PL_na))); /* signal_data */
        PERL_RETURN_OK;
    }
    
    PERL_RETURN_ERROR;
}

/*
 * weechat_perl_api_hook_config_cb: callback for config option hooked
 */

int
weechat_perl_api_hook_config_cb (void *data, char *type, char *option,
                                 char *value)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    perl_argv[0] = type;
    perl_argv[1] = option;
    perl_argv[2] = value;
    perl_argv[3] = NULL;
    
    rc = (int *) weechat_perl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    perl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    
    return ret;
}

/*
 * weechat::hook_config: hook a config option
 */

static XS (XS_weechat_hook_config)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_config");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_config");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_config (weechat_perl_plugin,
                                                     perl_current_script,
                                                     SvPV (ST (0), PL_na), /* type */
                                                     SvPV (ST (1), PL_na), /* option */
                                                     &weechat_perl_api_hook_config_cb,
                                                     SvPV (ST (2), PL_na))); /* perl function */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_completion_cb: callback for completion hooked
 */

int
weechat_perl_api_hook_completion_cb (void *data, char *completion,
                                     struct t_gui_buffer *buffer,
                                     struct t_weelist *list)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    perl_argv[0] = completion;
    perl_argv[1] = script_ptr2str (buffer);
    perl_argv[2] = script_ptr2str (list);
    perl_argv[3] = NULL;
    
    rc = (int *) weechat_perl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    perl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (perl_argv[1])
        free (perl_argv[1]);
    if (perl_argv[2])
        free (perl_argv[2]);
    
    return ret;
}

/*
 * weechat::hook_completion: hook a completion
 */

static XS (XS_weechat_hook_completion)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_completion");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_completion");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_completion (weechat_perl_plugin,
                                                         perl_current_script,
                                                         SvPV (ST (0), PL_na), /* completion */
                                                         &weechat_perl_api_hook_completion_cb,
                                                         SvPV (ST (1), PL_na))); /* perl function */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_modifier_cb: callback for modifier hooked
 */

char *
weechat_perl_api_hook_modifier_cb (void *data, char *modifier,
                                   char *modifier_data, char *string)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4];
    
    script_callback = (struct t_script_callback *)data;
    
    perl_argv[0] = modifier;
    perl_argv[1] = modifier_data;
    perl_argv[2] = string;
    perl_argv[3] = NULL;
    
    return (char *)weechat_perl_exec (script_callback->script,
                                      WEECHAT_SCRIPT_EXEC_STRING,
                                      script_callback->function,
                                      perl_argv);
}

/*
 * weechat::hook_modifier: hook a modifier
 */

static XS (XS_weechat_hook_modifier)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_modifier");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_modifier");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_modifier (weechat_perl_plugin,
                                                       perl_current_script,
                                                       SvPV (ST (0), PL_na), /* modifier */
                                                       &weechat_perl_api_hook_modifier_cb,
                                                       SvPV (ST (1), PL_na))); /* perl function */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::hook_modifier_exec: execute a modifier hook
 */

static XS (XS_weechat_hook_modifier_exec)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_modifier_exec");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_modifier_exec");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_hook_modifier_exec (SvPV (ST (0), PL_na), /* modifier */
                                         SvPV (ST (1), PL_na), /* modifier_data */
                                         SvPV (ST (2), PL_na)); /* string */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::unhook: unhook something
 */

static XS (XS_weechat_unhook)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("unhook");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("unhook");
        PERL_RETURN_ERROR;
    }
    
    script_api_unhook (weechat_perl_plugin,
                       perl_current_script,
                       script_str2ptr (SvPV (ST (0), PL_na)));
    
    PERL_RETURN_OK;
}

/*
 * weechat::unhook_all: unhook all for script
 */

static XS (XS_weechat_unhook_all)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    (void) items;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("unhook_all");
	PERL_RETURN_ERROR;
    }
    
    script_api_unhook_all (perl_current_script);
    
    PERL_RETURN_OK;
}

/*
 * weechat_perl_api_input_data_cb: callback for input data in a buffer
 */

int
weechat_perl_api_input_data_cb (void *data, struct t_gui_buffer *buffer,
                                char *input_data)
{
    struct t_script_callback *script_callback;
    char *perl_argv[3];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    perl_argv[0] = script_ptr2str (buffer);
    perl_argv[1] = input_data;
    perl_argv[2] = NULL;
    
    rc = (int *) weechat_perl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    perl_argv);
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (perl_argv[0])
        free (perl_argv[0]);
    
    return ret;
}

/*
 * weechat::buffer_new: create a new buffer
 */

static XS (XS_weechat_buffer_new)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_new");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_new");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_buffer_new (weechat_perl_plugin,
                                                    perl_current_script,
                                                    SvPV (ST (0), PL_na), /* category */
                                                    SvPV (ST (1), PL_na), /* name */
                                                    &weechat_perl_api_input_data_cb,
                                                    SvPV (ST (2), PL_na))); /* perl function */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::buffer_search: search a buffer
 */

static XS (XS_weechat_buffer_search)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_search");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_search");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_buffer_search (SvPV (ST (0), PL_na), /* category */
                                                    SvPV (ST (1), PL_na))); /* name */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::buffer_close: close a buffer
 */

static XS (XS_weechat_buffer_close)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_close");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_close");
        PERL_RETURN_ERROR;
    }
    
    script_api_buffer_close (weechat_perl_plugin,
                             perl_current_script,
                             script_str2ptr (SvPV (ST (0), PL_na)), /* buffer */
                             SvIV (ST (1))); /* switch_to_another */
    
    PERL_RETURN_OK;
}

/*
 * weechat::buffer_get: get a buffer property
 */

static XS (XS_weechat_buffer_get)
{
    char *value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_get");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_get");
        PERL_RETURN_EMPTY;
    }
    
    value = weechat_buffer_get (script_str2ptr (SvPV (ST (0), PL_na)), /* buffer */
                                SvPV (ST (1), PL_na)); /* property */
    PERL_RETURN_STRING(value);
}

/*
 * weechat::buffer_set: set a buffer property
 */

static XS (XS_weechat_buffer_set)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_set");
	PERL_RETURN_ERROR;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_set");
        PERL_RETURN_ERROR;
    }
    
    weechat_buffer_set (script_str2ptr (SvPV (ST (0), PL_na)), /* buffer */
                        SvPV (ST (1), PL_na), /* property */
                        SvPV (ST (2), PL_na)); /* value */
    
    PERL_RETURN_OK;
}

/*
 * weechat::nicklist_add_group: add a group in nicklist
 */

static XS (XS_weechat_nicklist_add_group)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_add_group");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_add_group");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_nicklist_add_group (script_str2ptr (SvPV (ST (0), PL_na)), /* buffer */
                                                         script_str2ptr (SvPV (ST (1), PL_na)), /* parent_group */
                                                         SvPV (ST (2), PL_na), /* name */
                                                         SvPV (ST (3), PL_na), /* color */
                                                         SvIV (ST (4)))); /* visible */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::nicklist_search_group: search a group in nicklist
 */

static XS (XS_weechat_nicklist_search_group)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_search_group");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_search_group");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_nicklist_search_group (script_str2ptr (SvPV (ST (0), PL_na)), /* buffer */
                                                            script_str2ptr (SvPV (ST (1), PL_na)), /* from_group */
                                                            SvPV (ST (2), PL_na))); /* name */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::nicklist_add_nick: add a nick in nicklist
 */

static XS (XS_weechat_nicklist_add_nick)
{
    char *prefix, char_prefix, *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_add_nick");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 7)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_add_nick");
        PERL_RETURN_EMPTY;
    }
    
    prefix = SvPV(ST (4), PL_na);
    if (prefix && prefix[0])
        char_prefix = prefix[0];
    else
        char_prefix = ' ';
    
    result = script_ptr2str (weechat_nicklist_add_nick (script_str2ptr (SvPV (ST (0), PL_na)), /* buffer */
                                                        script_str2ptr (SvPV (ST (1), PL_na)), /* group */
                                                        SvPV (ST (2), PL_na), /* name */
                                                        SvPV (ST (3), PL_na), /* color */
                                                        char_prefix,
                                                        SvPV (ST (5), PL_na), /* prefix_color */
                                                        SvIV (ST (6)))); /* visible */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::nicklist_search_nick: search a nick in nicklist
 */

static XS (XS_weechat_nicklist_search_nick)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_search_nick");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_search_nick");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_nicklist_search_nick (script_str2ptr (SvPV (ST (0), PL_na)), /* buffer */
                                                           script_str2ptr (SvPV (ST (1), PL_na)), /* from_group */
                                                           SvPV (ST (2), PL_na))); /* name */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::nicklist_remove_group: remove a group from nicklist
 */

static XS (XS_weechat_nicklist_remove_group)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_group");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_group");
        PERL_RETURN_ERROR;
    }
    
    weechat_nicklist_remove_group (script_str2ptr (SvPV (ST (0), PL_na)), /* buffer */
                                   script_str2ptr (SvPV (ST (1), PL_na))); /* group */
    
    PERL_RETURN_OK;
}

/*
 * weechat::nicklist_remove_nick: remove a nick from nicklist
 */

static XS (XS_weechat_nicklist_remove_nick)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_nick");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_nick");
        PERL_RETURN_ERROR;
    }
    
    weechat_nicklist_remove_nick (script_str2ptr (SvPV (ST (0), PL_na)), /* buffer */
                                  script_str2ptr (SvPV (ST (1), PL_na))); /* nick */
    
    PERL_RETURN_OK;
}

/*
 * weechat::nicklist_remove_all: remove all groups/nicks from nicklist
 */

static XS (XS_weechat_nicklist_remove_all)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_all");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_all");
        PERL_RETURN_ERROR;
    }
    
    weechat_nicklist_remove_all (script_str2ptr (SvPV (ST (0), PL_na))); /* buffer */
    
    PERL_RETURN_OK;
}

/*
 * weechat::command: execute a command on a buffer
 */

static XS (XS_weechat_command)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("command");
        PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("command");
        PERL_RETURN_ERROR;
    }
    
    script_api_command (weechat_perl_plugin,
                        perl_current_script,
                        script_str2ptr (SvPV (ST (0), PL_na)), /* buffer */
                        SvPV (ST (1), PL_na)); /* command */
    
    PERL_RETURN_OK;
}

/*
 * weechat::info_get: get info about WeeChat
 */

static XS (XS_weechat_info_get)
{
    char *value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("info_get");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("info_get");
        PERL_RETURN_EMPTY;
    }
    
    value = weechat_info_get (SvPV (ST (0), PL_na));
    PERL_RETURN_STRING(value);
}

/*
 * weechat_perl_xs_init: initialize subroutines
 */

void
weechat_perl_api_init (pTHX)
{
    HV *stash;
    
    newXS ("DynaLoader::boot_DynaLoader", boot_DynaLoader, __FILE__);
    
    /* interface functions */
    newXS ("weechat::register", XS_weechat_register, "weechat");
    newXS ("weechat::charset_set", XS_weechat_charset_set, "weechat");
    newXS ("weechat::iconv_to_internal", XS_weechat_iconv_to_internal, "weechat");
    newXS ("weechat::iconv_from_internal", XS_weechat_iconv_from_internal, "weechat");
    newXS ("weechat::gettext", XS_weechat_gettext, "weechat");
    newXS ("weechat::ngettext", XS_weechat_ngettext, "weechat");
    newXS ("weechat::mkdir_home", XS_weechat_mkdir_home, "weechat");
    newXS ("weechat::mkdir", XS_weechat_mkdir, "weechat");
    newXS ("weechat::list_new", XS_weechat_list_new, "weechat");
    newXS ("weechat::list_add", XS_weechat_list_add, "weechat");
    newXS ("weechat::list_search", XS_weechat_list_search, "weechat");
    newXS ("weechat::list_casesearch", XS_weechat_list_casesearch, "weechat");
    newXS ("weechat::list_get", XS_weechat_list_get, "weechat");
    newXS ("weechat::list_set", XS_weechat_list_set, "weechat");
    newXS ("weechat::list_next", XS_weechat_list_next, "weechat");
    newXS ("weechat::list_prev", XS_weechat_list_prev, "weechat");
    newXS ("weechat::list_string", XS_weechat_list_string, "weechat");
    newXS ("weechat::list_size", XS_weechat_list_size, "weechat");
    newXS ("weechat::list_remove", XS_weechat_list_remove, "weechat");
    newXS ("weechat::list_remove_all", XS_weechat_list_remove_all, "weechat");
    newXS ("weechat::list_free", XS_weechat_list_free, "weechat");
    newXS ("weechat::config_new", XS_weechat_config_new, "weechat");
    newXS ("weechat::config_new_section", XS_weechat_config_new_section, "weechat");
    newXS ("weechat::config_search_section", XS_weechat_config_search_section, "weechat");
    newXS ("weechat::config_new_option", XS_weechat_config_new_option, "weechat");
    newXS ("weechat::config_search_option", XS_weechat_config_search_option, "weechat");
    newXS ("weechat::config_string_to_boolean", XS_weechat_config_string_to_boolean, "weechat");
    newXS ("weechat::config_option_set", XS_weechat_config_option_set, "weechat");
    newXS ("weechat::config_boolean", XS_weechat_config_boolean, "weechat");
    newXS ("weechat::config_integer", XS_weechat_config_integer, "weechat");
    newXS ("weechat::config_string", XS_weechat_config_string, "weechat");
    newXS ("weechat::config_color", XS_weechat_config_color, "weechat");
    newXS ("weechat::config_write_line", XS_weechat_config_write_line, "weechat");
    newXS ("weechat::config_write", XS_weechat_config_write, "weechat");
    newXS ("weechat::config_read", XS_weechat_config_read, "weechat");
    newXS ("weechat::config_reload", XS_weechat_config_reload, "weechat");
    newXS ("weechat::config_free", XS_weechat_config_free, "weechat");
    newXS ("weechat::prefix", XS_weechat_prefix, "weechat");
    newXS ("weechat::color", XS_weechat_color, "weechat");
    newXS ("weechat::print", XS_weechat_print, "weechat");
    newXS ("weechat::infobar_print", XS_weechat_infobar_print, "weechat");
    newXS ("weechat::infobar_remove", XS_weechat_infobar_remove, "weechat");
    newXS ("weechat::log_print", XS_weechat_log_print, "weechat");
    newXS ("weechat::hook_command", XS_weechat_hook_command, "weechat");
    newXS ("weechat::hook_timer", XS_weechat_hook_timer, "weechat");
    newXS ("weechat::hook_fd", XS_weechat_hook_fd, "weechat");
    newXS ("weechat::hook_print", XS_weechat_hook_print, "weechat");
    newXS ("weechat::hook_signal", XS_weechat_hook_signal, "weechat");
    newXS ("weechat::hook_signal_send", XS_weechat_hook_signal_send, "weechat");
    newXS ("weechat::hook_config", XS_weechat_hook_config, "weechat");
    newXS ("weechat::hook_completion", XS_weechat_hook_completion, "weechat");
    newXS ("weechat::hook_modifier", XS_weechat_hook_modifier, "weechat");
    newXS ("weechat::hook_modifier_exec", XS_weechat_hook_modifier_exec, "weechat");
    newXS ("weechat::unhook", XS_weechat_unhook, "weechat");
    newXS ("weechat::unhook_all", XS_weechat_unhook_all, "weechat");
    newXS ("weechat::buffer_new", XS_weechat_buffer_new, "weechat");
    newXS ("weechat::buffer_search", XS_weechat_buffer_search, "weechat");
    newXS ("weechat::buffer_close", XS_weechat_buffer_close, "weechat");
    newXS ("weechat::buffer_get", XS_weechat_buffer_get, "weechat");
    newXS ("weechat::buffer_set", XS_weechat_buffer_set, "weechat");
    newXS ("weechat::nicklist_add_group", XS_weechat_nicklist_add_group, "weechat");
    newXS ("weechat::nicklist_search_group", XS_weechat_nicklist_search_group, "weechat");
    newXS ("weechat::nicklist_add_nick", XS_weechat_nicklist_add_nick, "weechat");
    newXS ("weechat::nicklist_search_nick", XS_weechat_nicklist_search_nick, "weechat");
    newXS ("weechat::nicklist_remove_group", XS_weechat_nicklist_remove_group, "weechat");
    newXS ("weechat::nicklist_remove_nick", XS_weechat_nicklist_remove_nick, "weechat");
    newXS ("weechat::nicklist_remove_all", XS_weechat_nicklist_remove_all, "weechat");
    newXS ("weechat::command", XS_weechat_command, "weechat");
    newXS ("weechat::info_get", XS_weechat_info_get, "weechat");
    
    /* interface constants */
    stash = gv_stashpv ("weechat", TRUE);
    newCONSTSUB (stash, "weechat::WEECHAT_RC_OK", newSViv (WEECHAT_RC_OK));
    newCONSTSUB (stash, "weechat::WEECHAT_RC_ERROR", newSViv (WEECHAT_RC_ERROR));
    newCONSTSUB (stash, "weechat::WEECHAT_RC_OK_IGNORE_WEECHAT", newSViv (WEECHAT_RC_OK_IGNORE_WEECHAT));
    newCONSTSUB (stash, "weechat::WEECHAT_RC_OK_IGNORE_PLUGINS", newSViv (WEECHAT_RC_OK_IGNORE_PLUGINS));
    newCONSTSUB (stash, "weechat::WEECHAT_RC_OK_IGNORE_ALL", newSViv (WEECHAT_RC_OK_IGNORE_ALL));
    newCONSTSUB (stash, "weechat::WEECHAT_RC_OK_WITH_HIGHLIGHT", newSViv (WEECHAT_RC_OK_WITH_HIGHLIGHT));
    newCONSTSUB (stash, "weechat::WEECHAT_LIST_POS_SORT", newSVpv (WEECHAT_LIST_POS_SORT, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_LIST_POS_BEGINNING", newSVpv (WEECHAT_LIST_POS_BEGINNING, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_LIST_POS_END", newSVpv (WEECHAT_LIST_POS_END, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOTLIST_LOW", newSVpv (WEECHAT_HOTLIST_LOW, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOTLIST_MESSAGE", newSVpv (WEECHAT_HOTLIST_MESSAGE, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOTLIST_PRIVATE", newSVpv (WEECHAT_HOTLIST_PRIVATE, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOTLIST_HIGHLIGHT", newSVpv (WEECHAT_HOTLIST_HIGHLIGHT, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_SIGNAL_STRING", newSVpv (WEECHAT_HOOK_SIGNAL_STRING, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_SIGNAL_INT", newSVpv (WEECHAT_HOOK_SIGNAL_INT, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_SIGNAL_POINTER", newSVpv (WEECHAT_HOOK_SIGNAL_POINTER, PL_na));
}