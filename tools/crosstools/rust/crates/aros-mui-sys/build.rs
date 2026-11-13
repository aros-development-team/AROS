fn main() {
    if std::env::var("CARGO_CFG_TARGET_OS").as_deref() != Ok("aros") {
        return;
    }

    // MUI_* stubs; muimaster_autoinit.o in the same archive opens the library.
    println!("cargo:rustc-link-lib=muimaster");
}
