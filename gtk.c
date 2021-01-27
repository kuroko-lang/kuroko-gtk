#include <gtk/gtk.h>
#include <kuroko.h>
#include <vm.h>
#include <value.h>
#include <object.h>
#define S(c) (krk_copyString(c,sizeof(c)-1))

static KrkInstance * module;

static KrkClass * createClass(const char * name, KrkClass * base) {
	if (!base) base = vm.objectClass;
	KrkString * str_Name = krk_copyString(name, strlen(name));
	krk_push(OBJECT_VAL(str_Name));
	KrkClass * obj_Class = krk_newClass(str_Name, base);
	krk_push(OBJECT_VAL(obj_Class));
	krk_attachNamedObject(&module->fields, name, (KrkObj *)obj_Class);
	krk_pop(); /* obj_Class */
	krk_pop(); /* str_Name */
	return obj_Class;
}

static KrkClass * KrkGtkApplication;
struct KrkGtkApplication {
	KrkInstance inst;
	GtkApplication * app;
};

static KrkClass * KrkGtkWidget;
struct KrkGtkWidget {
	KrkInstance inst;
	GtkWidget * widget;
};

static KrkClass * KrkGtkWindow;

static KrkValue _gtk_application_init(int argc, KrkValue argv[], int hasKw) {
	if (argc < 1 || !krk_isInstanceOf(argv[0], KrkGtkApplication))
		return krk_runtimeError(vm.exceptions.typeError, "expected GtkApplication");
	if (argc < 2 || !IS_STRING(argv[1]))
		return krk_runtimeError(vm.exceptions.typeError, "expected string");

	struct KrkGtkApplication * self = (void*)AS_INSTANCE(argv[0]);

	self->app = gtk_application_new(AS_CSTRING(argv[1]), G_APPLICATION_FLAGS_NONE);

	return argv[0];
}

static void _krk_gtk_callback_internal(GtkApplication * app, gpointer user_data) {
	KrkObj * userData = (void*)user_data;
	krk_callSimple(OBJECT_VAL(userData), 0, 0);
}

static KrkValue _gtk_application_signal_connect(int argc, KrkValue argv[], int hasKw) {
	if (argc < 1 || !krk_isInstanceOf(argv[0], KrkGtkApplication))
		return krk_runtimeError(vm.exceptions.typeError, "expected GtkApplication");
	if (argc < 2 || !IS_STRING(argv[1]))
		return krk_runtimeError(vm.exceptions.typeError, "expected string for signal type");
	if (argc < 3 || !IS_OBJECT(argv[2]))
		return krk_runtimeError(vm.exceptions.argumentError, "expected a callback");

	struct KrkGtkApplication * self = (void*)AS_INSTANCE(argv[0]);

	char * tmp = malloc(AS_STRING(argv[1])->length + 6);
	sprintf(tmp, "__c_%s", AS_CSTRING(argv[1]));
	krk_attachNamedValue(&self->inst.fields, tmp, argv[2]);
	free(tmp);

	g_signal_connect(self->app, AS_CSTRING(argv[1]), G_CALLBACK(_krk_gtk_callback_internal), (void*)AS_OBJECT(argv[2]));
	return NONE_VAL();
}

static KrkValue _gtk_application_run(int argc, KrkValue argv[], int hasKw) {
	if (argc < 1 || !krk_isInstanceOf(argv[0], KrkGtkApplication))
		return krk_runtimeError(vm.exceptions.typeError, "expected GtkApplication");
	struct KrkGtkApplication * self = (void*)AS_INSTANCE(argv[0]);

	/* TODO argc, argv */
	int status = g_application_run(G_APPLICATION(self->app), 0, NULL);

	return INTEGER_VAL(status);
}

