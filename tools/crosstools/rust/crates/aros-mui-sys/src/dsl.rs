use core::ffi::CStr;
use crate::*;

// Class names without the MUIC_ prefix, so obj!(Window { ... }) finds MUIC_Window.
pub mod class {
    use crate::*;
    pub const Application: &[u8] = MUIC_Application;
    pub const Window: &[u8] = MUIC_Window;
    pub const Group: &[u8] = MUIC_Group;
    pub const Text: &[u8] = MUIC_Text;
}

// The short names from the C macros in libraries/mui.h.
pub const Child: Tag = MUIA_Group_Child;
pub const SubWindow: Tag = MUIA_Application_Window;
pub const WindowContents: Tag = MUIA_Window_RootObject;

pub unsafe fn simple_button(label: &CStr) -> *mut Object {
    MUI_MakeObjectA(MUIO_Button as LONG, [label.as_ptr() as IPTR].as_mut_ptr())
}
