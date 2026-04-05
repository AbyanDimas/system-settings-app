#include "page-fonts.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;

static AdwEntryRow *r_sys_font;
static AdwSpinRow *r_sys_val;
static AdwEntryRow *r_mono_font;
static AdwSpinRow *r_mono_val;

static char* get_gsettings(const char *key) {
    g_autofree char *cmd = g_strdup_printf("gsettings get org.gnome.desktop.interface %s 2>/dev/null", key);
    g_autoptr(GError) err = NULL;
    char *out = subprocess_run_sync(cmd, &err);
    if (!out) return g_strdup("");
    g_strstrip(out);
    if (out[0] == '\'') {
        char *end = strrchr(out, '\'');
        if (end && end != out) {
            *end = '\0';
            char *clean = g_strdup(out + 1);
            g_free(out);
            return clean;
        }
    }
    return out;
}

static void set_gsettings(const char *key, const char *val, double size) {
    g_autofree char *cmd = g_strdup_printf("gsettings set org.gnome.desktop.interface %s '%s %g'", key, val, size);
    g_autoptr(GError) err = NULL;
    subprocess_run_sync(cmd, &err);
}

static void parse_font_str(const char *raw, char **name, double *size) {
    if (!raw || strlen(raw) == 0) {
        *name = g_strdup("");
        *size = 11.0;
        return;
    }
    // "Cantarell 11"
    char *last_space = strrchr(raw, ' ');
    if (last_space) {
        *size = atof(last_space + 1);
        *name = g_strndup(raw, last_space - raw);
    } else {
        *name = g_strdup(raw);
        *size = 11.0;
    }
}

static void on_save_fonts(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;

    const char *s_fam = gtk_editable_get_text(GTK_EDITABLE(r_sys_font));
    double s_sz = adw_spin_row_get_value(r_sys_val);
    const char *m_fam = gtk_editable_get_text(GTK_EDITABLE(r_mono_font));
    double m_sz = adw_spin_row_get_value(r_mono_val);

    set_gsettings("font-name", s_fam, s_sz);
    set_gsettings("document-font-name", s_fam, s_sz);
    set_gsettings("monospace-font-name", m_fam, m_sz);

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Fonts updated (GTK/GNOME schemas)"));
    }
}

static void on_font_picked(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    if (!row) return;
    AdwEntryRow *target = ADW_ENTRY_ROW(g_object_get_data(G_OBJECT(user_data), "target_row"));
    GtkWidget *label = gtk_list_box_row_get_child(row);
    if (target && label) {
        gtk_editable_set_text(GTK_EDITABLE(target), gtk_label_get_text(GTK_LABEL(label)));
    }
    AdwDialog *dlg = ADW_DIALOG(user_data);
    adw_dialog_close(dlg);
}

static void show_font_picker(AdwEntryRow *target, gboolean monospace_only) {
    AdwDialog *dlg = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_content_width(dlg, 400);
    adw_dialog_set_content_height(dlg, 500);
    adw_dialog_set_title(dlg, monospace_only ? "Select Monospace Font" : "Select Font");
    g_object_set_data(G_OBJECT(dlg), "target_row", target);

    GtkWidget *scroll = gtk_scrolled_window_new();
    GtkWidget *list = gtk_list_box_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list);

    g_autoptr(GError) err = NULL;
    const char *cmd = monospace_only ? "fc-list :spacing=100 family | cut -d',' -f1 | sort -u" : "fc-list :family | cut -d',' -f1 | sort -u";
    g_autofree char *f_out = subprocess_run_sync(cmd, &err);
    if (f_out) {
        g_auto(GStrv) fonts = g_strsplit(f_out, "\n", -1);
        for (int i = 0; fonts[i] != NULL; i++) {
            if (strlen(fonts[i]) > 0) {
                GtkWidget *lbl = gtk_label_new(fonts[i]);
                gtk_widget_set_halign(lbl, GTK_ALIGN_START);
                gtk_widget_set_margin_start(lbl, 16); gtk_widget_set_margin_end(lbl, 16);
                gtk_widget_set_margin_top(lbl, 8); gtk_widget_set_margin_bottom(lbl, 8);
                gtk_list_box_append(GTK_LIST_BOX(list), lbl);
            }
        }
    }

    g_signal_connect(list, "row-activated", G_CALLBACK(on_font_picked), dlg);
    adw_dialog_set_child(dlg, scroll);
    adw_dialog_present(dlg, page_widget);
}

static void on_browse_sys(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    show_font_picker(r_sys_font, FALSE);
}

static void on_browse_mono(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    show_font_picker(r_mono_font, TRUE);
}

