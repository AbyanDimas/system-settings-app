#include "page-ollama.h"
#include "../utils/subprocess.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static AdwPreferencesGroup *models_group = NULL;
static GtkWidget *models_list_box = NULL;
static AdwSwitchRow *r_enable = NULL;
static AdwEntryRow *r_pull = NULL;

static void refresh_ollama(void);

static void run_pkexec_async(const char *cmd) {
    g_autoptr(GError) err = NULL;
    const char *argv[] = {"sh", "-c", cmd, NULL};
    GSubprocess *proc = g_subprocess_newv(argv, G_SUBPROCESS_FLAGS_NONE, &err);
    if (proc) {
        g_subprocess_wait_async(proc, NULL, NULL, NULL);
    }
}

static void on_enable_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    if (adw_switch_row_get_active(ADW_SWITCH_ROW(obj))) {
        run_pkexec_async("pkexec systemctl enable --now ollama");
    } else {
        run_pkexec_async("pkexec systemctl disable --now ollama");
    }
}

static void on_model_action(GtkButton *btn, gpointer user_data) {
    (void)btn;
    char **args = user_data; // [action, model]
    
    if (strcmp(args[0], "run") == 0) {
        g_autofree char *cmd = g_strdup_printf("alacritty -t 'Ollama Chat: %s' -e ollama run %s", args[1], args[1]);
        subprocess_run_async(cmd, NULL, NULL);
    } else if (strcmp(args[0], "rm") == 0) {
        g_autofree char *cmd = g_strdup_printf("alacritty -t 'Deleting Model' -e bash -c 'ollama rm %s; sleep 1'", args[1]);
        subprocess_run_async(cmd, NULL, NULL);
    }
}

static void free_model_args(gpointer data, GClosure *closure) {
    (void)closure;
    g_strfreev((char**)data);
}

static void on_pull_model(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    const char *model = gtk_editable_get_text(GTK_EDITABLE(r_pull));
    if (strlen(model) > 0) {
        g_autofree char *cmd = g_strdup_printf("alacritty -t 'Pulling Model: %s' -e bash -c 'ollama pull %s; read -p \"Done. Press Enter...\"'", model, model);
        subprocess_run_async(cmd, NULL, NULL);
        gtk_editable_set_text(GTK_EDITABLE(r_pull), "");
    }
}

static void on_restart_daemon(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    run_pkexec_async("pkexec systemctl restart ollama");
    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Restarted Ollama Daemon (Freed VRAM)"));
    }
}

static void refresh_ollama(void) {
    if (!models_list_box) return;

    gtk_list_box_remove_all(GTK_LIST_BOX(models_list_box));

    g_autoptr(GError) err = NULL;
    
    g_autofree char *sys_out = subprocess_run_sync("systemctl is-active ollama", &err);
    gboolean is_active = (sys_out && strstr(sys_out, "active") && !strstr(sys_out, "inactive"));
    
    g_signal_handlers_block_by_func(r_enable, on_enable_toggled, NULL);
    adw_switch_row_set_active(r_enable, is_active);
    g_signal_handlers_unblock_by_func(r_enable, on_enable_toggled, NULL);

    if (!is_active) {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "Ollama daemon is offline.");
        gtk_list_box_append(GTK_LIST_BOX(models_list_box), GTK_WIDGET(row));
        return;
    }

    g_autofree char *ls_out = subprocess_run_sync("ollama list", &err);
    if (ls_out) {
        g_auto(GStrv) lines = g_strsplit(ls_out, "\n", -1);
        int c = 0;
        for (int i = 1; lines[i] != NULL; i++) { // skip header
            char *line = g_strstrip(g_strdup(lines[i]));
            if (strlen(line) == 0) { g_free(line); continue; }
            
            // Format: NAME               ID              SIZE      MODIFIED
            // Space delimited
            g_auto(GStrv) toks = g_strsplit_set(line, " \t", -1);
            GPtrArray *real = g_ptr_array_new();
            for(int j=0; toks[j]; j++) {
                if (strlen(toks[j])>0) g_ptr_array_add(real, toks[j]);
            }

            if (real->len >= 3) {
                const char *name = g_ptr_array_index(real, 0);
                const char *id = g_ptr_array_index(real, 1);
                const char *sz1 = g_ptr_array_index(real, 2);
                const char *sz2 = (real->len > 3) ? g_ptr_array_index(real, 3) : "";

                AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), name);
                adw_action_row_set_subtitle(row, g_strdup_printf("ID: %s  ·  Size: %s %s", id, sz1, sz2));

                GtkWidget *bb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
                
                GtkWidget *br = gtk_button_new_with_label("Run Model");
                gtk_widget_add_css_class(br, "suggested-action");
                gtk_widget_set_valign(br, GTK_ALIGN_CENTER);
                char **args_r = g_new0(char*, 3);
                args_r[0] = g_strdup("run");
                args_r[1] = g_strdup(name);
                g_signal_connect_data(br, "clicked", G_CALLBACK(on_model_action), args_r, free_model_args, 0);
                gtk_box_append(GTK_BOX(bb), br);

                GtkWidget *bd = gtk_button_new_from_icon_name("user-trash-symbolic");
                gtk_widget_add_css_class(bd, "destructive-action");
                gtk_widget_set_valign(bd, GTK_ALIGN_CENTER);
                char **args_d = g_new0(char*, 3);
                args_d[0] = g_strdup("rm");
                args_d[1] = g_strdup(name);
                g_signal_connect_data(bd, "clicked", G_CALLBACK(on_model_action), args_d, free_model_args, 0);
                gtk_box_append(GTK_BOX(bb), bd);

                adw_action_row_add_suffix(row, bb);
                gtk_list_box_append(GTK_LIST_BOX(models_list_box), GTK_WIDGET(row));
                c++;
            }
            g_ptr_array_unref(real);
            g_free(line);
        }
        if (c == 0) {
            AdwActionRow *r2 = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r2), "No language models downloaded.");
            gtk_list_box_append(GTK_LIST_BOX(models_list_box), GTK_WIDGET(r2));
        }
    }
}

