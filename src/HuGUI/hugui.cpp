#include "HuGUI/hugui.hpp"
#include "liblvgl/core/lv_obj.h"
#include "liblvgl/core/lv_obj_event.h"
#include "liblvgl/core/lv_obj_tree.h"
#include "liblvgl/display/lv_display.h"
#include "liblvgl/misc/lv_area.h"
#include "liblvgl/misc/lv_event.h"
#include "liblvgl/widgets/button/lv_button.h"
#include "liblvgl/widgets/slider/lv_slider.h"
#include "liblvgl/widgets/tabview/lv_tabview.h"
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include "api.h"
#include "HuGUI/gifclass.hpp"

namespace hugui {
// Global variables
lv_obj_t* posLabel;
lv_obj_t* selectedAutonLabel;

lv_obj_t* homeScreen;
lv_obj_t* tabview;
lv_obj_t* tabScreen;

lv_obj_t* startButton;
lv_obj_t* noAutonButton;

lv_obj_t* redAUTONS;
lv_obj_t* blueAUTONS;
lv_obj_t* otherAUTONS;
lv_obj_t* config;
lv_obj_t* console;

lv_obj_t* bar;

std::vector<lv_obj_t*> consoleText(10);

int selected_auton = 0;
int numRed = 0;
int numBlue = 0;
int numToggle = 0;
int numOther = 0;
std::vector<std::pair<std::pair<std::string, AutonType>, std::function<void()>>> auton_list;
std::unordered_map<lv_obj_t*, int> auton_map;

static lv_style_t optionsButtonStyle;
static lv_style_t redAutonButtonStyle;
static lv_style_t blueAutonButtonStyle;
static lv_style_t toggleConfigButtonStyleTrue;
static lv_style_t toggleConfigButtonStyleFalse;

lv_obj_t* create_button(lv_obj_t* parent, lv_coord_t x, 
                        lv_coord_t y, lv_coord_t width, 
                        lv_coord_t height, int id, 
                        const char* text) {
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_set_size(btn, width, height);
    lv_obj_set_pos(btn, x, y);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);


    return btn;
}

void set_button_pos(lv_obj_t* btn, int compare) {
    lv_obj_set_pos(btn, compare % 4 * 100, (compare / 4) * 60);
}

void change_selected_auton(int index) {
    selected_auton = index;

    if(!pros::usd::is_installed()) {
        return;
    }

    FILE* file = fopen("/usd/auton.txt", "w");
    if (file) {
        fputs(std::to_string(selected_auton).c_str(), file);
        fclose(file);
    }
}

void update_auton_string(int index) {
    if(index >= auton_list.size() || index < 0) {
        index = 0;
        selected_auton = 0;
    }
    std::string display = auton_list[index].first.first;
    auto alliance = auton_list[index].first.second;
    std::string allianceString = "";
    
    lv_async_call([](void* user_data) {
        int idx = selected_auton;
        if(idx >= auton_list.size() || idx < 0) {
            idx = 0;
        }
        auto alliance = auton_list[idx].first.second;
        std::string display = auton_list[idx].first.first;
        std::string allianceString = "";
        if(alliance == RED) {
            allianceString = " (R)";
            lv_obj_add_style(selectedAutonLabel, &redAutonButtonStyle, 0);
        } else if(alliance == BLUE) {
            allianceString = " (B)";
            lv_obj_add_style(selectedAutonLabel, &blueAutonButtonStyle, 0);
        }
        lv_label_set_text(selectedAutonLabel, ("Auton: " + display + allianceString).c_str());
    }, &index);
}

void change_auton_event(lv_event_t* e) {
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));
    change_selected_auton(auton_map[btn]);
    update_auton_string(auton_map[btn]);
}

void toggle_config_event(lv_event_t* e) {
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));
    bool* toggleVariable = (bool*)lv_event_get_user_data(e);
    *toggleVariable = !(*toggleVariable);
    
    lv_async_call([](void* user_data) {
        auto* data = static_cast<std::pair<lv_obj_t*, bool>*>(user_data);
        lv_obj_t* btn = data->first;
        bool toggleValue = data->second;
        lv_obj_remove_style(btn, (toggleValue ? &toggleConfigButtonStyleFalse : &toggleConfigButtonStyleTrue), 0);
        lv_obj_add_style(btn, (toggleValue ? &toggleConfigButtonStyleTrue : &toggleConfigButtonStyleFalse), 0);
    }, new std::pair<lv_obj_t*, bool>(btn, *toggleVariable));
}

