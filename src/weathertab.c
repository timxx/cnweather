#include "weathertab.h"

#define WEATHER_TAB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), weather_tab_get_type(), WeatherTabPrivate))

enum
{
	PROP_0,
	PROP_PAGE,
	PROP_PARENT
};

struct _WeatherTabPrivate
{
	GtkWidget*	label_title;	// title in tab
	GtkWidget*	button_close;	// close button

	GtkWidget*	page;			// WeatherPage
	GtkWidget*	parent;			// parent holds this tab
};

static void weather_tab_init(WeatherTab *tab);
static void weather_tab_class_init(WeatherTabClass *klass);

static void
weather_tab_set_property(GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec);

static void
weather_tab_get_property(GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec);

static void on_button_close_clicked(GtkButton *button, gpointer data);

GtkWidget* weather_tab_new(GtkWidget *parent, GtkWidget *page)
{
	return g_object_new(TYPE_WEATHER_TAB,
				"spacing", 5,
				"weather-page", page,
				"parent-notebook", parent,
				NULL);
}

GType weather_tab_get_type()
{
	static GType type;

    if (!type)
    {
        static const GTypeInfo info =
        {
            sizeof(WeatherTabClass),				    // class size
            NULL,									    // base init
            NULL,									    // base finalize
            (GClassInitFunc)weather_tab_class_init,		// class init
            NULL,									    // class finalize
            NULL,									    // data
            sizeof(WeatherTab),							// instance size
            0,										    // prealloc
            (GInstanceInitFunc)weather_tab_init			// instance init
        };

        type = g_type_register_static(GTK_TYPE_HBOX, "WeatherTab", &info, 0);
    }

    return type;
}

static void weather_tab_init(WeatherTab *tab)
{
	WeatherTabPrivate *priv;
	GtkWidget *image;

	tab->priv = WEATHER_TAB_GET_PRIVATE(tab);
	priv = tab->priv;

	priv->page = NULL;

	priv->label_title = gtk_label_new("");
	priv->button_close = gtk_button_new();

	image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(priv->button_close), image);

	gtk_button_set_relief(GTK_BUTTON(priv->button_close), GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click(GTK_BUTTON(priv->button_close), FALSE);

	gtk_box_pack_start(GTK_BOX(tab), priv->label_title, FALSE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(tab), priv->button_close, FALSE, TRUE, 0);

	g_signal_connect(priv->button_close, "clicked",
				G_CALLBACK(on_button_close_clicked),
				tab);

	gtk_widget_show_all(GTK_WIDGET(tab));
}

static void weather_tab_class_init(WeatherTabClass *klass)
{
	GObjectClass *g_object_class = (GObjectClass *)klass;

	g_object_class->set_property = weather_tab_set_property;
	g_object_class->get_property = weather_tab_get_property;

	g_object_class_install_property(g_object_class,
					 PROP_PAGE,
					 g_param_spec_object ("weather-page",
							      "Weather page",
							      "Weather page",
							      GTK_TYPE_WIDGET,
							      G_PARAM_READWRITE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
					 );

	g_object_class_install_property(g_object_class,
					 PROP_PARENT,
					 g_param_spec_object ("parent-notebook",
							      "Notebook",
							      "Notebook",
							      GTK_TYPE_WIDGET,
							      G_PARAM_READWRITE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
					 );

	g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(WeatherTabPrivate));
}

static void
weather_tab_set_property(GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	WeatherTab *tab = WEATHER_TAB(object);

	switch (prop_id)
	{
		case PROP_PAGE:
			tab->priv->page = g_value_get_object(value);
			break;

		case PROP_PARENT:
			tab->priv->parent = g_value_get_object(value);
			break;
	}
}

static void
weather_tab_get_property(GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	WeatherTab *tab = WEATHER_TAB(object);

	switch (prop_id)
	{
		case PROP_PAGE:
			g_value_set_object(value, tab->priv->page);
			break;

		case PROP_PARENT:
			g_value_set_object(value, tab->priv->parent);
			break;
	}
}

static void on_button_close_clicked(GtkButton *button, gpointer data)
{
	WeatherTab *tab = (WeatherTab *)data;
	gint page;

	page = gtk_notebook_page_num(GTK_NOTEBOOK(tab->priv->parent), tab->priv->page);
	gtk_notebook_remove_page(GTK_NOTEBOOK(tab->priv->parent), page);
}

void weather_tab_set_title(WeatherTab *tab, const gchar *title)
{
	g_return_if_fail(tab != NULL);

	gtk_label_set_text(GTK_LABEL(tab->priv->label_title), title);
}

const gchar* weather_tab_get_title(WeatherTab *tab)
{
	g_return_val_if_fail(tab != NULL, NULL);

	return gtk_label_get_text(GTK_LABEL(tab->priv->label_title));
}

void weather_tab_set_tooltip(WeatherTab *tab, const gchar *text)
{
	gtk_widget_set_tooltip_text(GTK_WIDGET(tab), text);
}
