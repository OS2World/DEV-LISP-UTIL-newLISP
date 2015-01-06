;; Mouse.lsp - graphics demo for newLISP-tk
;;
;; to run: (Mouse:run)
;;
;;

(context 'Mouse)

(define (clear )
  (tk ".mw.can delete art"))

(define (color )
  (set 'line-color (tk "tk_chooseColor -parent .mw -title {Pick line color}"))
  (if (= line-color "") 
   (set 'line-color "red"))
  (tk "focus -force .mw.can; update idletasks"))

(define (down x y)
  (set 'px x)
  (set 'py y))

(define (drag x y)
  (if (not px) 
   (set 'px x))
  (if (not py) 
   (set 'py y))
  (tk ".mw.can create line " px " " py " " x " " y " -fill " 
    line-color " -width 2 -tag art")
  (set 'px x)
  (set 'py y))

(set 'line-color "#0080c0")

(set 'px 299)

(set 'py 366)

(define (run )
  (tk "if {[winfo exists .mw] == 1} {destroy .mw}")
  (tk "toplevel .mw")
  (tk "canvas .mw.can -width 510 -height 390 -bg #FFFFF0")
  (tk ".mw.can config -cursor plus")
  (tk "pack .mw.can")
  (tk "wm geometry .mw +100+100")
  (tk "wm title .mw { Mouse.lsp}")
  (tk ".mw.can create text 100 370 -fill navy -font {fixed 10} -text {click and drag the mouse}")
  (tk ".mw.can create rectangle 270 358 315 380 -fill beige -tag clear")
  (tk ".mw.can create rectangle 340 358 385 380 -fill beige -tag color")
  (tk ".mw.can create text 292 368 -fill navy -font {fixed 10} -text clear -tag clear")
  (tk ".mw.can create text 362 368 -fill navy -font {fixed 10} -text color -tag color")
  (tk "bind .mw <Button-1> {Newlisp {(silent (Mouse:down %x %y))}}")
  (tk "bind .mw <B1-Motion> {Newlisp {(silent (Mouse:drag %x %y))}}")
  (tk ".mw.can bind clear <Button-1> {Newlisp {(silent (Mouse:clear))}}")
  (tk ".mw.can bind color <Button-1> {Newlisp {(silent (Mouse:color))}}")
  (tk "after 100; update idletasks")
  (set 'line-color "red"))


(context 'MAIN)

(Mouse:run)

