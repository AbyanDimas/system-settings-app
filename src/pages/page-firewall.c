#include "page-firewall.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static AdwPreferencesGroup *rules_group = NULL;
static GtkWidget *rules_list_box = NULL;
static AdwSwitchRow *r_enable = NULL;
static AdwComboRow *r_def_in = NULL;
static AdwComboRow *r_def_out = NULL;

static GtkTextBuffer *log_buf = NULL;

static void free_g_free(gpointer data, GClosure *closure) {
    (void)closure;
    g_free(data);
}

static void refresh_firewall(void);

static void append_log(const char *cmd, const char *out) {
    if (!log_buf) return;
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(log_buf, &end);
    g_autofree char *full = g_strdup_printf("$ %s\n%s\n", cmd, out ? out : "");
    gtk_text_buffer_insert(log_buf, &end, full, -1);
}

static void on_pkexec_done(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GSubprocess *proc = G_SUBPROCESS(source_object);
    char *cmd = user_data;
    
    g_autoptr(GError) err = NULL;
    g_autofree char *stdout_buf = NULL;
    g_autofree char *stderr_buf = NULL;
    
    g_subprocess_communicate_utf8_finish(proc, res, &stdout_buf, &stderr_buf, &err);
    
    if (stdout_buf) g_strstrip(stdout_buf);
    append_log(cmd, stdout_buf);
    
    refresh_firewall();
    g_free(cmd);
}

static void run_pkexec_async(const char *cmd) {
    g_autoptr(GError) err = NULL;
    const char *argv[] = {"sh", "-c", cmd, NULL};
    GSubprocess *proc = g_subprocess_newv(argv, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_MERGE, &err);
    if (proc) {
        g_subprocess_communicate_utf8_async(proc, NULL, NULL, on_pkexec_done, g_strdup(cmd));
    }
}

static void on_enable_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    if (adw_switch_row_get_active(ADW_SWITCH_ROW(obj))) {
        run_pkexec_async("pkexec ufw enable");
    } else {
        run_pkexec_async("pkexec ufw disable");
    }
}

static void on_def_in_changed(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    const char *pols[] = {"deny", "allow", "reject"};
    guint idx = adw_combo_row_get_selected(ADW_COMBO_ROW(obj));
    g_autofree char *cmd = g_strdup_printf("pkexec ufw default %s incoming", pols[idx]);
    run_pkexec_async(cmd);
}

static void on_def_out_changed(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    const char *pols[] = {"deny", "allow", "reject"};
    guint idx = adw_combo_row_get_selected(ADW_COMBO_ROW(obj));
    g_autofree char *cmd = g_strdup_printf("pkexec ufw default %s outgoing", pols[idx]);
    run_pkexec_async(cmd);
}

static void on_delete_rule(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *num = user_data;
    g_autofree char *cmd = g_strdup_printf("pkexec ufw --force delete %s", num);
    run_pkexec_async(cmd);
}

static void refresh_firewall(void) {
    if (!rules_list_box) return;
    gtk_list_box_remove_all(GTK_LIST_BOX(rules_list_box));

    g_autoptr(GError) err = NULL;
    g_autofree char *out = subprocess_run_sync("pkexec ufw status numbered", &err);
    if (!out) return;

    // Status: active
    if (strstr(out, "Status: active")) {
        g_signal_handlers_block_by_func(r_enable, on_enable_toggled, NULL);
        adw_switch_row_set_active(r_enable, TRUE);
        g_signal_handlers_unblock_by_func(r_enable, on_enable_toggled, NULL);
    } else {
        g_signal_handlers_block_by_func(r_enable, on_enable_toggled, NULL);
        adw_switch_row_set_active(r_enable, FALSE);
        g_signal_handlers_unblock_by_func(r_enable, on_enable_toggled, NULL);
    }

    g_auto(GStrv) lines = g_strsplit(out, "\n", -1);
    for (int i = 0; lines[i] != NULL; i++) {
        char *line = g_strstrip(g_strdup(lines[i]));
        // [ 1] 80/tcp                     ALLOW IN    Anywhere
        if (line[0] == '[' && strchr(line, ']')) {
            char *rbkt = strchr(line, ']');
            char *num = g_strndup(line + 1, rbkt - (line + 1));
            g_strstrip(num);

            char *rest = g_strdup(rbkt + 1);
            g_strstrip(rest);

            AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
            g_autofree char *t = g_strdup_printf("Rule %s", num);
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), t);
            adw_action_row_set_subtitle(row, rest);

            GtkWidget *del_btn = gtk_button_new_from_icon_name("user-trash-symbolic");
            gtk_widget_add_css_class(del_btn, "destructive-action");
            gtk_widget_set_valign(del_btn, GTK_ALIGN_CENTER);
            g_signal_connect_data(del_btn, "clicked", G_CALLBACK(on_delete_rule), g_strdup(num), free_g_free, 0);
            adw_action_row_add_suffix(row, del_btn);

            gtk_list_box_append(GTK_LIST_BOX(rules_list_box), GTK_WIDGET(row));
            g_free(rest);
            g_free(num);
        }
        g_free(line);
    }
}

