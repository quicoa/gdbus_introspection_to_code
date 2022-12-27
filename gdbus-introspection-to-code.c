#include <glib.h>
#include <gio/gio.h>

#define PREFIX "_"
#define INDENT "\t"
#define SKIP_ANNOTATIONS FALSE

static gchar * str_to_lower(gchar *str);
static gchar * str_replace(gchar *str, gchar matching_char, gchar replacing_char);
static char * read_file(const char *path);
static gboolean parse_data(const char *file_content);
static void print_info(const GDBusNodeInfo *node_info);
static void print_interface(GDBusInterfaceInfo *interface);
static gchar * print_every_method(GDBusMethodInfo **methods, const gchar *parent);
static gchar * print_one_method(GDBusMethodInfo *method, const gchar *parent);
static gchar * print_method_pointers(GSList *list, const gchar *parent);
static gchar * print_every_signal(GDBusSignalInfo **signals, const gchar *parent);
static gchar * print_one_signal(GDBusSignalInfo *signal, const gchar *parent);
static gchar * print_signal_pointers(GSList *list, const gchar *parent);
static gchar * print_every_property(GDBusPropertyInfo **properties, const gchar *parent);
static gchar * print_one_property(GDBusPropertyInfo *property, const gchar *parent);
static gchar * print_property_pointers(GSList *list, const gchar *parent);
static gchar * print_every_annotation(GDBusAnnotationInfo **annotations, const gchar *parent);
static gchar * print_one_annotation(GDBusAnnotationInfo *signal, const gchar *parent, gint number);
static gchar * print_annotation_pointers(GSList *list, const gchar *parent);
static gchar * print_every_argument(GDBusArgInfo **arguments, const gchar *parent, const gchar *type);
static gchar * print_one_argument(GDBusArgInfo *arg, const gchar *function_name, const gchar *type);
static gchar * print_argument_pointers(GSList *list, const gchar *parent, const gchar *type);

static gchar *
str_to_lower(gchar *str)
{
	gchar *s;

	for (s = str; *s != '\0'; s++)
	{
		*s = g_ascii_tolower(*s);
	}

	return str;
}

static gchar *
str_replace(gchar *str, gchar matching_char, gchar replacing_char)
{
	gchar *s;

	for (s = str; *s != '\0'; s++)
	{
		if (*s == matching_char)
		{
			*s = replacing_char;
		}
	}

	return str;
}

static char *
read_file(const char *path)
{
	const gchar *msg = "(unknown)";
	char *content = NULL;
	gsize size = 0;
	GFile *file = NULL;
	GError *error = NULL;

	g_assert(path != NULL);

	file = g_file_new_for_commandline_arg(path);

	if (!g_file_load_contents(file, NULL /* GCancellable */, &content, &size, NULL /* etag */, &error))
	{
		if (error != NULL)
		{
			msg = error->message;
		}

		g_printerr("Read error: %s\n", msg);

		if (error != NULL)
		{
			g_error_free(error);
		}

		g_object_unref(file);

		return NULL;
	}
	else if (content == NULL)
	{
		g_printerr("Empty file\n");

		g_object_unref(file);

		return NULL;
	}
	else
	{
		g_object_unref(file);

		return content;
	}
}

static gboolean
parse_data(const char *file_content)
{
	const char *msg = "(unknown)";
	GDBusNodeInfo *node = NULL;
	GError *error = NULL;

	g_assert(file_content != NULL);

	node = g_dbus_node_info_new_for_xml(file_content, &error);

	if (error != NULL)
	{
		msg = error->message;
	}

	if (node == NULL)
	{
		g_printerr("Parsing error: %s", msg);

		if (error != NULL)
		{
			g_error_free(error);
		}

		return FALSE;
	}

	print_info(node);

	g_dbus_node_info_unref(node);

	return TRUE;
}

