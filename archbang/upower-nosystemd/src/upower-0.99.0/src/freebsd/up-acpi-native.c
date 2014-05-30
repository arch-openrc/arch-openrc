/* up-acpi-native.c generated by valac, the Vala compiler
 * generated from up-acpi-native.vala, do not modify */


#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>


#define TYPE_UP_ACPI_NATIVE (up_acpi_native_get_type ())
#define UP_ACPI_NATIVE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_UP_ACPI_NATIVE, UpAcpiNative))
#define UP_ACPI_NATIVE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_UP_ACPI_NATIVE, UpAcpiNativeClass))
#define IS_UP_ACPI_NATIVE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_UP_ACPI_NATIVE))
#define IS_UP_ACPI_NATIVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_UP_ACPI_NATIVE))
#define UP_ACPI_NATIVE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_UP_ACPI_NATIVE, UpAcpiNativeClass))

typedef struct _UpAcpiNative UpAcpiNative;
typedef struct _UpAcpiNativeClass UpAcpiNativeClass;
typedef struct _UpAcpiNativePrivate UpAcpiNativePrivate;
#define _g_free0(var) (var = (g_free (var), NULL))
#define _g_regex_unref0(var) ((var == NULL) ? NULL : (var = (g_regex_unref (var), NULL)))
#define _g_match_info_free0(var) ((var == NULL) ? NULL : (var = (g_match_info_free (var), NULL)))
#define _g_error_free0(var) ((var == NULL) ? NULL : (var = (g_error_free (var), NULL)))

struct _UpAcpiNative {
	GObject parent_instance;
	UpAcpiNativePrivate * priv;
};

struct _UpAcpiNativeClass {
	GObjectClass parent_class;
};

struct _UpAcpiNativePrivate {
	gchar	*_driver;
	gint	 _unit;
	gchar	*_path;
};


static gpointer up_acpi_native_parent_class = NULL;

GType up_acpi_native_get_type (void);
#define UP_ACPI_NATIVE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_UP_ACPI_NATIVE, UpAcpiNativePrivate))
enum  {
	UP_ACPI_NATIVE_DUMMY_PROPERTY,
	UP_ACPI_NATIVE_DRIVER,
	UP_ACPI_NATIVE_UNIT,
	UP_ACPI_NATIVE_PATH
};
UpAcpiNative* up_acpi_native_new (const char* path);
UpAcpiNative* up_acpi_native_construct (GType object_type, const char* path);
UpAcpiNative* up_acpi_native_new_driver_unit (const char* driver, gint unit);
UpAcpiNative* up_acpi_native_construct_driver_unit (GType object_type, const char* driver, gint unit);
const char* up_acpi_native_get_driver (UpAcpiNative* self);
gint up_acpi_native_get_unit (UpAcpiNative* self);
const char* up_acpi_native_get_path (UpAcpiNative* self);
static void up_acpi_native_finalize (GObject* obj);
static void up_acpi_native_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);



UpAcpiNative* up_acpi_native_construct (GType object_type, const char* path) {
	GError * _inner_error_;
	UpAcpiNative * self;
	GRegex* r;
	GMatchInfo* mi;
	gboolean ret = FALSE;
	char* _tmp9_;
	g_return_val_if_fail (path != NULL, NULL);
	_inner_error_ = NULL;
	self = (UpAcpiNative*) g_object_new (object_type, NULL);
	r = NULL;
	mi = NULL;
	{
		GRegex* _tmp0_;
		GRegex* _tmp1_;
		GMatchInfo* _tmp4_;
		gboolean _tmp3_;
		GMatchInfo* _tmp2_ = NULL;
		_tmp0_ = g_regex_new ("dev\\.([^\\.])\\.(\\d+)", 0, 0, &_inner_error_);
		if (_inner_error_ != NULL) {
			if (_inner_error_->domain == G_REGEX_ERROR) {
				goto __catch0_g_regex_error;
			}
			goto __finally0;
		}
		r = (_tmp1_ = _tmp0_, _g_regex_unref0 (r), _tmp1_);
		ret = (_tmp3_ = g_regex_match (r, path, 0, &_tmp2_), mi = (_tmp4_ = _tmp2_, _g_match_info_free0 (mi), _tmp4_), _tmp3_);
		if (ret) {
			char* _tmp5_;
			char* _tmp6_;
			self->priv->_driver = (_tmp5_ = g_match_info_fetch (mi, 1), _g_free0 (self->priv->_driver), _tmp5_);
			self->priv->_unit = atoi (_tmp6_ = g_match_info_fetch (mi, 2));
			_g_free0 (_tmp6_);
		} else {
			char* _tmp7_;
			self->priv->_driver = (_tmp7_ = NULL, _g_free0 (self->priv->_driver), _tmp7_);
			self->priv->_unit = -1;
		}
	}
	goto __finally0;
	__catch0_g_regex_error:
	{
		GError * re;
		re = _inner_error_;
		_inner_error_ = NULL;
		{
			char* _tmp8_;
			self->priv->_driver = (_tmp8_ = NULL, _g_free0 (self->priv->_driver), _tmp8_);
			self->priv->_unit = -1;
			_g_error_free0 (re);
		}
	}
	__finally0:
	if (_inner_error_ != NULL) {
		_g_regex_unref0 (r);
		_g_match_info_free0 (mi);
		g_critical ("file %s: line %d: uncaught error: %s (%s, %d)", __FILE__, __LINE__, _inner_error_->message, g_quark_to_string (_inner_error_->domain), _inner_error_->code);
		g_clear_error (&_inner_error_);
		return NULL;
	}
	self->priv->_path = (_tmp9_ = g_strdup (path), _g_free0 (self->priv->_path), _tmp9_);
	_g_regex_unref0 (r);
	_g_match_info_free0 (mi);
	return self;
}