static AdwComboRow *r_add_act, *r_add_proto;
static AdwEntryRow *r_add_port, *r_add_from;

static void on_add_rule(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    const char *acts[] = {"allow", "deny", "reject", "limit"};
    const char *act = acts[adw_combo_row_get_selected(r_add_act)];
    
    const char *protos[] = {"tcp", "udp", "any"};
    const char *proto = protos[adw_combo_row_get_selected(r_add_proto)];

    const char *port = gtk_editable_get_text(GTK_EDITABLE(r_add_port));
    const char *from = gtk_editable_get_text(GTK_EDITABLE(r_add_from));

    if (strlen(port) == 0) return;

    GString *cmd = g_string_new("pkexec ufw ");
    g_string_append_printf(cmd, "%s ", act);
    
    if (strlen(from) > 0 && g_strcmp0(from, "any") != 0) {
        g_string_append_printf(cmd, "from %s ", from);
    }
    
    g_string_append_printf(cmd, "to any port %s ", port);
    if (g_strcmp0(proto, "any") != 0) {
        g_string_append_printf(cmd, "proto %s", proto);
    }

    run_pkexec_async(cmd->str);
    g_string_free(cmd, TRUE);
}

static void on_quick_rule(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *cmd = user_data;
    run_pkexec_async(cmd);
}

static gboolean on_auto_refresh(gpointer user_data) {
    (void)user_data;
    refresh_firewall();
    return G_SOURCE_CONTINUE;
}

