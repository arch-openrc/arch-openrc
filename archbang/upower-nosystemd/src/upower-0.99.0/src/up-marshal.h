
#ifndef __up_marshal_MARSHAL_H__
#define __up_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:BOOLEAN,STRING,BOOLEAN,INT,INT,STRING,DOUBLE (up-marshal.list:1) */
extern void up_marshal_VOID__BOOLEAN_STRING_BOOLEAN_INT_INT_STRING_DOUBLE (GClosure     *closure,
                                                                           GValue       *return_value,
                                                                           guint         n_param_values,
                                                                           const GValue *param_values,
                                                                           gpointer      invocation_hint,
                                                                           gpointer      marshal_data);

/* VOID:STRING,BOOLEAN,STRING,BOOLEAN,INT,INT,STRING,DOUBLE (up-marshal.list:2) */
extern void up_marshal_VOID__STRING_BOOLEAN_STRING_BOOLEAN_INT_INT_STRING_DOUBLE (GClosure     *closure,
                                                                                  GValue       *return_value,
                                                                                  guint         n_param_values,
                                                                                  const GValue *param_values,
                                                                                  gpointer      invocation_hint,
                                                                                  gpointer      marshal_data);

/* VOID:STRING,INT (up-marshal.list:3) */
extern void up_marshal_VOID__STRING_INT (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data);

/* VOID:POINTER,BOOLEAN (up-marshal.list:4) */
extern void up_marshal_VOID__POINTER_BOOLEAN (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

/* VOID:POINTER,POINTER (up-marshal.list:5) */
extern void up_marshal_VOID__POINTER_POINTER (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

/* VOID:POINTER,POINTER,BOOLEAN (up-marshal.list:6) */
extern void up_marshal_VOID__POINTER_POINTER_BOOLEAN (GClosure     *closure,
                                                      GValue       *return_value,
                                                      guint         n_param_values,
                                                      const GValue *param_values,
                                                      gpointer      invocation_hint,
                                                      gpointer      marshal_data);

G_END_DECLS

#endif /* __up_marshal_MARSHAL_H__ */

