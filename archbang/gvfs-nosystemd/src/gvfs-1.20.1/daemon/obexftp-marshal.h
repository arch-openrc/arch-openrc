
#ifndef __obexftp_marshal_MARSHAL_H__
#define __obexftp_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:STRING (obexftp-marshal.list:1) */
#define obexftp_marshal_VOID__STRING	g_cclosure_marshal_VOID__STRING

/* VOID:UINT64 (obexftp-marshal.list:2) */
extern void obexftp_marshal_VOID__UINT64 (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);

/* VOID:STRING,STRING (obexftp-marshal.list:3) */
extern void obexftp_marshal_VOID__STRING_STRING (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);

/* VOID:STRING,STRING,STRING (obexftp-marshal.list:4) */
extern void obexftp_marshal_VOID__STRING_STRING_STRING (GClosure     *closure,
                                                        GValue       *return_value,
                                                        guint         n_param_values,
                                                        const GValue *param_values,
                                                        gpointer      invocation_hint,
                                                        gpointer      marshal_data);

/* VOID:STRING,STRING,UINT64 (obexftp-marshal.list:5) */
extern void obexftp_marshal_VOID__STRING_STRING_UINT64 (GClosure     *closure,
                                                        GValue       *return_value,
                                                        guint         n_param_values,
                                                        const GValue *param_values,
                                                        gpointer      invocation_hint,
                                                        gpointer      marshal_data);

G_END_DECLS

#endif /* __obexftp_marshal_MARSHAL_H__ */

