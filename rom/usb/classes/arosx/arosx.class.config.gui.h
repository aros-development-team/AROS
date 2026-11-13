#ifndef AROSXClassGUI_H
#define AROSXClassGUI_H

struct AROSXClassConfigGUI {
    Object             *App;
    Object             *MainWindow;
    Object             *UseObj;
    Object             *CloseObj;

    Object             *GamepadGroupObject;
    Object             *GamepadObject_button_a;
    Object             *GamepadObject_button_b;
    Object             *GamepadObject_button_x;
    Object             *GamepadObject_button_y;
    Object             *GamepadObject_button_ls;
    Object             *GamepadObject_button_rs;
    Object             *GamepadObject_left_thumb;
    Object             *GamepadObject_right_thumb;

    Object             *GamepadObject_dpad_left;
    Object             *GamepadObject_dpad_right;
    Object             *GamepadObject_dpad_up;
    Object             *GamepadObject_dpad_down;

    Object             *GamepadObject_button_back;
    Object             *GamepadObject_button_start;

    Object             *GamepadObject_left_trigger;
    Object             *GamepadObject_right_trigger;
    Object             *GamepadObject_left_stick_x;
    Object             *GamepadObject_left_stick_y;
    Object             *GamepadObject_right_stick_x;
    Object             *GamepadObject_right_stick_y;

    Object             *AboutMI;
    Object             *UseMI;
    Object             *MUIPrefsMI;
};

#endif /* AROSXClassConfigGUI_H */