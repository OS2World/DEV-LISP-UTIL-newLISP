;; Demo.lsp - graphics demo for newLISP-tk
;;
;; to run: (Demo:run)
;;
;;

(context 'Demo)

(define (run )
  (tk "if {[winfo exists .dw] == 1} {destroy .dw}")
  (tk "toplevel .dw")
  (tk "wm geometry .dw 200x340+540+80")
  (tk "label .dw.lb -width 20 -height 1")
  (tk "button .dw.turtle -relief groove -text {Turtle Graphics}")
  (tk "button .dw.random -relief groove -text {Random M&Ms}")
  (tk "button .dw.hanoi -relief groove -text {Towers Of Hanoi}")
  (tk "button .dw.mouse -relief groove -text {Mouse drawing}")
  (tk "button .dw.drag -relief groove -text {Object dragging}")
  (tk "button .dw.hide -relief groove -text {Hide Console} -command {wm withdraw .}")
  (tk "button .dw.quit -text Quit -command {destroy .dw}")
  (tk "pack .dw.lb .dw.turtle .dw.random .dw.hanoi .dw.mouse -side top -pady 6")
  (tk "pack .dw.lb .dw.drag .dw.hide .dw.quit -side top -pady 6")
  (tk ".dw.turtle config -command {Newlisp {(silent (load \"Turtle.lsp\"))}}")
  (tk ".dw.random config -command {Newlisp {(silent (load \"Random.lsp\"))}}")
  (tk ".dw.hanoi config -command {Newlisp {(silent (load \"Hanoi.lsp\"))}}")
  (tk ".dw.mouse config -command {Newlisp {(silent (load \"Mouse.lsp\"))}}")
  (tk ".dw.drag config -command {Newlisp {(silent (load \"Drag.lsp\"))}}")
  (tk "bind .dw <Destroy> { wm deiconify . }")
  (tk "wm title .dw { Demo.lsp}"))

(context 'MAIN)

(Demo:run)

