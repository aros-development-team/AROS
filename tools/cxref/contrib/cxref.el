;; $Header$
;;
;; C Cross Referencing & Documentation tool. Version 1.3.
;;
;; Some useful Emacs lisp functions for use with cxref.
;; Adds in comments of the appropriate format for cxref.
;;
;; Written by Andrew M. Bishop
;;
;; This file Copyright 1995,96 Andrew M. Bishop
;; It may be distributed under the GNU Public License, version 2, or
;; any higher version.  See section COPYING of the GNU Public license
;; for conditions under which this file may be redistributed.
;;

;;;###autoload
(defun cxref-c-mode-common-hook () "Set up the key bindings for cxref in cc-mode"
  (define-key c-mode-map "\C-c\C-f"  'cxref-file-comment)     ;; Control-C Control-F
  (define-key c-mode-map "\C-cf"     'cxref-function-comment) ;; Control-C f
  (define-key c-mode-map "\C-cv"     'cxref-variable-comment) ;; Control-C v
  (define-key c-mode-map "\C-ce"     'cxref-endline-comment)  ;; Control-C e
  (define-key c-mode-map "\C-ci"     'cxref-inline-comment)   ;; Control-C i
)

;;;###autoload
(add-hook 'c-mode-common-hook 'cxref-c-mode-common-hook)

;; Insert a file comment suitable for parsing with cxref

(defun cxref-file-comment () "Inserts a file comment suitable for parsing with cxref" (interactive)
  (let ((cp (make-marker)))

    (goto-char (point-min))

    (insert   "/***************************************") (c-indent-command)
    (insert (concat "\n$" "Header" "$")) (c-indent-command)
    (insert "\n")
    (insert "\n")(c-indent-command)
    (set-marker cp (point))
    (insert "\n***************************************/") (c-indent-command)
    (insert "\n\n\n")
    (while (looking-at "\n") (delete-char 1))

    (if (string-match "\\.h$" (buffer-file-name))
        (let ((st) (defname (file-name-nondirectory (buffer-file-name))))
          (while (setq st (string-match "\\." defname))
            (setq defname (concat (substring defname 0 st) "_" (substring defname (+ 1 st) nil))))
          (setq defname (upcase defname))
          (insert (concat "#ifndef " defname "\n"))
          (insert (concat "#define " defname "    /*+ To stop multiple inclusions. +*/\n"))
          (insert "\n")
          (goto-char (point-max))
          (while (looking-at-backward "\n") (delete-char -1))
          (insert "\n\n")
          (insert (concat "#endif /* " defname " */\n"))
          ))

    (goto-char cp) (c-indent-command)
    ))

;; Insert a function comment suitable for parsing with cxref

(defun cxref-function-comment () "Inserts a function comment suitable for parsing with cxref" (interactive)
  (let ((bp (make-marker)) (cp (make-marker)) (fp (make-marker)) (as) (ae) (a) (depth 0))

    (beginning-of-line)

    (while (looking-at-backward "\n\n") (delete-backward-char 1))

    (insert "\n\n")
    (insert "/*++++++++++++++++++++++++++++++++++++++") (c-indent-command)
    (insert "\n")
    (set-marker bp (point))
    (insert "\n")
    (set-marker cp (point))
    (insert "++++++++++++++++++++++++++++++++++++++*/") (c-indent-command)
    (insert "\n\n")

    (if (looking-at "static") (re-search-forward "[ \t\n]+"))
    (set-marker fp (point))

    (if (not (looking-at "void[\t ]+[a-zA-Z0-9_]"))
        (progn
          (setq as (point))
          (search-forward "(") (backward-char)
          (setq ae (point))
          (setq a (buffer-substring as ae))
          (goto-char cp)
          (insert "\n")
          (insert a) (c-indent-line)
          (insert "\n")
          (set-marker cp (point))
          (goto-char fp)
          )
      )

    (search-forward "(") (set-marker fp (point))

    (catch 'finished-args

      (while t

        (if (looking-at-backward ")")
            (throw 'finished-args t))

        (re-search-forward "[\n\t ]*")
        (set-marker fp (point))
        (setq as (point))

        (while (not (and (= depth 0) (looking-at "[,)]")))
          (if (looking-at "[({]")
              (setq depth (1+ depth)))
          (if (looking-at "[)}]")
              (setq depth (1- depth)))
          (forward-char))

        (set-marker fp (1+ (point)))

        (re-search-backward "[\n\t ]*")
        (setq ae (point))

        (setq a (buffer-substring as ae))

        (if (not (or (string= a "void") (string= a "")))
            (progn
              (goto-char cp)
              (insert "\n")
              (insert a) (c-indent-line)
              (insert "\n")
              (set-marker cp (point))
              ))
        (goto-char fp)
        ))

    (goto-char bp) (c-indent-command)
    ))

;; Insert a variable comment suitable for parsing with cxref

(defun cxref-variable-comment () "Inserts a variable comment suitable for parsing with cxref" (interactive)
  (let ((fp (make-marker)))
    (beginning-of-line)

    (while (looking-at-backward "\n\n") (delete-backward-char 1))

    (insert "\n")
    (insert "/*+ ")
    (set-marker fp (point))
    (insert " +*/")
    (insert "\n")

    (goto-char fp) (c-indent-command)
    ))

;; Inserts an end of line comment that is parsed by cxref

(defun cxref-endline-comment () "Inserts an end of line comment that is parsed by cxref" (interactive)
  (let ((fp (make-marker)))
    (end-of-line)
    (indent-to-column (c-comment-indent))

    (insert "/*+ ")
    (setq fp (point))
    (insert " +*/")

    (goto-char fp)
    ))

;; Insert an inline comment that is not parsed with cxref

(defun cxref-inline-comment () "Inserts an inline comment that is not parsed with cxref" (interactive)
  (let ((fp (make-marker)))
    (beginning-of-line)

    (while (looking-at-backward "\n\n") (delete-backward-char 1))

    (insert "\n")
    (insert "/* ")
    (set-marker fp (point))
    (insert " */")
    (insert "\n\n")

    (goto-char fp) (c-indent-command)
    ))

;;  A Very Useful Function

(defun looking-at-backward (arg)
  (save-excursion
    (let ((cp (point)) (return))
      (if (re-search-backward arg (point-min) t)
          (if (re-search-forward arg cp t)
              (if (= (point) cp)
                  (setq return t)
                )))
      return
      )))
