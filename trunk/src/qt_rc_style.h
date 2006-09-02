#include <gtk/gtkrc.h>

typedef struct _QtEngineRcStyle QtEngineRcStyle;
typedef struct _QtEngineRcStyleClass QtEngineRcStyleClass;

extern GType qtengine_type_rc_style;

#define QTENGINE_TYPE_RC_STYLE              qtengine_type_rc_style
#define QTENGINE_RC_STYLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), QTENGINE_TYPE_RC_STYLE, QtEngineRcStyle))
#define QTENGINE_RC_STYLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), QTENGINE_TYPE_RC_STYLE, QtEngineRcStyleClass))
#define QTENGINE_IS_RC_STYLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), QTENGINE_TYPE_RC_STYLE))
#define QTENGINE_IS_RC_STYLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), QTENGINE_TYPE_RC_STYLE))
#define QTENGINE_RC_STYLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), QTENGINE_TYPE_RC_STYLE, QtEngineRcStyleClass))

struct _QtEngineRcStyle
{
	GtkRcStyle parent_instance;
};

struct _QtEngineRcStyleClass
{
	GtkRcStyleClass parent_class;
};

void qtengine_rc_style_register_type (GTypeModule *module);


