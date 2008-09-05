/* ply-label.c - APIs for showing text
 *
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Written by: Ray Strode <rstrode@redhat.com>
 */
#include "config.h"
#include "ply-label.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <wchar.h>

#include "ply-label-plugin.h"
#include "ply-event-loop.h"
#include "ply-list.h"
#include "ply-logger.h"
#include "ply-utils.h"

struct _ply_label
{
  ply_event_loop_t *loop;
  ply_module_handle_t *module_handle;
  const ply_label_plugin_interface_t *plugin_interface;
  ply_label_plugin_control_t *control;

  char *text;
};

typedef const ply_label_plugin_interface_t *
        (* get_plugin_interface_function_t) (void);

ply_label_t *
ply_label_new (void)
{
  ply_label_t *label;

  label = calloc (1, sizeof (struct _ply_label));
  return label;
}

void
ply_label_free (ply_label_t *label)
{
  if (label == NULL)
    return;

  free (label);

}

static bool
ply_label_load_plugin (ply_label_t *label)
{
  assert (label != NULL);

  get_plugin_interface_function_t get_label_plugin_interface;

  label->module_handle = ply_open_module (PLYMOUTH_PLUGIN_PATH "label.so");

  if (label->module_handle == NULL)
    return false;

  get_label_plugin_interface = (get_plugin_interface_function_t)
      ply_module_look_up_function (label->module_handle,
                                   "ply_label_plugin_get_interface");

  if (get_label_plugin_interface == NULL)
    {
      ply_save_errno ();
      ply_close_module (label->module_handle);
      label->module_handle = NULL;
      ply_restore_errno ();
      return false;
    }

  label->plugin_interface = get_label_plugin_interface ();

  if (label->plugin_interface == NULL)
    {
      ply_save_errno ();
      ply_close_module (label->module_handle);
      label->module_handle = NULL;
      ply_restore_errno ();
      return false;
    }

  label->control = label->plugin_interface->create_control ();

  if (label->text != NULL)
    label->plugin_interface->set_text_for_control (label->control,
                                                   label->text);

  return true;
}

static void
ply_label_unload_plugin (ply_label_t *label)
{
  assert (label != NULL);
  assert (label->plugin_interface != NULL);
  assert (label->module_handle != NULL);

  ply_close_module (label->module_handle);
  label->plugin_interface = NULL;
  label->module_handle = NULL;
}

bool
ply_label_show (ply_label_t      *label,
                ply_window_t     *window,
                long              x,
                long              y)
{
  if (label->plugin_interface == NULL)
    {
      if (!ply_label_load_plugin (label))
        return false;
    }

  return label->plugin_interface->show_control (label->control,
                                                window, x, y);
}

void
ply_label_draw (ply_label_t *label)
{
  if (label->plugin_interface == NULL)
    return;

  label->plugin_interface->draw_control (label->control);
}

void
ply_label_hide (ply_label_t *label)
{
  if (label->plugin_interface == NULL)
    return;

  label->plugin_interface->hide_control (label->control);
}

bool
ply_label_is_hidden (ply_label_t *label)
{
  if (label->plugin_interface == NULL)
    return true;

  return label->plugin_interface->is_control_hidden (label->control);
}

void
ply_label_set_text (ply_label_t *label,
                    const char  *text)
{

  free (label->text);
  label->text = strdup (text);

  if (label->plugin_interface == NULL)
    return;

  label->plugin_interface->set_text_for_control (label->control,
                                                 text);
}

long
ply_label_get_width (ply_label_t *label)
{
  if (label->plugin_interface == NULL)
    return 0;

  return label->plugin_interface->get_width_of_control (label->control);
}

long
ply_label_get_height (ply_label_t *label)
{
  if (label->plugin_interface == NULL)
    return 0;

  return label->plugin_interface->get_height_of_control (label->control);
}
/* vim: set ts=4 sw=4 expandtab autoindent cindent cino={.5s,(0: */