static void
print_info(const GDBusNodeInfo *node_info)
{
	GDBusInterfaceInfo **interface_array, *interface;

	g_assert(node_info != NULL);

	g_print("#include <glib.h>\n"
	        "#include <gio/gio.h>\n");
	g_print("\n/* Introspection data begins */\n\n");

	for (interface_array = node_info->interfaces; *interface_array != NULL; interface_array++)
	{
		interface = *interface_array;

		print_interface(interface);
	}

	g_print("/* Introspection data ends */\n");
}

static void
print_interface(GDBusInterfaceInfo *interface)
{
	gchar *name, *name_lower;
	gchar *array_properties, *array_methods, *array_signals, *array_annotations;

	g_assert(interface != NULL);

	name = interface->name;
	name_lower = g_strdup(name);
	str_replace(name_lower, '.', '_');
	str_to_lower(name_lower);

	array_methods = print_every_method(interface->methods, name_lower);
	array_signals = print_every_signal(interface->signals, name_lower);
	array_properties = print_every_property(interface->properties, name_lower);
	array_annotations = print_every_annotation(interface->annotations, name_lower);

	g_print("// Interface %s\n", name);

	// Interface info
	g_print("static GDBusInterfaceInfo " PREFIX "%s_interface =\n"
	        "{\n"
	        INDENT "-1,\n"
	        INDENT "\"%s\",\n"
	        INDENT "%s,\n"
	        INDENT "%s,\n"
	        INDENT "%s,\n"
	        INDENT "%s\n"
	        "};\n\n", name_lower, name, array_methods, array_signals, array_properties, array_annotations);

	// Get interface info function
	g_print("GDBusInterfaceInfo *\n"
	        PREFIX "%s_get_interface_info(void)\n"
	        "{\n"
	        INDENT "return &" PREFIX "%s_interface;\n"
	        "}\n\n", name_lower, name_lower);

	g_free(name_lower);
	g_free(array_methods);
	g_free(array_signals);
	g_free(array_properties);
	g_free(array_annotations);
}

static gchar *
print_every_method(GDBusMethodInfo **methods, const gchar *parent)
{
	GDBusMethodInfo **method_array, *method;
	GSList *print_array = NULL;
	gchar *var_name, *array;

	g_assert(parent != NULL);

	if (methods == NULL && methods[0] == NULL)
	{
		return g_strdup("NULL");
	}

	g_print("// Methods for %s\n", parent);

	for (method_array = methods; *method_array != NULL; method_array++)
	{
		method = *method_array;

		g_print("\n");
		var_name = print_one_method(method, parent);

		g_assert(var_name != NULL);

		print_array = g_slist_append(print_array, var_name);
	}

	g_print("\n");
	array = print_method_pointers(print_array, parent);

	g_slist_free_full(print_array, g_free);

	return array;
}

static gchar *
print_one_method(GDBusMethodInfo *method, const gchar *parent)
{
	gchar *var_name, *name, *name_lower, *array_args_in, *array_args_out, *array_annotations;

	g_assert(method != NULL);
	g_assert(parent != NULL);

	name = method->name;
	name_lower = g_strdup(name);
	str_to_lower(name_lower);

	array_args_in = print_every_argument(method->in_args, name_lower, "in");
	array_args_out = print_every_argument(method->out_args, name_lower, "out");
	array_annotations = print_every_annotation(method->annotations, name_lower);

	g_print("// Method %s\n", name);

	var_name = g_strdup_printf(PREFIX "%s_method_%s", parent, name_lower);

	g_print("static GDBusMethodInfo %s =\n"
	        "{\n"
	        INDENT "-1,\n"
	        INDENT "\"%s\",\n"
	        INDENT "%s,\n"
	        INDENT "%s,\n"
	        INDENT "%s\n"
	        "};\n", var_name, name, array_args_in, array_args_out, array_annotations);

	g_free(name_lower);
	g_free(array_args_in);
	g_free(array_args_out);
	g_free(array_annotations);

	return var_name;
}

