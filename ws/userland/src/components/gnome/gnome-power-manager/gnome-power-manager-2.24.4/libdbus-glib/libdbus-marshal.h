
#ifndef __libdbus_marshal_MARSHAL_H__
#define __libdbus_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* NONE:STRING,STRING,STRING (libdbus-marshal.list:1) */
extern void libdbus_marshal_VOID__STRING_STRING_STRING (GClosure     *closure,
                                                        GValue       *return_value,
                                                        guint         n_param_values,
                                                        const GValue *param_values,
                                                        gpointer      invocation_hint,
                                                        gpointer      marshal_data);
#define libdbus_marshal_NONE__STRING_STRING_STRING	libdbus_marshal_VOID__STRING_STRING_STRING

G_END_DECLS

#endif /* __libdbus_marshal_MARSHAL_H__ */

