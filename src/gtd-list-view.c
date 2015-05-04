/* gtd-list-view.c
 *
 * Copyright (C) 2015 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include "gtd-list-view.h"
#include "gtd-manager.h"
#include "gtd-task.h"
#include "gtd-task-list.h"
#include "gtd-task-row.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

typedef struct
{
  GtkListBox            *listbox;
  GtdTaskRow            *new_task_row;
  GtkRevealer           *revealer;

  /* internal */
  gboolean               readonly;
  GList                 *list;
  GtdTaskList           *task_list;
  GtdManager            *manager;
} GtdListViewPrivate;

struct _GtdListView
{
  GtkBox               parent;

  /*<private>*/
  GtdListViewPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE (GtdListView, gtd_list_view, GTK_TYPE_BOX)

enum {
  PROP_0,
  PROP_MANAGER,
  PROP_READONLY,
  LAST_PROP
};

static gint
gtd_list_view__listbox_sort_func (GtdTaskRow *row1,
                                  GtdTaskRow *row2,
                                  gpointer    user_data)
{
  g_return_val_if_fail (GTD_IS_TASK_ROW (row1), 0);
  g_return_val_if_fail (GTD_IS_TASK_ROW (row2), 0);

  if (gtd_task_row_get_new_task_mode (row1))
    return 1;
  else if (gtd_task_row_get_new_task_mode (row2))
    return -1;
  else
    return gtd_task_compare (gtd_task_row_get_task (row1), gtd_task_row_get_task (row2));
}

static void
gtd_list_view__header_func (GtkListBoxRow *row,
                            GtkListBoxRow *before,
                            gpointer       user_data)
{
  if (before)
    {
      GtkWidget *separator;

      separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
      gtk_widget_show (separator);

      gtk_list_box_row_set_header (row, separator);
    }
}

static void
gtd_list_view__clear_list (GtdListView *view)
{
  GList *children;
  GList *l;

  g_return_if_fail (GTD_IS_LIST_VIEW (view));

  children = gtk_container_get_children (GTK_CONTAINER (view->priv->listbox));

  for (l = children; l != NULL; l = l->next)
    {
      if (l->data != view->priv->new_task_row)
        gtk_widget_destroy (l->data);
    }

  gtk_revealer_set_reveal_child (view->priv->revealer, FALSE);

  g_list_free (children);
}

static void
gtd_list_view__add_task (GtdListView *view,
                         GtdTask     *task)
{
  GtdListViewPrivate *priv = view->priv;
  GtkWidget *new_row;

  g_return_if_fail (GTD_IS_LIST_VIEW (view));
  g_return_if_fail (GTD_IS_TASK (task));

  new_row = gtd_task_row_new (task);

  if (!gtd_task_get_complete (task))
    {
      gtk_list_box_insert (priv->listbox,
                           new_row,
                           0);
    }
  else if (!gtk_revealer_get_reveal_child (priv->revealer))
    {
      gtk_revealer_set_reveal_child (priv->revealer, TRUE);
    }
}

static void
gtd_list_view__task_added (GtdTaskList *list,
                           GtdTask     *task,
                           gpointer     user_data)
{
  GtdListViewPrivate *priv = GTD_LIST_VIEW (user_data)->priv;

  g_return_if_fail (GTD_IS_LIST_VIEW (user_data));
  g_return_if_fail (GTD_IS_TASK_LIST (list));
  g_return_if_fail (GTD_IS_TASK (task));

  /* Add the new task to the list */
  gtd_list_view__add_task (GTD_LIST_VIEW (user_data), task);
}

static void
gtd_list_view__create_task (GtdTaskRow *row,
                            GtdTask    *task,
                            gpointer    user_data)
{
  GtdListViewPrivate *priv = GTD_LIST_VIEW (user_data)->priv;
  GtkWidget *new_row;

  g_return_if_fail (GTD_IS_LIST_VIEW (user_data));
  g_return_if_fail (GTD_IS_TASK_ROW (row));
  g_return_if_fail (GTD_IS_TASK (task));
  g_return_if_fail (!priv->readonly);
  g_return_if_fail (priv->task_list);

  /*
   * Newly created tasks are not aware of
   * their parent lists.
   */
  gtd_task_set_list (task, priv->task_list);
  gtd_task_list_save_task (priv->task_list, task);

  gtd_manager_create_task (priv->manager, task);
}

static void
gtd_list_view_finalize (GObject *object)
{
  GtdListView *self = (GtdListView *)object;
  GtdListViewPrivate *priv = gtd_list_view_get_instance_private (self);

  G_OBJECT_CLASS (gtd_list_view_parent_class)->finalize (object);
}

static void
gtd_list_view_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GtdListView *self = GTD_LIST_VIEW (object);

