;;;
;;; Emacs JDK Project File
;;; maintained by Oliver Steele
;;; 
;; * E_LZ_COPYRIGHT_BEGIN ******************************************************
;; * Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
;; * Use is subject to license terms.                                          *
;; * E_LZ_COPYRIGHT_END ********************************************************

(jde-project-file-version "1.0")
(jde-set-variables)

;(setq jde-ant-invocation-method '("Java"))
;(setq jde-ant-invocation-method '("Script"))
;(setq jde-ant-args "-emacs -Dbuild.compiler.emacs=true compile")
(setq jde-ant-args "-emacs -Dbuild.compiler.emacs=true")
;(setq jde-ant-home "$LPS_HOME/../tools/jakarta-ant-1.5.1")
(setq jde-ant-enable-find t)

(setq jde-build-function '(jde-ant-build))
(setq jde-make-working-directory "build/")
(setq jde-sourcepath '("./src" "./sc/src"))
(setq jde-lib-directory-names '("^lib" "^dev"))
;(setq jde-global-classpath '("lib/lps.jar"))
(setq jde-global-classpath '(
        "$LPS_HOME/3rd-party/jars/dev"
        "$LPS_HOME/WEB-INF/lib"))

(setq jde-bug-vm-includes-jpda-p t)
(setq jde-bug-jdk-directory (getenv "JAVA_HOME"))

(setq jde-db-option-classpath jde-run-option-classpath)
(setq jde-db-option-vm-args jde-run-option-vm-args)
(setq jde-db-option-application-args jde-run-option-application-args)

(setq jde-compile-option-directory "build")

(setq jde-run-application-class "org.openlaszlo.compiler.Main")
(setq jde-run-option-classpath
      (append '("$LPS_HOME/server/lib/lps_noship.jar")
              jde-global-classpath))
(setq jde-run-working-directory "$LPS_HOME/test")
(setq jde-run-option-vm-args
      (list (concatenate 'string "-DLPS_HOME=" (getenv "LPS_HOME"))))
(setq jde-run-option-application-args '("hello.lzx"))
;jde-run-read-app-args
;debug depend

;(push '("javadoc" "doc" nil) jde-help-docsets)

