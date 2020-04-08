
#ifndef __libhal_marshal_MARSHAL_H__
#define __libhal_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* NONE:INT,BOXED (libhal-marshal.list:1) */
extern void libhal_marshal_VOID__INT_BOXED (GClosure     *closure,
                                            GValue       *return_value,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint,
                                            gpointer      marshal_data);
#define libhal_marshal_NONE__INT_BOXED	libhal_marshal_VOID__INT_BOXED

/* NONE:STRING,STRING (libhal-marshal.list:2) */
extern void libhal_marshal_VOID__STRING_STRING (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);
#define libhal_marshal_NONE__STRING_STRING	libhal_marshal_VOID__STRING_STRING

/* NONE:STRING,STRING,STRING (libhal-marshal.list:3) */
extern void libhal_marshal_VOID__STRING_STRING_STRING (GClosure     *closure,
                                                       GValue       *return_value,
                                                       guint         n_param_values,
                                                       const GValue *param_values,
                                                       gpointer      invocation_hint,
                                                       gpointer      marshal_data);
#define libhal_marshal_NONE__STRING_STRING_STRING	libhal_marshal_VOID__STRING_STRING_STRING

/* NONE:STRING,BOOLEAN (libhal-marshal.list:4) */
extern void libhal_marshal_VOID__STRING_BOOLEAN (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);
#define libhal_marshal_NONE__STRING_BOOLEAN	libhal_marshal_VOID__STRING_BOOLEAN

/* NONE:STRING,STRING,BOOLEAN (libhal-marshal.list:5) */
extern void libhal_marshal_VOID__STRING_STRING_BOOLEAN (GClosure     *closure,
                                                        GValue       *return_value,
                                                        guint         n_param_values,
                                                        const GValue *param_values,
                                                        gpointer      invocation_hint,
                                                        gpointer      marshal_data);
#define libhal_marshal_NONE__STRING_STRING_BOOLEAN	libhal_marshal_VOID__STRING_STRING_BOOLEAN

/* NONE:STRING,BOOLEAN,BOOLEAN,BOOLEAN (libhal-marshal.list:6) */
extern void libhal_marshal_VOID__STRING_BOOLEAN_BOOLEAN_BOOLEAN (GClosure     *closure,
                                                                 GValue       *return_value,
                                                                 guint         n_param_values,
                                                                 const GValue *param_values,
                                                                 gpointer      invocation_hint,
                                                                 gpointer      marshal_data);
#define libhal_marshal_NONE__STRING_BOOLEAN_BOOLEAN_BOOLEAN	libhal_marshal_VOID__STRING_BOOLEAN_BOOLEAN_BOOLEAN

/* NONE:INT (libhal-marshal.list:7) */
#define libhal_marshal_VOID__INT	g_cclosure_marshal_VOID__INT
#define libhal_marshal_NONE__INT	libhal_marshal_VOID__INT

/* NONE:STRING (libhal-marshal.list:8) */
#define libhal_marshal_VOID__STRING	g_cclosure_marshal_VOID__STRING
#define libhal_marshal_NONE__STRING	libhal_marshal_VOID__STRING

/* NONE:INT,LONG,BOOLEAN,BOOLEAN (libhal-marshal.list:9) */
extern void libhal_marshal_VOID__INT_LONG_BOOLEAN_BOOLEAN (GClosure     *closure,
                                                           GValue       *return_value,
                                                           guint         n_param_values,
                                                           const GValue *param_values,
                                                           gpointer      invocation_hint,
                                                           gpointer      marshal_data);
#define libhal_marshal_NONE__INT_LONG_BOOLEAN_BOOLEAN	libhal_marshal_VOID__INT_LONG_BOOLEAN_BOOLEAN

G_END_DECLS

#endif /* __libhal_marshal_MARSHAL_H__ */

