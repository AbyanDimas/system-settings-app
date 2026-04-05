#include "page-mise.h"
#include "../utils/subprocess.h"
#include "../window.h"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static AdwPreferencesGroup *list_group = NULL;
static AdwComboRow *r_lang = NULL;
static AdwEntryRow *r_ver = NULL;
static GtkTextView *tv_log = NULL;
static GtkTextBuffer *log_buf = NULL;
static GtkWidget *list_box = NULL;

static void refresh_mise(void);

static void append_log(const char *text) {
    if (!log_buf) return;
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(log_buf, &end);
    gtk_text_buffer_insert(log_buf, &end, text, -1);
    gtk_text_buffer_insert(log_buf, &end, "\n", -1);
}

static void on_proc_done(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GSubprocess *proc = G_SUBPROCESS(source_object);
    (void)user_data;
    
    g_autoptr(GError) err = NULL;
    g_autofree char *stdout_buf = NULL;
    g_autofree char *stderr_buf = NULL;
    
    g_subprocess_communicate_utf8_finish(proc, res, &stdout_buf, &stderr_buf, &err);
    
    if (stdout_buf && strlen(stdout_buf) > 0) append_log(stdout_buf);
    if (stderr_buf && strlen(stderr_buf) > 0) append_log(stderr_buf);
    append_log("--- Done ---");
    
    refresh_mise();
}

static void run_mise_cmd(const char *cmd) {
    append_log(g_strdup_printf("$ %s", cmd));
    g_autoptr(GError) err = NULL;
    const char *argv[] = {"sh", "-c", cmd, NULL};
    GSubprocess *proc = g_subprocess_newv(argv, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE, &err);
    if (proc) {
        g_subprocess_communicate_utf8_async(proc, NULL, NULL, on_proc_done, NULL);
    }
}

static void on_mise_action(GtkButton *btn, gpointer user_data) {
    (void)btn;
    char **args = user_data; // [action, tool, ver]
    if (strcmp(args[0], "uninstall") == 0) {
        g_autofree char *cmd = g_strdup_printf("mise uninstall %s@%s", args[1], args[2]);
        run_mise_cmd(cmd);
    } else if (strcmp(args[0], "use_g") == 0) {
        g_autofree char *cmd = g_strdup_printf("mise use -g %s@%s", args[1], args[2]);
        run_mise_cmd(cmd);
    }
}

static void free_mise_args(gpointer data, GClosure *closure) {
    (void)closure;
    g_strfreev((char**)data);
}

static void on_install(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    const char *langs[] = {"ruby", "node", "python", "php", "go", "rust", "java", "deno", "bun", "zig", "elixir", "ocaml", "dotnet"};
    guint idx = adw_combo_row_get_selected(r_lang);
    if (idx >= sizeof(langs)/sizeof(langs[0])) return;
    
    const char *lang = langs[idx];
    const char *ver = gtk_editable_get_text(GTK_EDITABLE(r_ver));
    if (strlen(ver) == 0) ver = "latest";

    g_autofree char *cmd = g_strdup_printf("mise use -g %s@%s", lang, ver);
    run_mise_cmd(cmd);
}

static void on_list_remote(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    const char *langs[] = {"ruby", "node", "python", "php", "go", "rust", "java", "deno", "bun", "zig", "elixir", "ocaml", "dotnet"};
    guint idx = adw_combo_row_get_selected(r_lang);
    if (idx >= sizeof(langs)/sizeof(langs[0])) return;
    
    const char *lang = langs[idx];
    g_autofree char *cmd = g_strdup_printf("mise ls-remote %s", lang);
    run_mise_cmd(cmd);
}

