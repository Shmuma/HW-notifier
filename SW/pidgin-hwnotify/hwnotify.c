/*
 * HWNotify
 * Copyright (C) 2012  Max Lapan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#define PURPLE_PLUGINS

#define VERSION "0.1"

#include <glib.h>
#include <string.h>
#include <stdio.h>

#include <ftdi.h>

#include "notify.h"
#include "plugin.h"
#include "version.h"
#include "debug.h"
#include "cmds.h"
#include "gtkconv.h"
#include "prefs.h"
#include "gtkprefs.h"
#include "gtkutils.h"
#include "gtkplugin.h"
#include "gtkblist.h"


typedef enum {
    LED_NONE,
    LED_RED,
    LED_GREEN,
    LED_BLUE
} led_color;


led_color color_unread = LED_NONE;
led_color color_important = LED_NONE;

gboolean ftdi_ok = FALSE;
struct ftdi_context ctx;

GList* important_list;


led_color str2color (const char* str)
{
    if (str == NULL)
        return LED_NONE;
    if (strcmp (str, "none") == 0)
        return LED_NONE;
    if (strcmp (str, "red") == 0)
        return LED_RED;
    if (strcmp (str, "green") == 0)
        return LED_GREEN;
    if (strcmp (str, "blue") == 0)
        return LED_BLUE;
    return LED_NONE;
}


unsigned char get_led_state ()
{
    unsigned char buf;

    ftdi_read_data (&ctx, &buf, 1);

    return buf;
}


void set_led_state (unsigned char state)
{
    ftdi_write_data (&ctx, &state, 1);   
}


unsigned char set_led (unsigned char buf, led_color color, gboolean state)
{
    int bit = -1;

    if (!ftdi_ok || color == LED_NONE)
        return buf;

    switch (color) {
    case LED_NONE:
        bit = -1;
        break;
    case LED_RED:
        bit = 0;
        break;
    case LED_GREEN:
        bit = 5;
        break;
    case LED_BLUE:
        bit = 6;
        break;
    }

    if (bit < 0)
        return buf;

    if (state)
        buf |= (1 << bit);
    else
        buf &= ~(1 << bit);

    return buf;
}


gboolean is_important (const char* username)
{
    GList *p = important_list;

    while (p) {
        if (strstr (username, (const char*)p->data))
            return TRUE;
        p = p->next;
    }

    return FALSE;
}


void get_pending_events(gboolean* unread, gboolean* important) {
	const char *im=purple_prefs_get_string("/plugins/gtk/ftdi-hwnotify/im");
	const char *chat=purple_prefs_get_string("/plugins/gtk/ftdi-hwnotify/chat");
	GList *l_im = NULL;
	GList *l_chat = NULL;

        *unread = FALSE;
        *important = FALSE;
	
	if (im != NULL && strcmp(im, "always") == 0) {
		l_im = pidgin_conversations_find_unseen_list(PURPLE_CONV_TYPE_IM,
		                                             PIDGIN_UNSEEN_TEXT,
		                                             FALSE, 1);
	} else if (im != NULL && strcmp(im, "hidden") == 0) {
		l_im = pidgin_conversations_find_unseen_list(PURPLE_CONV_TYPE_IM,
		                                             PIDGIN_UNSEEN_TEXT,
		                                             TRUE, 1);
	}

        // check for chat
	if (chat != NULL && strcmp(chat, "always") == 0) {
		l_chat = pidgin_conversations_find_unseen_list(PURPLE_CONV_TYPE_CHAT,
		                                               PIDGIN_UNSEEN_TEXT,
		                                               FALSE, 1);
	} else if (chat != NULL && strcmp(chat, "nick") == 0) {
		l_chat = pidgin_conversations_find_unseen_list(PURPLE_CONV_TYPE_CHAT,
		                                               PIDGIN_UNSEEN_NICK,
		                                               FALSE, 1);
	}

        gboolean unimportant = FALSE;

        // check for important contacts
        if (l_im != NULL && color_important != LED_NONE) {
            GList* p = l_im;

            while (p != NULL) {
                PurpleConversation* conv = (PurpleConversation*)p->data;
                if (is_important (conv->account->username))
                    *important = TRUE;
                else
                    unimportant = TRUE;
                p = p->next;
            }
        }

        if (!*important)
            unimportant = TRUE;

        if (l_im != NULL || l_chat != NULL)
            *unread = unimportant;

        if (l_im != NULL)
            g_list_free (l_im);

        if (l_chat != NULL)
            g_list_free (l_chat);
}


static GList* parse_important_list (const char* data)
{
    const char *p = data, *pp;
    GList* res = NULL;

    while (p != NULL) {
        pp = strchr (p, ',');
        if (pp != NULL) {
            res = g_list_append (res, strndup (p, pp-p));
            pp++;
            purple_debug_info ("hwnotify", "important: %s, %s\n", p, pp);
        }
        else
            res = g_list_append (res, strdup (p));
        p = pp;
    }

    return res;
}


static void hwnotify_conversation_updated(PurpleConversation *conv, 
                                          PurpleConvUpdateType type) {
	if( type != PURPLE_CONV_UPDATE_UNSEEN ) {
		return;
	}

        gboolean unread, important;

	get_pending_events (&unread, &important);

        purple_debug_info ("hwnotify", "pending_events (%d, %d)\n", unread, important);

        unsigned char state = get_led_state ();

        state = set_led (state, color_unread, unread);
        state = set_led (state, color_important, important);

        set_led_state (state);
}


static GtkWidget *plugin_config_frame(PurplePlugin *plugin) {
	GtkWidget *frame;
	GtkWidget *vbox, *vbox2;
	GtkSizeGroup *sg;
	GtkWidget *dd;

	frame = gtk_vbox_new(FALSE, 18);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 12);

	vbox = pidgin_make_frame(frame, "Inform about unread...");
	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	dd = pidgin_prefs_dropdown(vbox, "LED:",
	                           PURPLE_PREF_STRING,
	                           "/plugins/gtk/ftdi-hwnotify/led-one",
	                           "None", "none",
	                           "Blue", "blue",
	                           "Red", "red",
	                           "Green", "green",
	                           NULL);
	gtk_size_group_add_widget(sg, dd);

	dd = pidgin_prefs_dropdown(vbox, "Instant Messages:",
	                           PURPLE_PREF_STRING,
	                           "/plugins/gtk/ftdi-hwnotify/im",
	                           "Never", "never",
	                           "In hidden conversations", "hidden",
	                           "Always", "always",
	                           NULL);
	gtk_size_group_add_widget(sg, dd);

	dd = pidgin_prefs_dropdown(vbox, "Chat Messages:",
	                        PURPLE_PREF_STRING,
	                        "/plugins/gtk/ftdi-hwnotify/chat",
	                        "Never", "never",
	                        "When my nick is said", "nick",
	                        "Always", "always",
	                        NULL);
	gtk_size_group_add_widget(sg, dd);

        vbox2 = pidgin_make_frame(frame, "Important contacts...");
	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	dd = pidgin_prefs_dropdown(vbox2, "LED:",
	                           PURPLE_PREF_STRING,
	                           "/plugins/gtk/ftdi-hwnotify/led-imp",
	                           "None", "none",
	                           "Blue", "blue",
	                           "Red", "red",
	                           "Green", "green",
	                           NULL);
	gtk_size_group_add_widget(sg, dd);
        pidgin_prefs_labeled_entry (vbox2, "Contacts:", "/plugins/gtk/ftdi-hwnotify/contacts-imp", sg);

	gtk_widget_show_all(frame);
	return frame;
}

static void init_plugin(PurplePlugin *plugin) {
	purple_prefs_add_none("/plugins/gtk/ftdi-hwnotify");
	purple_prefs_add_string("/plugins/gtk/ftdi-hwnotify/led-one", "blue");
	purple_prefs_add_string("/plugins/gtk/ftdi-hwnotify/im", "always");
	purple_prefs_add_string("/plugins/gtk/ftdi-hwnotify/chat", "always");
	purple_prefs_add_string("/plugins/gtk/ftdi-hwnotify/led-imp", "red");
	purple_prefs_add_string("/plugins/gtk/ftdi-hwnotify/contacts-imp", "boss,boss2");
}

static gboolean plugin_load(PurplePlugin *plugin) {
    // read settings
    const char* one = purple_prefs_get_string("/plugins/gtk/ftdi-hwnotify/led-one");
    const char* imp = purple_prefs_get_string("/plugins/gtk/ftdi-hwnotify/led-imp");

    color_unread = str2color (one);
    color_important = str2color (imp);

    // init ftdi
    ftdi_ok = FALSE;

    if (ftdi_init (&ctx) >= 0) {
        int f = ftdi_usb_open (&ctx, 0x0403, 0x6001);

        if (f >= 0 || f == -5) {
            ftdi_set_bitmode (&ctx, 0xFF, BITMODE_BITBANG);
            ftdi_ok = TRUE;
            set_led_state (0);
        }
    }

    important_list = parse_important_list (purple_prefs_get_string ("/plugins/gtk/ftdi-hwnotify/contacts-imp"));

    purple_signal_connect(purple_conversations_get_handle(),
                          "conversation-updated", plugin,
                          PURPLE_CALLBACK(hwnotify_conversation_updated), NULL);

    return TRUE;
}

static gboolean plugin_unload(PurplePlugin *plugin) {
    purple_signal_disconnect(purple_conversations_get_handle(),
                             "conversation-updated", plugin,
                             PURPLE_CALLBACK(hwnotify_conversation_updated));

    if (ftdi_ok) {
        ftdi_usb_close (&ctx);
        ftdi_deinit (&ctx);
    }

    if (important_list)
        g_list_free_full (important_list, free);

    return TRUE;
}

static PidginPluginUiInfo ui_info = {
	plugin_config_frame,
	0 /* page_num (Reserved) */
};

static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_STANDARD,
    PIDGIN_PLUGIN_TYPE,
    0,
    NULL,
    PURPLE_PRIORITY_DEFAULT,

    "ftdi-hwnotify",
    "FTDI HW Notification",
    VERSION,

    "FTDI Notification board",
    "Displays incoming messages by leds",
    "Max Lapan <max.lapan@gmail.com>",
    "http://www.shmuma.ru",

    plugin_load,   /* load */
    plugin_unload, /* unload */
    NULL,          /* destroy */

    &ui_info,
    NULL,
    NULL,
    NULL
};

PURPLE_INIT_PLUGIN(hwnotify, init_plugin, info);