UpAcpiNative* up_acpi_native_new (const char* path) {
	return up_acpi_native_construct (TYPE_UP_ACPI_NATIVE, path);
}


UpAcpiNative* up_acpi_native_construct_driver_unit (GType object_type, const char* driver, gint unit) {
	UpAcpiNative * self;
	char* _tmp0_;
	char* _tmp1_;
	g_return_val_if_fail (driver != NULL, NULL);
	self = (UpAcpiNative*) g_object_new (object_type, NULL);
	self->priv->_driver = (_tmp0_ = g_strdup (driver), _g_free0 (self->priv->_driver), _tmp0_);
	self->priv->_unit = unit;
	self->priv->_path = (_tmp1_ = g_strdup_printf ("dev.%s.%i", self->priv->_driver, self->priv->_unit), _g_free0 (self->priv->_path), _tmp1_);
	return self;
}


UpAcpiNative* up_acpi_native_new_driver_unit (const char* driver, gint unit) {
	return up_acpi_native_construct_driver_unit (TYPE_UP_ACPI_NATIVE, driver, unit);
}


const char* up_acpi_native_get_driver (UpAcpiNative* self) {
	const char* result;
	g_return_val_if_fail (self != NULL, NULL);
	result = self->priv->_driver;
	return result;
}


gint up_acpi_native_get_unit (UpAcpiNative* self) {
	gint result;
	g_return_val_if_fail (self != NULL, 0);
	result = self->priv->_unit;
	return result;
}


const char* up_acpi_native_get_path (UpAcpiNative* self) {
	const char* result;
	g_return_val_if_fail (self != NULL, NULL);
	result = self->priv->_path;
	return result;
}


static void up_acpi_native_class_init (UpAcpiNativeClass * klass) {
	up_acpi_native_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (UpAcpiNativePrivate));
	G_OBJECT_CLASS (klass)->get_property = up_acpi_native_get_property;
	G_OBJECT_CLASS (klass)->finalize = up_acpi_native_finalize;
	g_object_class_install_property (G_OBJECT_CLASS (klass), UP_ACPI_NATIVE_DRIVER, g_param_spec_string ("driver", "driver", "driver", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), UP_ACPI_NATIVE_UNIT, g_param_spec_int ("unit", "unit", "unit", G_MININT, G_MAXINT, 0, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), UP_ACPI_NATIVE_PATH, g_param_spec_string ("path", "path", "path", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
}


static void up_acpi_native_instance_init (UpAcpiNative * self) {
	self->priv = UP_ACPI_NATIVE_GET_PRIVATE (self);
}


static void up_acpi_native_finalize (GObject* obj) {
	UpAcpiNative * self;
	self = UP_ACPI_NATIVE (obj);
	_g_free0 (self->priv->_driver);
	_g_free0 (self->priv->_path);
	G_OBJECT_CLASS (up_acpi_native_parent_class)->finalize (obj);
}


GType up_acpi_native_get_type (void) {
	static GType up_acpi_native_type_id = 0;
	if (up_acpi_native_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (UpAcpiNativeClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) up_acpi_native_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (UpAcpiNative), 0, (GInstanceInitFunc) up_acpi_native_instance_init, NULL };
		up_acpi_native_type_id = g_type_register_static (G_TYPE_OBJECT, "UpAcpiNative", &g_define_type_info, 0);
	}
	return up_acpi_native_type_id;
}


static void up_acpi_native_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec) {
	UpAcpiNative * self;
	self = UP_ACPI_NATIVE (object);
	switch (property_id) {
		case UP_ACPI_NATIVE_DRIVER:
		g_value_set_string (value, up_acpi_native_get_driver (self));
		break;
		case UP_ACPI_NATIVE_UNIT:
		g_value_set_int (value, up_acpi_native_get_unit (self));
		break;
		case UP_ACPI_NATIVE_PATH:
		g_value_set_string (value, up_acpi_native_get_path (self));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