static void refresh_mise(void) {
    if (!list_box) return;

    gtk_list_box_remove_all(GTK_LIST_BOX(list_box));

    g_autoptr(GError) err = NULL;
    g_autofree char *out = subprocess_run_sync("mise ls", &err);
    if (!out) return;

    g_auto(GStrv) lines = g_strsplit(out, "\n", -1);
    int c = 0;
    for (int i = 0; lines[i] != NULL; i++) {
        char *line = g_strstrip(g_strdup(lines[i]));
        if (strlen(line) == 0) { g_free(line); continue; }
        
        g_auto(GStrv) toks = g_strsplit_set(line, " \t", -1);
        GPtrArray *real = g_ptr_array_new();
        for (int j=0; toks[j]; j++) {
            if (strlen(toks[j]) > 0) g_ptr_array_add(real, toks[j]);
        }

        if (real->len >= 2) {
            const char *tool = g_ptr_array_index(real, 0);
            const char *ver = g_ptr_array_index(real, 1);
            
            gboolean is_active = (strstr(line, "~") != NULL || strstr(line, "active") != NULL || strstr(line, "config.toml") != NULL);

            AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), tool);
            adw_action_row_set_subtitle(row, g_strdup_printf("Version %s %s", ver, is_active ? "(Active)" : "(Installed)"));

            GtkWidget *bb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

            GtkWidget *bs = gtk_button_new_with_label("Set Global");
            gtk_widget_add_css_class(bs, "suggested-action");
            gtk_widget_set_valign(bs, GTK_ALIGN_CENTER);
            char **args_s = g_new0(char*, 4);
            args_s[0] = g_strdup("use_g"); args_s[1] = g_strdup(tool); args_s[2] = g_strdup(ver);
            g_signal_connect_data(bs, "clicked", G_CALLBACK(on_mise_action), args_s, free_mise_args, 0);
            gtk_box_append(GTK_BOX(bb), bs);

            GtkWidget *bu = gtk_button_new_with_label("Uninstall");
            gtk_widget_add_css_class(bu, "destructive-action");
            gtk_widget_set_valign(bu, GTK_ALIGN_CENTER);
            char **args_u = g_new0(char*, 4);
            args_u[0] = g_strdup("uninstall"); args_u[1] = g_strdup(tool); args_u[2] = g_strdup(ver);
            g_signal_connect_data(bu, "clicked", G_CALLBACK(on_mise_action), args_u, free_mise_args, 0);
            gtk_box_append(GTK_BOX(bb), bu);

            adw_action_row_add_suffix(row, bb);
            gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(row));
            c++;
        }
        g_ptr_array_unref(real);
        g_free(line);
    }
    if (c == 0) {
        AdwActionRow *r2 = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r2), "No language environments installed.");
        gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(r2));
    }
}

GtkWidget *page_mise_new(void) {
    if (!g_find_program_in_path("mise")) {
        return omarchy_make_unavailable_page("Developer Runtimes", "curl https://mise.run | sh", "applications-development-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Runtimes / Mise");
    adw_preferences_page_set_icon_name(page, "applications-development-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    /* ── Install ───────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *inst_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(inst_group, "Install Language Runtime");
    adw_preferences_page_add(page, inst_group);

    GtkStringList *lm = gtk_string_list_new(NULL);
    const char *langs[] = {"ruby", "node", "python", "php", "go", "rust", "java", "deno", "bun", "zig", "elixir", "ocaml", "dotnet"};
    for (int i=0; i<13; i++) gtk_string_list_append(lm, langs[i]);

    r_lang = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_lang), "Language Plugin");
    adw_combo_row_set_model(r_lang, G_LIST_MODEL(lm));
    adw_preferences_group_add(inst_group, GTK_WIDGET(r_lang));

    r_ver = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_ver), "Version (default: 'latest')");
    adw_preferences_group_add(inst_group, GTK_WIDGET(r_ver));

    AdwActionRow *aa_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(aa_row), "Execute Installation");
    
    GtkWidget *bb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *bl = gtk_button_new_with_label("List Remote Versions");
    gtk_widget_set_valign(bl, GTK_ALIGN_CENTER);
    g_signal_connect(bl, "clicked", G_CALLBACK(on_list_remote), NULL);
    gtk_box_append(GTK_BOX(bb), bl);

    GtkWidget *bi = gtk_button_new_with_label("Install Global");
    gtk_widget_add_css_class(bi, "suggested-action");
    gtk_widget_set_valign(bi, GTK_ALIGN_CENTER);
    g_signal_connect(bi, "clicked", G_CALLBACK(on_install), NULL);
    gtk_box_append(GTK_BOX(bb), bi);
    
    adw_action_row_add_suffix(aa_row, bb);
    adw_preferences_group_add(inst_group, GTK_WIDGET(aa_row));

    /* ── Log ───────────────────────────────────────────────────────────────── */
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_size_request(scroll, -1, 150);
    tv_log = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_text_view_set_editable(tv_log, FALSE);
    gtk_text_view_set_monospace(tv_log, TRUE);
    gtk_text_view_set_bottom_margin(tv_log, 6);
    gtk_text_view_set_top_margin(tv_log, 6);
    gtk_text_view_set_left_margin(tv_log, 6);
    gtk_text_view_set_right_margin(tv_log, 6);
    log_buf = gtk_text_view_get_buffer(tv_log);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), GTK_WIDGET(tv_log));
    adw_preferences_group_add(inst_group, scroll);

    /* ── Installed ─────────────────────────────────────────────────────────── */
    list_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(list_group, "Installed Environments");
    
    GtkWidget *top_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *ref_b = gtk_button_new_from_icon_name("view-refresh-symbolic");
    g_signal_connect(ref_b, "clicked", G_CALLBACK(refresh_mise), NULL);
    gtk_box_append(GTK_BOX(top_box), ref_b);
    adw_preferences_group_set_header_suffix(list_group, top_box);
    adw_preferences_page_add(page, list_group);

    list_box = gtk_list_box_new();
    gtk_widget_add_css_class(list_box, "boxed-list");
    adw_preferences_group_add(list_group, list_box);

    refresh_mise();

    return overlay;
}
