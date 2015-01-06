;; Drag.lsp - graphics demo for newLISP-tk
;;
;;
;; to run: (Drag:run)
;;
;;

(context 'Drag)

(define (draw-grid )
  (for (x 5 570 20) 
   (tk ".drw.can create line " x " 0 " x " 390 -fill lightgray"))
  (for (y 5 390 20) 
   (tk ".drw.can create line 0 " y " 590 " y " -fill lightgray")))

(define (run )
  (tk "if {[winfo exists .drw] == 1} {destroy .drw}")
  (tk "toplevel .drw")
  (tk "canvas .drw.can -width 510 -height 390 -bg #FFFFFF")
  (tk "pack .drw.can")
  (tk "wm geometry .drw +110+150")
  (tk "wm title .drw { Drag.lsp}")
  (draw-grid)
  (tk "update idletasks")
  (tk ".drw.can create text 150 374 -fill navy -font {fixed 10} " 
   "-text {click and drag objects with the mouse}")
  (tk ".drw.can create rectangle 100 30 200 170 -fill blue -tag movable")
  (tk ".drw.can create oval 300 100 400 200 -fill red -tag movable")
  (tk ".drw.can create text 200 260 -fill darkgreen -font {fixed 18} -text {Drag Me} -tag movable")
  (tk ".drw.can bind movable <Button-1> {CanvasMark %x %y %W}")
  (tk ".drw.can bind movable <B1-Motion> {CanvasDrag %x %y %W}"))

(set 'x 10)


(context 'MAIN)

(Drag:run)

