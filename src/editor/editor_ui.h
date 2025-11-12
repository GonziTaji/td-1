#pragma once

#include <raylib.h>

#define EDITOR_UI_MAX_CHARS_TEXT_NODE 128
#define EDITOR_UI_TOOLBAR_MAX_BUTTONS 8
#define EDITOR_UI_COMBOBOX_MAX_OPTIONS 128

typedef struct {
    int padding, gap;

    enum {
        LAYOUT_DIR_COL,
        LAYOUT_DIR_ROW,
    } direction;
} UILayout;

typedef enum {
    INPUT_TYPE_TEXT,
    INPUT_TYPE_INT,
    INPUT_TYPE_FLOAT,
} InputType;

typedef bool IsActive;

typedef struct {
    char *label;
    char **options;
    int options_count;
    int *selected_index;
    int font_size;
} UIComboBoxProps;

typedef struct {
    bool activated;
    bool selection_changed;
    bool is_open;
} UIComboBoxResult;

typedef struct {
    Vector2 origin;
    Vector2 master_origin;
    Vector2 detail_origin;
    float gap;
    bool master_started;
    bool detail_started;
    UILayout master_layout;
    UILayout detail_layout;
    Rectangle master_bounds;
    Rectangle detail_bounds;
} UIMasterDetailPanel;

void ui_StartPanel(Vector2 position, UILayout layout);
Rectangle ui_EndPanel();

void ui_AddTextNode(char *text, int font_size);
void ui_AddSeparator(int thickness);
void ui_AddOptionButton(const char *label, int *value, char **options, int option_count, int font_size);
IsActive ui_AddButton(char *text, int font_size);
void ui_AddSwitchButtons(int button_count, int *active_button_idx, char **labels, int font_size);
int ui_AddToolbar(int button_count, char **labels, int font_size);
IsActive ui_AddIntInput(char *label, int *value, int font_size, bool is_edit_mode);
IsActive ui_AddTextInput(char *label, char *text_value, int font_size, bool is_edit_mode);
IsActive ui_AddFloatInput(char *label, float *value, int font_size, bool is_edit_mode);
UIComboBoxResult ui_AddComboBox(const UIComboBoxProps *props);

void ui_MasterDetailBegin(UIMasterDetailPanel *panel, Vector2 origin, float gap);
void ui_MasterDetailBeginMaster(UIMasterDetailPanel *panel, UILayout layout);
Rectangle ui_MasterDetailEndMaster(UIMasterDetailPanel *panel);
void ui_MasterDetailBeginDetail(UIMasterDetailPanel *panel, UILayout layout);
Rectangle ui_MasterDetailEndDetail(UIMasterDetailPanel *panel);
Rectangle ui_MasterDetailGetBounds(const UIMasterDetailPanel *panel);
