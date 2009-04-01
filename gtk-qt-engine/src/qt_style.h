#include <gtk/gtkstyle.h>

typedef struct _QtEngineStyle QtEngineStyle;
typedef struct _QtEngineStyleClass QtEngineStyleClass;

extern GType qtengine_type_style;

#define QTENGINE_TYPE_STYLE              qtengine_type_style
#define QTENGINE_STYLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), QTENGINE_TYPE_STYLE, QtEngineStyle))
#define QTENGINE_STYLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), QTENGINE_TYPE_STYLE, QtEngineStyleClass))
#define QTENGINE_IS_STYLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), QTENGINE_TYPE_STYLE))
#define QTENGINE_IS_STYLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), QTENGINE_TYPE_STYLE))
#define QTENGINE_STYLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), QTENGINE_TYPE_STYLE, QtEngineStyleClass))

struct _QtEngineStyle
{
	GtkStyle parent_instance;
	
	GdkPixmap* menuBackground;
};

struct _QtEngineStyleClass
{
	GtkStyleClass parent_class;
};

void qtengine_style_register_type (GTypeModule *module);


