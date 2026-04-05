#pragma once
#include <adwaita.h>
#include "app.h"

#define OMARCHY_TYPE_SETTINGS_WINDOW (omarchy_settings_window_get_type())
G_DECLARE_FINAL_TYPE(OmarchySettingsWindow, omarchy_settings_window, OMARCHY, SETTINGS_WINDOW, AdwApplicationWindow)

OmarchySettingsWindow *omarchy_settings_window_new(OmarchySettingsApp *app);

GtkWidget *omarchy_make_unavailable_page(
  const char *tool_name,
  const char *install_cmd,
  const char *icon_name
);
