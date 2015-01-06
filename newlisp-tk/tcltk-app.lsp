;; tcltk-app.lsp - application demo
;;
;; to run on LINUX:
;;
;; ./newlisp-tk tcltk-app.lsp
;;
;; to run on Win2k
;;
;; newlisp-tk.exe tcltk-app.lsp
;;
;;
;; newlisp-tk version 0.72 or later is required
;;
;;
;;

(context 'App)

(define (app-example ) 
  (tk "toplevel .appex")
  (tk "button .appex.bExit -text Exit -command exit")
  (tk "pack .appex.bExit -side top -padx 60 -pady 30")
  (tk "bind .appex <Destroy> exit"))


(context 'MAIN)

(tk "wm withdraw .")

(App:app-example)