static gchar *
print_method_pointers(GSList *list, const gchar *parent)
{
	GSList *l;
	gchar *var_name, *array_name, *res;

	g_assert(parent != NULL);

	if (list != NULL)
	{
		array_name = g_strdup_printf(PREFIX "%s_method_pointers", parent);
		g_print("// Array with method pointers\n");
		g_print("static GDBusMethodInfo * %s[] =\n"
		        "{\n", array_name);

		for (l = list; l != NULL; l = l->next)
		{
			var_name = l->data;

			g_print(INDENT "&%s,\n", var_name);
		}

		g_print(INDENT "NULL\n"
		        "};\n\n");

		// Add '&' to string to be used in GDBus info structs
		res = g_strdup_printf("%s", array_name);

		g_free(array_name);

		return res;
	}

	return g_strdup("NULL");
}

static gchar *
print_every_signal(GDBusSignalInfo **signals, const gchar *parent)
{
	GDBusSignalInfo **signal_array, *signal;
	GSList *print_array = NULL;
	gchar *var_name, *array;

	g_assert(parent != NULL);

	if (signals == NULL && signals[0] == NULL)
	{
		return g_strdup("NULL");
	}

	g_print("// Signals for %s\n", parent);

	for (signal_array = signals; *signal_array != NULL; signal_array++)
	{
		signal = *signal_array;

		g_print("\n");
		var_name = print_one_signal(signal, parent);

		g_assert(var_name != NULL);

		print_array = g_slist_append(print_array, var_name);
	}

	g_print("\n");
	array = print_signal_pointers(print_array, parent);

	g_slist_free_full(print_array, g_free);

	return array;
}

static gchar *
print_one_signal(GDBusSignalInfo *signal, const gchar *parent)
{
	gchar *name, *name_lower, *array_args, *array_annotations, *var_name;

	g_assert(signal != NULL);
	g_assert(parent != NULL);

	name = signal->name;
	name_lower = g_strdup(name);
	str_to_lower(name_lower);

	array_args = print_every_argument(signal->args, name_lower, "out");
	array_annotations = print_every_annotation(signal->annotations, name_lower);

	g_print("// Signal %s\n", name);

	var_name = g_strdup_printf(PREFIX "%s_signal_%s", parent, name_lower);

	g_print("static GDBusSignalInfo %s =\n"
	        "{\n"
	        INDENT "-1,\n"
	        INDENT "\"%s\",\n"
	        INDENT "%s,\n"
	        INDENT "%s\n"
	        "};\n", var_name, name, array_args, array_annotations);

	g_free(name_lower);
	g_free(array_args);

	return var_name;
}

static gchar *
print_signal_pointers(GSList *list, const gchar *parent)
{
	GSList *l;
	gchar *var_name, *array_name, *res;

	g_assert(parent != NULL);

	if (list != NULL)
	{
		array_name = g_strdup_printf(PREFIX "%s_signal_pointers", parent);
		g_print("// Array with signal pointers\n");
		g_print("static GDBusSignalInfo * %s[] =\n"
		        "{\n", array_name);

		for (l = list; l != NULL; l = l->next)
		{
			var_name = l->data;

			g_print(INDENT "&%s,\n", var_name);
		}

		g_print(INDENT "NULL\n"
		        "};\n\n");

		// Add '&' to string to be used in GDBus info structs
		res = g_strdup_printf("%s", array_name);

		g_free(array_name);

		return res;
	}

	return g_strdup("NULL");
}

static gchar *
print_every_property(GDBusPropertyInfo **properties, const gchar *parent)
{
	GDBusPropertyInfo **property_array, *property;
	GSList *print_array = NULL;
	gchar *var_name, *array;

	if (properties == NULL || properties[0] == NULL)
	{
		return g_strdup("NULL");
	}

	g_print("// Properties for %s\n", parent);

	for (property_array = properties; *property_array != NULL; property_array++)
	{
		property = *property_array;

		g_print("\n");
		var_name = print_one_property(property, parent);

		g_assert(var_name != NULL);

		print_array = g_slist_append(print_array, var_name);
	}

	g_print("\n");
	array = print_property_pointers(print_array, parent);

	g_slist_free_full(print_array, g_free);

	return array;
}

