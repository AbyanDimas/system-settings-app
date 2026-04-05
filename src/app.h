#pragma once
#include <adwaita.h>

#define OMARCHY_TYPE_SETTINGS_APP (omarchy_settings_app_get_type())
G_DECLARE_FINAL_TYPE(OmarchySettingsApp, omarchy_settings_app, OMARCHY, SETTINGS_APP, AdwApplication)

OmarchySettingsApp *omarchy_settings_app_new(void);
