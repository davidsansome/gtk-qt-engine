#include <gmodule.h>
#include <stdio.h>
#include "qt_rc_style.h"
#include "qt_style.h"

#include "qt_qt_wrapper.h"

G_MODULE_EXPORT void theme_init (GTypeModule *module);
G_MODULE_EXPORT void theme_exit (void);
G_MODULE_EXPORT GtkRcStyle * theme_create_rc_style (void);

G_MODULE_EXPORT void theme_init (GTypeModule *module)
{
	createQApp();
	qtengine_rc_style_register_type (module);
	qtengine_style_register_type (module);
}

G_MODULE_EXPORT void theme_exit (void)
{
	destroyQApp();
}

G_MODULE_EXPORT GtkRcStyle * theme_create_rc_style (void)
{
	void *ptr = GTK_RC_STYLE (g_object_new (QTENGINE_TYPE_RC_STYLE, NULL));  
	return (GtkRcStyle *)ptr;
}