static gchar *
print_one_property(GDBusPropertyInfo *property, const gchar *parent)
{
	GDBusPropertyInfoFlags flags;
	gchar *var_name, *name, *name_lower, *signature, *flags_str, *annotations;

	g_assert(property != NULL);
	g_assert(parent != NULL);

	name = property->name;
	name_lower = g_strdup(name);
	str_to_lower(name_lower);
	signature = property->signature;
	flags = property->flags;

	annotations = print_every_annotation(property->annotations, name_lower);

	if ((flags & G_DBUS_PROPERTY_INFO_FLAGS_READABLE) &&
	    (flags & G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE))
	{
		flags_str = "G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE";
	}
	else if (flags & G_DBUS_PROPERTY_INFO_FLAGS_READABLE)
	{
		flags_str = "G_DBUS_PROPERTY_INFO_FLAGS_READABLE";
	}
	else if (flags & G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE)
	{
		flags_str = "G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE";
	}
	else
	{
		flags_str = "G_DBUS_PROPERTY_INFO_FLAGS_NONE";
	}

	g_print("// Property %s\n", name);

	var_name = g_strdup_printf(PREFIX "%s_property_%s", parent, name_lower);

	g_print("static GDBusPropertyInfo %s =\n"
	        "{\n"
	        INDENT "-1,\n"
	        INDENT "\"%s\",\n"
	        INDENT "\"%s\",\n"
	        INDENT "%s,\n"
	        INDENT "%s\n"
	        "};\n", var_name, name, signature, flags_str, annotations);

	g_free(name_lower);
	g_free(annotations);

	return var_name;
}

static gchar *
print_property_pointers(GSList *list, const gchar *parent)
{
	GSList *l;
	gchar *var_name, *array_name, *res;

	g_assert(parent != NULL);

	if (list != NULL)
	{
		array_name = g_strdup_printf(PREFIX "%s_property_pointers", parent);
		g_print("// Array with property pointers\n");
		g_print("static GDBusPropertyInfo * %s[] =\n"
		        "{\n", array_name);

		for (l = list; l != NULL; l = l->next)
		{
			var_name = l->data;

			g_print(INDENT "&%s,\n", var_name);
		}

		g_print(INDENT "NULL\n"
		        "};\n\n");

		// Add '&' to string to be used in GDBus info structs
		res = g_strdup_printf("%s", array_name);

		g_free(array_name);

		return res;
	}

	return g_strdup("NULL");
}

static gchar *
print_every_annotation(GDBusAnnotationInfo **annotations, const gchar *parent)
{
	GDBusAnnotationInfo **annotation_array, *annotation;
	GSList *print_array = NULL;
	gchar *var_name, *array;
	gint x = 0;

	if (SKIP_ANNOTATIONS || annotations == NULL || annotations[0] == NULL)
	{
		return g_strdup("NULL");
	}

	g_print("// Annotations for %s\n", parent);

	for (annotation_array = annotations; *annotation_array != NULL; annotation_array++)
	{
		annotation = *annotation_array;

		g_print("\n");
		var_name = print_one_annotation(annotation, parent, x);

		g_assert(var_name != NULL);

		print_array = g_slist_append(print_array, var_name);

		x++;
	}

	g_print("\n");
	array = print_annotation_pointers(print_array, parent);

	g_slist_free_full(print_array, g_free);

	return array;
}

static gchar *
print_one_annotation(GDBusAnnotationInfo *annotation, const gchar *parent, gint number)
{
	gchar *key, *value, *var_name, *child_annotations;

	g_assert(annotation != NULL);
	g_assert(number >= 0);

	key = annotation->key;
	value = annotation->value;

	child_annotations = print_every_annotation(annotation->annotations, key);

	g_print("// Annotation %d\n", number);

	var_name = g_strdup_printf(PREFIX "%s_annotation_%d", parent, number);

	g_print("static GDBusAnnotationInfo %s =\n"
	        "{\n"
	        INDENT "-1,\n"
	        INDENT "\"%s\",\n"
	        INDENT "\"%s\",\n"
	        INDENT "\"%s\"\n"
	        "};\n", var_name, key, value, child_annotations);

	g_free(child_annotations);

	return var_name;
}