void GUI::show_home_screen(lv_event_t* e) {
    lv_async_call([](void* user_data) {
        lv_obj_set_parent(bar, homeScreen);
        lv_obj_remove_event_cb(startButton, show_home_screen);
        lv_obj_add_event_cb(startButton, show_tab_view, LV_EVENT_CLICKED, NULL);
        lv_screen_load(homeScreen);
    }, NULL);
}
void GUI::show_tab_view(lv_event_t* e) {
    lv_async_call([](void* user_data) {
        lv_obj_set_parent(bar, tabview);
        lv_obj_remove_event_cb(startButton, show_tab_view);
        lv_obj_add_event_cb(startButton, show_home_screen, LV_EVENT_CLICKED, NULL);
        lv_screen_load(tabScreen);
    }, NULL);
}

void GUI::create_tab_view() {
    tabScreen = lv_obj_create(NULL);
    tabview = lv_tabview_create(tabScreen);
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(tabview, 50);

    redAUTONS = lv_tabview_add_tab(tabview, "RED");
    blueAUTONS = lv_tabview_add_tab(tabview, "BLUE");
    otherAUTONS = lv_tabview_add_tab(tabview, "OTHER");
    config = lv_tabview_add_tab(tabview, "CONFIG");
    console = lv_tabview_add_tab(tabview, "CONSOLE");
}

void GUI::create_home_screen(bool addLogo) {
    homeScreen = lv_obj_create(NULL);

    lv_obj_t* logo = lv_obj_create(homeScreen);
    lv_obj_set_size(logo, 470, 200);
    lv_obj_set_style_bg_color(logo, lv_color_make(0, 0, 0), 0);
    lv_obj_align(logo, LV_ALIGN_CENTER, 0, -20);

    if(pros::usd::is_installed() && addLogo) {
        static gif::Gif gif("/usd/logo.gif", logo);
    }

    bar = lv_obj_create(homeScreen);

    lv_obj_set_size(bar, 480, 40);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);   

    startButton = create_button(bar, 0, 0, 70, 25, 1, "");
    lv_obj_set_align(startButton, LV_ALIGN_CENTER);
    lv_obj_set_pos(startButton, 0, 0);
    lv_obj_add_style(startButton, &optionsButtonStyle, 0);
    lv_obj_add_event_cb(startButton, show_tab_view, LV_EVENT_CLICKED, NULL);
    posLabel = lv_label_create(bar);
    lv_label_set_text(posLabel, "0, 0, 0"); 
    lv_obj_align(posLabel, LV_ALIGN_RIGHT_MID, -10, 0);

    selectedAutonLabel = lv_label_create(bar);

    update_auton_string(selected_auton);

    lv_obj_align(selectedAutonLabel, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_scrollbar_mode(bar, LV_SCROLLBAR_MODE_OFF);
}

void GUI::load_selected_auton() {
    if(!pros::usd::is_installed()) {
        return;
    }
    FILE* file = fopen("/usd/auton.txt", "r");
    if (file) {
        char buffer[10];
        if (fgets(buffer, sizeof(buffer), file)) {
            selected_auton = std::stoi(buffer);
        } else {
            selected_auton = 0; // Default if reading fails
        }
        fclose(file);
    } else {
        selected_auton = 0; // Default if file does not exist
    }
}

void GUI::add_auton(std::function<void()> auton, std::string auton_name, AutonType alliance) {
    auton_list.push_back(std::make_pair(std::make_pair(auton_name, alliance), auton));
    lv_obj_t* btn = create_button(tabview, 0, 40, 90, 50, 1, auton_name.c_str());
    lv_obj_add_event_cb(btn, change_auton_event, LV_EVENT_CLICKED, NULL);
    auton_map[btn] = auton_list.size() - 1;
    switch (alliance) {
        case RED:
            lv_obj_set_parent(btn, redAUTONS);
            set_button_pos(btn, numRed);
            numRed++;
            lv_obj_add_style(btn, &redAutonButtonStyle, 0);
            break;
        case BLUE:
            lv_obj_set_parent(btn, blueAUTONS);
            set_button_pos(btn, numBlue);
            numBlue++;
            lv_obj_add_style(btn, &blueAutonButtonStyle, 0);
            break;
        case OTHER:
            lv_obj_set_parent(btn, otherAUTONS);
            set_button_pos(btn, numOther);
            numOther++;
            lv_obj_add_style(btn, &optionsButtonStyle, 0);
            break;
    }
}

