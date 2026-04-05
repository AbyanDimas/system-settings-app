#include <gtk/gtk.h>
#include <adwaita.h>
#include <stdio.h>

#include "../src/pages/page-hotspot.h"
#include "../src/pages/page-notifications.h"
#include "../src/pages/page-appearance.h"
#include "../src/pages/page-waybar.h"

static void test_page_hotspot(void) {
    GtkWidget *page = page_hotspot_new();
    if (page != NULL) {
        printf("test_page_hotspot: PASS\n");
    } else {
        printf("test_page_hotspot: FAIL\n");
    }
}

static void test_page_notifications(void) {
    GtkWidget *page = page_notifications_new();
    if (page != NULL) {
        printf("test_page_notifications: PASS\n");
    } else {
        printf("test_page_notifications: FAIL\n");
    }
}

static void test_page_appearance(void) {
    GtkWidget *page = page_appearance_new();
    if (page != NULL) {
        printf("test_page_appearance: PASS\n");
    } else {
        printf("test_page_appearance: FAIL\n");
    }
}

static void test_page_waybar(void) {
    GtkWidget *page = page_waybar_new();
    if (page != NULL) {
        printf("test_page_waybar: PASS\n");
    } else {
        printf("test_page_waybar: FAIL\n");
    }
}

int main(int argc, char *argv[]) {
    gtk_init();
    adw_init();
    
    printf("Running Omarchy Settings Tests...\n");
    test_page_hotspot();
    test_page_notifications();
    test_page_appearance();
    test_page_waybar();
    
    return 0;
}