GtkWidget *page_fonts_new(void) {
    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Fonts");
    adw_preferences_page_set_icon_name(page, "preferences-desktop-font-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    g_autofree char *raw_sys = get_gsettings("font-name");
    g_autofree char *raw_mono = get_gsettings("monospace-font-name");
    
    char *sys_f = NULL; double sys_sz = 11.0;
    char *mono_f = NULL; double mono_sz = 11.0;
    parse_font_str(raw_sys, &sys_f, &sys_sz);
    parse_font_str(raw_mono, &mono_f, &mono_sz);

    /* ── Controls ──────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *ctrl_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_page_add(page, ctrl_group);

    AdwActionRow *save_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(save_row), "Apply Changes");
    GtkWidget *save_btn = gtk_button_new_with_label("Save System Fonts");
    gtk_widget_add_css_class(save_btn, "suggested-action");
    gtk_widget_set_valign(save_btn, GTK_ALIGN_CENTER);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_fonts), NULL);
    adw_action_row_add_suffix(save_row, save_btn);
    adw_preferences_group_add(ctrl_group, GTK_WIDGET(save_row));

    /* ── Interface Fonts ───────────────────────────────────────────────────── */
    AdwPreferencesGroup *if_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(if_group, "Interface Fonts");
    adw_preferences_page_add(page, if_group);

    r_sys_font = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_sys_font), "System Font");
    gtk_editable_set_text(GTK_EDITABLE(r_sys_font), sys_f ? sys_f : "");
    GtkWidget *bs1 = gtk_button_new_with_label("Browse");
    gtk_widget_set_valign(bs1, GTK_ALIGN_CENTER);
    g_signal_connect(bs1, "clicked", G_CALLBACK(on_browse_sys), NULL);
    adw_entry_row_add_suffix(r_sys_font, bs1);
    adw_preferences_group_add(if_group, GTK_WIDGET(r_sys_font));

    r_sys_val = ADW_SPIN_ROW(adw_spin_row_new_with_range(6.0, 36.0, 0.5));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_sys_val), "Size");
    adw_spin_row_set_value(r_sys_val, sys_sz);
    adw_preferences_group_add(if_group, GTK_WIDGET(r_sys_val));

    r_mono_font = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_mono_font), "Monospace Font");
    gtk_editable_set_text(GTK_EDITABLE(r_mono_font), mono_f ? mono_f : "");
    GtkWidget *bs2 = gtk_button_new_with_label("Browse");
    gtk_widget_set_valign(bs2, GTK_ALIGN_CENTER);
    g_signal_connect(bs2, "clicked", G_CALLBACK(on_browse_mono), NULL);
    adw_entry_row_add_suffix(r_mono_font, bs2);
    adw_preferences_group_add(if_group, GTK_WIDGET(r_mono_font));

    r_mono_val = ADW_SPIN_ROW(adw_spin_row_new_with_range(6.0, 36.0, 0.5));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_mono_val), "Size");
    adw_spin_row_set_value(r_mono_val, mono_sz);
    adw_preferences_group_add(if_group, GTK_WIDGET(r_mono_val));

    /* ── Nerd Fonts ────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *nf_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(nf_group, "Nerd Fonts Validation");
    adw_preferences_page_add(page, nf_group);

    g_autoptr(GError) ne = NULL;
    g_autofree char *nf_out = subprocess_run_sync("fc-list | grep -i 'nerd' | head -1", &ne);
    gboolean has_nf = (nf_out && strlen(nf_out) > 5);

    AdwActionRow *nf_row = ADW_ACTION_ROW(adw_action_row_new());
    if (has_nf) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(nf_row), "Nerd Fonts installed: Yes");
        adw_action_row_set_subtitle(nf_row, "System icons should display correctly.");
        AdwStatusPage *ic = ADW_STATUS_PAGE(adw_status_page_new()); // cheat just for icon
        adw_status_page_set_icon_name(ic, "emblem-ok-symbolic");
        gtk_widget_set_size_request(GTK_WIDGET(ic), 32, 32);
        adw_action_row_add_prefix(nf_row, GTK_WIDGET(ic));
    } else {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(nf_row), "Warning: Icons may be missing.");
        adw_action_row_set_subtitle(nf_row, "Install a Nerd Font (e.g. ttf-jetbrains-mono-nerd)");
        AdwStatusPage *ic = ADW_STATUS_PAGE(adw_status_page_new());
        adw_status_page_set_icon_name(ic, "dialog-warning-symbolic");
        gtk_widget_set_size_request(GTK_WIDGET(ic), 32, 32);
        adw_action_row_add_prefix(nf_row, GTK_WIDGET(ic));
    }
    adw_preferences_group_add(nf_group, GTK_WIDGET(nf_row));

    g_free(sys_f);
    g_free(mono_f);

    return overlay;
}