GtkWidget *page_firewall_new(void) {
    OmarchySystemCaps *caps = omarchy_get_system_caps();
    if (strcmp(caps->firewall_tool, "ufw") != 0) {
        return omarchy_make_unavailable_page("UFW Firewall", "sudo pacman -S ufw", "network-firewall-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Firewall");
    adw_preferences_page_set_icon_name(page, "network-firewall-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    /* ── Status ────────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *st_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(st_group, "Status & Policy");
    adw_preferences_page_add(page, st_group);

    r_enable = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_enable), "Enable Firewall");
    g_signal_connect(r_enable, "notify::active", G_CALLBACK(on_enable_toggled), NULL);
    adw_preferences_group_add(st_group, GTK_WIDGET(r_enable));

    GtkStringList *pol_model = gtk_string_list_new(NULL);
    gtk_string_list_append(pol_model, "Deny");
    gtk_string_list_append(pol_model, "Allow");
    gtk_string_list_append(pol_model, "Reject");

    r_def_in = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_def_in), "Default Incoming Policy");
    adw_combo_row_set_model(r_def_in, G_LIST_MODEL(pol_model));
    adw_combo_row_set_selected(r_def_in, 0); // deny
    g_signal_connect(r_def_in, "notify::selected", G_CALLBACK(on_def_in_changed), NULL);
    adw_preferences_group_add(st_group, GTK_WIDGET(r_def_in));

    r_def_out = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_def_out), "Default Outgoing Policy");
    adw_combo_row_set_model(r_def_out, G_LIST_MODEL(pol_model));
    adw_combo_row_set_selected(r_def_out, 1); // allow
    g_signal_connect(r_def_out, "notify::selected", G_CALLBACK(on_def_out_changed), NULL);
    adw_preferences_group_add(st_group, GTK_WIDGET(r_def_out));

    /* ── Add Rule ──────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *add_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(add_group, "Add Custom Rule");
    adw_preferences_page_add(page, add_group);

    r_add_act = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_add_act), "Action");
    GtkStringList *act_m = gtk_string_list_new(NULL);
    gtk_string_list_append(act_m, "Allow"); gtk_string_list_append(act_m, "Deny");
    gtk_string_list_append(act_m, "Reject"); gtk_string_list_append(act_m, "Limit");
    adw_combo_row_set_model(r_add_act, G_LIST_MODEL(act_m));
    adw_preferences_group_add(add_group, GTK_WIDGET(r_add_act));

    r_add_proto = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_add_proto), "Protocol");
    GtkStringList *proto_m = gtk_string_list_new(NULL);
    gtk_string_list_append(proto_m, "TCP"); gtk_string_list_append(proto_m, "UDP"); gtk_string_list_append(proto_m, "Any");
    adw_combo_row_set_model(r_add_proto, G_LIST_MODEL(proto_m));
    adw_preferences_group_add(add_group, GTK_WIDGET(r_add_proto));

    r_add_port = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_add_port), "Port (e.g. 22 or ssh)");
    adw_preferences_group_add(add_group, GTK_WIDGET(r_add_port));

    r_add_from = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_add_from), "From IP (optional)");
    adw_preferences_group_add(add_group, GTK_WIDGET(r_add_from));

    AdwActionRow *sub_row = ADW_ACTION_ROW(adw_action_row_new());
    GtkWidget *add_btn = gtk_button_new_with_label("Add Rule");
    gtk_widget_add_css_class(add_btn, "suggested-action");
    gtk_widget_set_valign(add_btn, GTK_ALIGN_CENTER);
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_rule), NULL);
    adw_action_row_add_suffix(sub_row, add_btn);
    adw_preferences_group_add(add_group, GTK_WIDGET(sub_row));

    /* ── Quick Rules ───────────────────────────────────────────────────────── */
    AdwPreferencesGroup *q_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(q_group, "Quick Rules");
    adw_preferences_page_add(page, q_group);

    GtkWidget *qb_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(qb_box, 12); gtk_widget_set_margin_end(qb_box, 12);
    gtk_widget_set_margin_top(qb_box, 12); gtk_widget_set_margin_bottom(qb_box, 12);

    struct {const char *lbl; const char *cmd;} quicks[] = {
        {"Allow SSH", "pkexec ufw allow ssh"},
        {"Allow HTTP", "pkexec ufw allow http"},
        {"Allow HTTPS", "pkexec ufw allow https"},
        {"Allow LocalSend", "pkexec ufw allow 53317"}
    };
    for (int i=0; i<4; i++) {
        GtkWidget *b = gtk_button_new_with_label(quicks[i].lbl);
        g_signal_connect_data(b, "clicked", G_CALLBACK(on_quick_rule), g_strdup(quicks[i].cmd), free_g_free, 0);
        gtk_box_append(GTK_BOX(qb_box), b);
    }
    adw_preferences_group_add(q_group, qb_box);

    /* ── Rules List ────────────────────────────────────────────────────────── */
    rules_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(rules_group, "Active Rules");
    
    rules_list_box = gtk_list_box_new();
    gtk_widget_add_css_class(rules_list_box, "boxed-list");
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(rules_list_box), GTK_SELECTION_NONE);
    adw_preferences_group_add(rules_group, rules_list_box);
    adw_preferences_page_add(page, rules_group);

    refresh_firewall();
    g_timeout_add_seconds(30, on_auto_refresh, NULL);

    /* ── CLI Output ────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *log_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(log_group, "CLI Output");
    adw_preferences_page_add(page, log_group);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_size_request(scroll, -1, 150);
    GtkWidget *tv = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(tv), 6);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(tv), 6);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(tv), 6);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(tv), 6);
    log_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), tv);
    adw_preferences_group_add(log_group, scroll);

    return overlay;
}