void GUI::add_config_toggle(bool* toggleVariable, std::string name) {
    lv_obj_t* btn = create_button(config, 0, 0, 90, 50, 1, name.c_str());
    lv_obj_add_event_cb(btn, toggle_config_event, LV_EVENT_CLICKED, toggleVariable);
    set_button_pos(btn, numToggle);
    numToggle++;
    if (*toggleVariable) {
        lv_obj_add_style(btn, &toggleConfigButtonStyleTrue, 0);
    } else {
        lv_obj_add_style(btn, &toggleConfigButtonStyleFalse, 0);
    }
}

void GUI::add_config_slider(double* sliderVariable, std::string name, int min, int max, int step) {
    lv_obj_t* slider = lv_slider_create(config);
    lv_obj_set_size(slider, 200, 20);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, *sliderVariable, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, [](lv_event_t* e) {
        double* sliderVariable = (double*)lv_event_get_user_data(e);
        *sliderVariable = lv_slider_get_value(static_cast<lv_obj_t*>(lv_event_get_target(e)));
    }, LV_EVENT_VALUE_CHANGED, sliderVariable);
}

void GUI::console_print(std::string text, int line) {
    struct ConsoleData {
        std::string text;
        int line;
    };
    
    ConsoleData* data = new ConsoleData{text, line};
    
    lv_async_call([](void* user_data) {
        ConsoleData* console_data = static_cast<ConsoleData*>(user_data);
        
        if (consoleText[console_data->line] != NULL) {
            lv_obj_delete(consoleText[console_data->line]);
        }
        consoleText[console_data->line] = lv_label_create(console);
        lv_label_set_text(consoleText[console_data->line], console_data->text.c_str());
        lv_obj_align(consoleText[console_data->line], LV_ALIGN_TOP_LEFT, 0, (20 * console_data->line) - 20);
        
        delete console_data;
    }, data);
}

void GUI::update_pos(double x, double y, double theta, int precision) {
    static double pos_x, pos_y, pos_theta;
    static int pos_precision;
    pos_x = x;
    pos_y = y;
    pos_theta = theta;
    pos_precision = precision;
    
    lv_async_call([](void* user_data) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(pos_precision) << pos_x << ", " << pos_y << ", " << pos_theta;
        std::string pos = ss.str();
        lv_label_set_text(posLabel, pos.c_str());
    }, NULL);
}

void GUI::run_selected_auton() {
    if(selected_auton == 0) {
        return;
    }
    auton_list[selected_auton].second();
}

void GUI::initialize_auton_selector(std::vector<Auton> autons, bool addLogo) {
    load_selected_auton();

    lv_style_init(&optionsButtonStyle);
    lv_style_set_bg_color(&optionsButtonStyle, lv_color_make(255, 255, 255));
    lv_style_set_text_color(&optionsButtonStyle, lv_color_make(0, 0, 0));

    lv_style_init(&redAutonButtonStyle);
    lv_style_set_bg_color(&redAutonButtonStyle, lv_color_make(178, 22, 83));

    lv_style_init(&blueAutonButtonStyle);
    lv_style_set_bg_color(&blueAutonButtonStyle, lv_color_make(104, 175, 240));

    lv_style_init(&toggleConfigButtonStyleTrue);
    lv_style_set_bg_color(&toggleConfigButtonStyleTrue, lv_color_make(0, 255, 0));

    lv_style_init(&toggleConfigButtonStyleFalse);
    lv_style_set_bg_color(&toggleConfigButtonStyleFalse, lv_color_make(255, 0, 0));

    create_tab_view();

    add_auton([]() {}, "NONE", OTHER);

    for(auto auton : autons) {
        add_auton(auton.auton, auton.auton_name, auton.alliance);
    }
    create_home_screen(addLogo);
    lv_screen_load(homeScreen);
}

}
