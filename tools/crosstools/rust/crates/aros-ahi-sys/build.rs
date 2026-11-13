fn main() {
    println!("cargo:rerun-if-env-changed=AROS_CC");
    println!("cargo:rerun-if-env-changed=AROS_AR");
    println!("cargo:rerun-if-changed=src/ahi.c");

    if std::env::var("CARGO_CFG_TARGET_OS").as_deref() != Ok("aros") {
        return;
    }

    // The crate store's cargo config passes the target compiler as AROS_CC/AROS_AR.
    let mut build = cc::Build::new();
    if let Ok(cc) = std::env::var("AROS_CC") {
        build.compiler(cc);
    }
    if let Ok(ar) = std::env::var("AROS_AR") {
        build.archiver(ar);
    }

    // AROS executables are relocatable objects: no PIC, which cc adds by default.
    build.file("src/ahi.c").pic(false).compile("aros_ahi_glue");
}