static KrkValue _gtk_widget_show_all(int argc, KrkValue argv[], int hasKw) {
	if (argc < 1 || !krk_isInstanceOf(argv[0], KrkGtkWidget))
		return krk_runtimeError(vm.exceptions.typeError, "expected GtkWidget");
	struct KrkGtkWidget * self = (void*)AS_INSTANCE(argv[0]);
	gtk_widget_show_all(self->widget);
	return NONE_VAL();
}

static KrkValue _gtk_window_init(int argc, KrkValue argv[], int hasKw) {
	if (argc < 1 || !krk_isInstanceOf(argv[0], KrkGtkWindow))
		return krk_runtimeError(vm.exceptions.typeError, "expected GtkWindow");
	if (argc < 2 || !krk_isInstanceOf(argv[1], KrkGtkApplication))
		return krk_runtimeError(vm.exceptions.typeError, "expected GtkApplication");
	struct KrkGtkWidget * self = (void*)AS_INSTANCE(argv[0]);
	struct KrkGtkApplication * app = (void*)AS_INSTANCE(argv[1]);
	self->widget = gtk_application_window_new(app->app);
	return argv[0];
}

static KrkValue _gtk_window_set_title(int argc, KrkValue argv[], int hasKw) {
	if (argc < 1 || !krk_isInstanceOf(argv[0], KrkGtkWindow))
		return krk_runtimeError(vm.exceptions.typeError, "expected GtkWindow");
	if (argc < 2 || !IS_STRING(argv[1]))
		return krk_runtimeError(vm.exceptions.typeError, "expected string");
	struct KrkGtkWidget * self = (void*)AS_INSTANCE(argv[0]);
	gtk_window_set_title(GTK_WINDOW(self->widget), AS_CSTRING(argv[1]));
	return NONE_VAL();
}

static KrkValue _gtk_window_set_default_size(int argc, KrkValue argv[], int hasKw) {
	if (argc < 1 || !krk_isInstanceOf(argv[0], KrkGtkWindow))
		return krk_runtimeError(vm.exceptions.typeError, "expected GtkWindow");
	if (argc < 3 || !IS_INTEGER(argv[1]) || !IS_INTEGER(argv[2]))
		return krk_runtimeError(vm.exceptions.typeError, "expected two ints");
	struct KrkGtkWidget * self = (void*)AS_INSTANCE(argv[0]);
	gtk_window_set_default_size(GTK_WINDOW(self->widget), AS_INTEGER(argv[1]), AS_INTEGER(argv[2]));
	return NONE_VAL();
}

KrkValue krk_module_onload_gtk(void) {
	module = krk_newInstance(vm.moduleClass);
	krk_push(OBJECT_VAL(module));
	KrkGtkApplication = createClass("GtkApplication", NULL);
	KrkGtkApplication->allocSize = sizeof(struct KrkGtkApplication);
	krk_defineNative(&KrkGtkApplication->methods, ".__init__", _gtk_application_init);
	krk_defineNative(&KrkGtkApplication->methods, ".signal_connect", _gtk_application_signal_connect);
	krk_defineNative(&KrkGtkApplication->methods, ".run", _gtk_application_run);
	krk_finalizeClass(KrkGtkApplication);

	KrkGtkWidget = createClass("GtkWidget", NULL);
	KrkGtkWidget->allocSize = sizeof(struct KrkGtkWidget);
	krk_defineNative(&KrkGtkWidget->methods, ".show_all", _gtk_widget_show_all);
	krk_finalizeClass(KrkGtkWidget);

	/* should be Widget -> Container -> Bin -> Window */
	KrkGtkWindow = createClass("GtkWindow", KrkGtkWidget);
	krk_defineNative(&KrkGtkWindow->methods, ".__init__", _gtk_window_init);
	krk_defineNative(&KrkGtkWindow->methods, ".set_title", _gtk_window_set_title);
	krk_defineNative(&KrkGtkWindow->methods, ".set_default_size", _gtk_window_set_default_size);
	krk_finalizeClass(KrkGtkWindow);

	/* Also -> Dialog -> Lots of sublcasses */

	return krk_pop();
}
