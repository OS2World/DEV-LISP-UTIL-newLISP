;; Random.lsp - graphics demo for newLISP-tk
;;
;; to run: (Random:run)
;;
;;

(context 'Random)

(set 'colors '("800000" "800080" "8000FF" "808000" "808080" "8080FF" 
  "80FF00" "80FF80" "80FFFF" "FF0000" "FF0080" "FF00FF" "FF8000" 
  "FF8080" "FF80FF" "FFFF00"))

(define (fireworks )
  (set 'x (rand 450))
  (set 'y (rand 330))
  (set 'sz (rand 100))
  (tk ".rw.can create oval " x " " y " " (+ x sz) " " (+ y sz) " -fill #" 
   (nth (rand 16) colors)))

(define (run )
  (tk "if {[winfo exists .rw] == 1} {destroy .rw}")
  (tk "toplevel .rw")
  (tk "canvas .rw.can -width 510 -height 390 -bg #000000")
  (tk "pack .rw.can")
  (tk "wm geometry .rw +110+150")
  (tk "wm title .rw { Random.lsp}")
  (tk "after 300; update idletasks")
  (dotimes (i 300) 
   (if (= (% i 20) 0) 
    (tk "update idletasks")) 
   (fireworks)))

(context 'MAIN)

(Random:run)
