#ifndef __BUILDER_H__
#define __BUILDER_H__

#include <gtk/gtk.h>

GtkBuilder*	builder_new(const gchar *file);
GtkWidget*	builder_get_widget(GtkBuilder *builder, const gchar *name);

#endif /* __BUILDER_H__ */