static gchar *
print_annotation_pointers(GSList *list, const gchar *parent)
{
	GSList *l;
	gchar *var_name, *array_name, *res;

	g_assert(parent != NULL);

	if (list != NULL)
	{
		array_name = g_strdup_printf(PREFIX "%s_annotation_pointers", parent);
		g_print("// Array with annotation pointers\n");
		g_print("static GDBusAnnotationInfo * %s[] =\n"
		        "{\n", array_name);

		for (l = list; l != NULL; l = l->next)
		{
			var_name = l->data;

			g_print(INDENT "&%s,\n", var_name);
		}

		g_print(INDENT "NULL\n"
		        "};\n\n");

		// Add '&' to string to be used in GDBus info structs
		res = g_strdup_printf("%s", array_name);

		g_free(array_name);

		return res;
	}

	return g_strdup("NULL");
}

static gchar *
print_every_argument(GDBusArgInfo **arguments, const gchar *parent, const gchar *type)
{
	GDBusArgInfo **argument_array, *argument;
	GSList *print_array = NULL;
	gchar *var_name, *array;

	g_assert(parent != NULL);
	g_assert(type != NULL);

	if (arguments == NULL || arguments[0] == NULL)
	{
		return g_strdup("NULL");
	}

	g_print("// Arguments %s for %s\n", parent, type);

	for (argument_array = arguments; *argument_array != NULL; argument_array++)
	{
		argument = *argument_array;

		g_print("\n");
		var_name = print_one_argument(argument, parent, type);

		g_assert(var_name != NULL);

		print_array = g_slist_append(print_array, var_name);
	}

	g_print("\n");
	array = print_argument_pointers(print_array, parent, type);

	g_slist_free_full(print_array, g_free);

	return array;
}

static gchar *
print_one_argument(GDBusArgInfo *arg, const gchar *function_name, const gchar *type)
{
	gchar *var_name, *name, *name_lower, *signature, *annotations;

	g_assert(arg != NULL);
	g_assert(function_name != NULL);

	name = arg->name;
	name_lower = g_strdup(name);
	str_to_lower(name_lower);
	signature = arg->signature;

	annotations = print_every_annotation(arg->annotations, name_lower);

	g_print("// Argument %s\n", name);

	var_name = g_strdup_printf(PREFIX "%s_arg_%s_%s", function_name, name_lower, type);

	g_print("static GDBusArgInfo %s =\n"
	        "{\n"
	        INDENT "-1,\n"
	        INDENT "\"%s\",\n"
	        INDENT "\"%s\",\n"
	        INDENT "%s\n"
	        "};\n", var_name, name, signature, annotations);

	g_free(name_lower);
	g_free(annotations);

	return var_name;
}

static gchar *
print_argument_pointers(GSList *list, const gchar *parent, const gchar *type)
{
	GSList *l;
	gchar *var_name, *array_name, *res;

	g_assert(parent != NULL);
	g_assert(type != NULL);

	if (list != NULL)
	{
		array_name = g_strdup_printf(PREFIX "%s_arg_%s_pointers", parent, type);
		g_print("// Array with argument pointers\n");
		g_print("static GDBusArgInfo * %s[] =\n"
		        "{\n", array_name);

		for (l = list; l != NULL; l = l->next)
		{
			var_name = l->data;

			g_print(INDENT "&%s,\n", var_name);
		}

		g_print(INDENT "NULL\n"
		        "};\n\n");

		// Add '&' to string to be used in GDBus info structs
		res = g_strdup_printf("%s", array_name);

		g_free(array_name);

		return res;
	}

	return g_strdup("NULL");
}

int
main(int argc, const char *argv[])
{
	char *data;
	gint result = 0;

	if (argc <= 1)
	{
		g_printerr("Please provide a file for parameter 1\n");
		return 1;
	}

	data = read_file(argv[1]);

	if (data == NULL)
	{
		return 2;
	}

	if (!parse_data(data))
	{
		result = 3;
	}

	g_free(data);

	return result;
}