  switch (prop_id)
    {
    case PROP_MANAGER:
      g_value_set_object (value, self->priv->manager);
      break;

    case PROP_READONLY:
      g_value_set_boolean (value, self->priv->readonly);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_list_view_constructed (GObject *object)
{
  GtdListView *self = GTD_LIST_VIEW (object);

  G_OBJECT_CLASS (gtd_list_view_parent_class)->constructed (object);

  /* show a nifty separator between lines */
  gtk_list_box_set_header_func (self->priv->listbox,
                                (GtkListBoxUpdateHeaderFunc) gtd_list_view__header_func,
                                NULL,
                                NULL);

  gtk_list_box_set_sort_func (self->priv->listbox,
                              (GtkListBoxSortFunc) gtd_list_view__listbox_sort_func,
                              NULL,
                              NULL);
}

static void
gtd_list_view_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GtdListView *self = GTD_LIST_VIEW (object);

  switch (prop_id)
    {
    case PROP_MANAGER:
      self->priv->manager = g_value_get_object (value);
      break;

    case PROP_READONLY:
      gtd_list_view_set_readonly (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_list_view_class_init (GtdListViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = gtd_list_view_finalize;
  object_class->constructed = gtd_list_view_constructed;
  object_class->get_property = gtd_list_view_get_property;
  object_class->set_property = gtd_list_view_set_property;

  /**
   * GtdListView::manager:
   *
   * A weak reference to the application's #GtdManager instance.
   */
  g_object_class_install_property (
        object_class,
        PROP_MANAGER,
        g_param_spec_object ("manager",
                             _("Manager of this window's application"),
                             _("The manager of the window's application"),
                             GTD_TYPE_MANAGER,
                             G_PARAM_READWRITE));

  /**
   * GtdListView::readonly:
   *
   * Whether the list shows the "New Task" row or not.
   */
  g_object_class_install_property (
        object_class,
        PROP_READONLY,
        g_param_spec_boolean ("readonly",
                              _("Whether the list is readonly"),
                              _("Whether the list is readonly, i.e. doesn't show the New Task row, or not"),
                              TRUE,
                              G_PARAM_READWRITE));

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/todo/ui/list-view.ui");

  gtk_widget_class_bind_template_child_private (widget_class, GtdListView, listbox);
  gtk_widget_class_bind_template_child_private (widget_class, GtdListView, revealer);
}

static void
gtd_list_view_init (GtdListView *self)
{
  self->priv = gtd_list_view_get_instance_private (self);
  self->priv->readonly = TRUE;
  self->priv->new_task_row = GTD_TASK_ROW (gtd_task_row_new (NULL));
  gtd_task_row_set_new_task_mode (self->priv->new_task_row, TRUE);

  g_signal_connect (self->priv->new_task_row,
                    "create-task",
                    G_CALLBACK (gtd_list_view__create_task),
                    self);

  gtk_widget_init_template (GTK_WIDGET (self));
}

/**
 * gtd_list_view_new:
 *
 * Creates a new #GtdListView
 *
 * Returns: a newly allocated #GtdListView
 */
GtkWidget*
gtd_list_view_new (void)
{
  return g_object_new (GTD_TYPE_LIST_VIEW, NULL);
}

/**
 * gtd_list_view_get_list:
 * @view: a #GtdListView
 *
 * Retrieves the list of tasks from @view. Note that,
 * if a #GtdTaskList is set, the #GtdTaskList's list
 * of task will be returned.
 *
 * Returns: (element-type GtdTaskList) (transfer full): the internal list of
 * tasks. Free with @g_list_free after use.
 */
GList*
gtd_list_view_get_list (GtdListView *view)
{
  g_return_val_if_fail (GTD_IS_LIST_VIEW (view), NULL);

  if (view->priv->task_list)
    return gtd_task_list_get_tasks (view->priv->task_list);
  else if (view->priv->list)
    return g_list_copy (view->priv->list);
  else
    return NULL;
}

/**
 * gtd_list_view_set_list:
 * @view: a #GtdListView
 *
 * Copies the tasks from @list to @view.
 *
 * Returns:
 */
void
gtd_list_view_set_list (GtdListView *view,
                        GList       *list)
{
  g_return_if_fail (GTD_IS_LIST_VIEW (view));

  if (view->priv->list)
    g_list_free (view->priv->list);

  view->priv->list = g_list_copy (list);
}

/**
 * gtd_list_view_get_manager:
 * @view: a #GtdListView
 *
 * Retrieves the #GtdManager from @view.
 *
 * Returns: (transfer none): the #GtdManager of @view
 */
GtdManager*
gtd_list_view_get_manager (GtdListView *view)
{
  g_return_val_if_fail (GTD_IS_LIST_VIEW (view), NULL);

  return view->priv->manager;
}

/**
 * gtd_list_view_set_manager:
 * @view: a #GtdListView
 * @manager: a #GtdManager
 *
 * Sets the #GtdManager of @view.
 *
 * Returns:
 */
void
gtd_list_view_set_manager (GtdListView *view,
                           GtdManager  *manager)
{
  g_return_if_fail (GTD_IS_LIST_VIEW (view));
  g_return_if_fail (GTD_IS_MANAGER (manager));

  if (view->priv->manager != manager)
    {
      view->priv->manager = manager;

      g_object_notify (G_OBJECT (view), "manager");
    }
}

/**
 * gtd_list_view_get_readonly:
 * @view: a #GtdListView
 *
 * Gets the readonly state of @view.
 *
 * Returns: %TRUE if @view is readonly, %FALSE otherwise
 */
gboolean
gtd_list_view_get_readonly (GtdListView *view)
{
  g_return_val_if_fail (GTD_IS_LIST_VIEW (view), FALSE);

  return view->priv->readonly;
}

/**
 * gtd_list_view_set_readonly:
 * @view: a #GtdListView
 *
 * Sets the GtdListView::readonly property of @view.
 *
 * Returns:
 */
void
gtd_list_view_set_readonly (GtdListView *view,
                            gboolean     readonly)
{
  g_return_if_fail (GTD_IS_LIST_VIEW (view));

  if (view->priv->readonly != readonly)
    {
      view->priv->readonly = readonly;

      /* Add/remove the new task row */
      if (gtk_widget_get_parent (GTK_WIDGET (view->priv->new_task_row)))
        {
          if (readonly)
            {
              gtk_container_remove (GTK_CONTAINER (view->priv->listbox), GTK_WIDGET (view->priv->new_task_row));
              gtk_widget_hide (GTK_WIDGET (view->priv->new_task_row));
            }
        }
      else
        {
          if (!readonly)
            {
              gtk_list_box_insert (view->priv->listbox,
                                   GTK_WIDGET (view->priv->new_task_row),
                                   -1);
              gtk_widget_show (GTK_WIDGET (view->priv->new_task_row));
            }
        }

      g_object_notify (G_OBJECT (view), "readonly");
    }
}

/**
 * gtd_list_view_get_task_list:
 * @view: a #GtdListView
 *
 * Retrieves the #GtdTaskList from @view, or %NULL if none was set.
 *
 * Returns: (transfer none): the @GtdTaskList of @view, or %NULL is
 * none was set.
 */
GtdTaskList*
gtd_list_view_get_task_list (GtdListView *view)
{
  g_return_val_if_fail (GTD_IS_LIST_VIEW (view), NULL);

  return view->priv->task_list;
}

/**
 * gtd_list_view_set_task_list:
 * @view: a #GtdListView
 * @list: a #GtdTaskList
 *
 * Sets the internal #GtdTaskList of @view.
 *
 * Returns:
 */
void
gtd_list_view_set_task_list (GtdListView *view,
                             GtdTaskList *list)
{
  GtdListViewPrivate *priv = view->priv;

  g_return_if_fail (GTD_IS_LIST_VIEW (view));
  g_return_if_fail (GTD_IS_TASK_LIST (list));

  if (priv->task_list != list)
    {
      GList *task_list;
      GList *l;

      /*
       * Disconnect the old GtdTaskList signals.
       */
      if (priv->task_list)
        {
          g_signal_handlers_disconnect_by_func (priv->task_list,
                                                gtd_list_view__task_added,
                                                view);
        }

      priv->task_list = list;

      /* clear previous tasks */
      gtd_list_view__clear_list (view);

      /* Add the tasks from the list */
      task_list = gtd_task_list_get_tasks (list);

      for (l = task_list; l != NULL; l = l->next)
        {
          gtd_list_view__add_task (view, l->data);
        }

      g_list_free (task_list);

      g_signal_connect (list,
                        "task-added",
                        G_CALLBACK (gtd_list_view__task_added),
                        view);
    }
}