GtkWidget *page_ollama_new(void) {
    if (!g_find_program_in_path("ollama")) {
        return omarchy_make_unavailable_page("Local AI", "curl -fsSL https://ollama.com/install.sh | sh", "network-server-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Local LLM AI");
    adw_preferences_page_set_icon_name(page, "network-server-symbolic"); // Fallback
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    /* ── Daemon Status ─────────────────────────────────────────────────────── */
    AdwPreferencesGroup *st_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(st_group, "Ollama Engine");
    adw_preferences_page_add(page, st_group);

    r_enable = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_enable), "Start Inference Server");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(r_enable), "Enable the ollama.service on localhost:11434");
    g_signal_connect(r_enable, "notify::active", G_CALLBACK(on_enable_toggled), NULL);
    adw_preferences_group_add(st_group, GTK_WIDGET(r_enable));

    AdwActionRow *ac_res = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ac_res), "Free Graphics Memory");
    adw_action_row_set_subtitle(ac_res, "Interrupt any currently generating model block and evict it from VRAM");
    GtkWidget *b_res = gtk_button_new_with_label("Stop Active Model");
    gtk_widget_add_css_class(b_res, "destructive-action");
    gtk_widget_set_valign(b_res, GTK_ALIGN_CENTER);
    g_signal_connect(b_res, "clicked", G_CALLBACK(on_restart_daemon), NULL);
    adw_action_row_add_suffix(ac_res, b_res);
    adw_preferences_group_add(st_group, GTK_WIDGET(ac_res));

    /* ── Pull Model ────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *pull_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(pull_group, "Find Models");
    adw_preferences_page_add(page, pull_group);

    r_pull = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_pull), "Model Tag (e.g. llama3, phi3)");
    
    GtkWidget *b_pull = gtk_button_new_with_label("Download");
    gtk_widget_add_css_class(b_pull, "suggested-action");
    gtk_widget_set_valign(b_pull, GTK_ALIGN_CENTER);
    g_signal_connect(b_pull, "clicked", G_CALLBACK(on_pull_model), NULL);
    adw_entry_row_add_suffix(r_pull, b_pull);
    adw_preferences_group_add(pull_group, GTK_WIDGET(r_pull));

    /* ── Local Models ──────────────────────────────────────────────────────── */
    models_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(models_group, "Cached Weights");
    
    models_list_box = gtk_list_box_new();
    gtk_widget_add_css_class(models_list_box, "boxed-list");
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(models_list_box), GTK_SELECTION_NONE);
    adw_preferences_group_add(models_group, models_list_box);

    GtkWidget *top_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *ref_b = gtk_button_new_from_icon_name("view-refresh-symbolic");
    g_signal_connect(ref_b, "clicked", G_CALLBACK(refresh_ollama), NULL);
    gtk_box_append(GTK_BOX(top_box), ref_b);
    adw_preferences_group_set_header_suffix(models_group, top_box);
    adw_preferences_page_add(page, models_group);

    refresh_ollama();

    return overlay;
}
