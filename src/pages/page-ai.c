#include "page-ai.h"
#include "../utils/subprocess.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static AdwPasswordEntryRow *r_openai = NULL;
static AdwPasswordEntryRow *r_anthropic = NULL;
static AdwPasswordEntryRow *r_gemini = NULL;

static char *get_bashrc_val(const char *key) {
    g_autofree char *path = g_build_filename(g_get_home_dir(), ".bashrc", NULL);
    g_autofree char *contents = NULL;
    if (g_file_get_contents(path, &contents, NULL, NULL)) {
        g_auto(GStrv) lines = g_strsplit(contents, "\n", -1);
        for (int i = 0; lines[i] != NULL; i++) {
            char *line = g_strstrip(g_strdup(lines[i]));
            if (g_str_has_prefix(line, "export ") && strstr(line, key)) {
                char *eq = strchr(line, '=');
                if (eq) {
                    char *val = g_strdup(eq + 1);
                    // Strip quotes if they exist
                    if (val[0] == '"' || val[0] == '\'') {
                        val[strlen(val)-1] = '\0';
                        char *v2 = g_strdup(val + 1);
                        g_free(line);
                        g_free(val);
                        return v2;
                    }
                    g_free(line);
                    return val;
                }
            }
            g_free(line);
        }
    }
    return NULL;
}

static void on_save_keys(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    
    const char *keys[] = {"OPENAI_API_KEY", "ANTHROPIC_API_KEY", "GEMINI_API_KEY"};
    GtkWidget *rows[] = {GTK_WIDGET(r_openai), GTK_WIDGET(r_anthropic), GTK_WIDGET(r_gemini)};
    
    g_autofree char *path = g_build_filename(g_get_home_dir(), ".bashrc", NULL);

    for (int i=0; i<3; i++) {
        const char *val = gtk_editable_get_text(GTK_EDITABLE(rows[i]));
        if (strlen(val) > 0) {
            // Remove old line
            g_autofree char *escaped = g_regex_escape_string(keys[i], -1);
            g_autofree char *cmd_sed = g_strdup_printf("sed -i '/^export %s=/d' \"%s\"", escaped, path);
            g_autoptr(GError) err1 = NULL;
            subprocess_run_sync(cmd_sed, &err1);
            
            // Append new line
            g_autofree char *cmd_echo = g_strdup_printf("echo 'export %s=\"%s\"' >> \"%s\"", keys[i], val, path);
            g_autoptr(GError) err2 = NULL;
            subprocess_run_sync(cmd_echo, &err2);
        }
    }

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("API Keys saved to ~/.bashrc"));
    }
}

GtkWidget *page_ai_new(void) {
    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Cloud AI & Services");
    adw_preferences_page_set_icon_name(page, "network-server-symbolic"); // Fallback
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    /* ── Keys ──────────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *key_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(key_group, "Global API Tokens");
    adw_preferences_group_set_description(key_group, "Inject these environment variables into your shell globally, making them available to tools like aider, gptme, and web apps.");
    adw_preferences_page_add(page, key_group);

    r_openai = ADW_PASSWORD_ENTRY_ROW(adw_password_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_openai), "OpenAI API Key (sk-...)");
    g_autofree char *v_openai = get_bashrc_val("OPENAI_API_KEY");
    if (v_openai) gtk_editable_set_text(GTK_EDITABLE(r_openai), v_openai);
    adw_preferences_group_add(key_group, GTK_WIDGET(r_openai));

    r_anthropic = ADW_PASSWORD_ENTRY_ROW(adw_password_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_anthropic), "Anthropic API Key");
    g_autofree char *v_anth = get_bashrc_val("ANTHROPIC_API_KEY");
    if (v_anth) gtk_editable_set_text(GTK_EDITABLE(r_anthropic), v_anth);
    adw_preferences_group_add(key_group, GTK_WIDGET(r_anthropic));

    r_gemini = ADW_PASSWORD_ENTRY_ROW(adw_password_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_gemini), "Google Gemini API Key");
    g_autofree char *v_gem = get_bashrc_val("GEMINI_API_KEY");
    if (v_gem) gtk_editable_set_text(GTK_EDITABLE(r_gemini), v_gem);
    adw_preferences_group_add(key_group, GTK_WIDGET(r_gemini));

    AdwActionRow *ac_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ac_row), "Apply Configuration");
    
    GtkWidget *b_sv = gtk_button_new_with_label("Save to ~/.bashrc");
    gtk_widget_add_css_class(b_sv, "suggested-action");
    gtk_widget_set_valign(b_sv, GTK_ALIGN_CENTER);
    g_signal_connect(b_sv, "clicked", G_CALLBACK(on_save_keys), NULL);
    adw_action_row_add_suffix(ac_row, b_sv);
    adw_preferences_group_add(key_group, GTK_WIDGET(ac_row));

    return overlay;
}
