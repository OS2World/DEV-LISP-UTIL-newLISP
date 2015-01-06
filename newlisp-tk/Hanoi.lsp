;; Hanoi.lsp - graphics demo for newLISP-tk
;;
;; to run: (Hanoi:run 6)
;;
;;


(context 'Hanoi)

(define (delete-disk n)
  (tk ".hw.can delete disk" n)
  (tk-yield))

(define (hanoi-prep )
  (tk "if {[winfo exists .hw] == 1} {destroy .hw}")
  (tk "toplevel .hw")
  (tk "canvas .hw.can -width 505 -height 310 -bg #f0f0f0")
  (tk "pack .hw.can")
  (tk "wm geometry .hw +10+10")
  (tk "wm title .hw { hanoi.lsp}")
  (tk ".hw.can create text 150 30 -fill navy -font {Times 18} -text {Towers Of Hanoi}")
  (tk-yield))

(define (make-disk disk pole height , x x1 y y1 width)
  (set 'x 
   (case pole 
    (pole1 115) 
    (pole2 235) 
    (pole3 355)))
  (set 'width (+ 20 (* 10 disk)))
  (set 'x (- x (/ width 2)))
  (set 'y (- 250 (* 10 height)))
  (set 'x1 (+ x width))
  (set 'y1 (+ y 10))
  (tk ".hw.can create rectangle " x " " y " " x1 " " y1 " -fill red -tag " 
    "disk" disk)
  (tk-yield))

(define (move n from to with)
  (if (> n 0) 
   (begin 
    (move (- n 1) from with to) 
    (pull-disk n from) 
    (move-disk-over n from to) 
    (push-disk n to) 
    (move (- n 1) with to from))))

(define (move-disk-over n from to)
  (delete-disk n)
  (make-disk n to 18))

(set 'pole1 '())

(set 'pole2 '())

(set 'pole3 '(1 2 3 4 5))

(define (pull-disk n pole)
  (pop (eval pole))
  (delete-disk n)
  (make-disk n pole 18))

(define (push-disk n pole)
  (push n (eval pole))
  (delete-disk n)
  (make-disk n pole (length (eval pole))))

(define (run n)
  (hanoi-prep)
  (if (> n 10) 
   (set 'n 10))
  (setup-poles n)
  (move n 'pole1 'pole3 'pole2))

(define (setup-poles n)
  (set 'pole1 (set 'pole2 (set 'pole3 '())))
  (tk ".hw.can create rectangle 50 250 440 260 -fill green")
  (tk ".hw.can create rectangle 110 90 120 250 -fill green")
  (tk ".hw.can create rectangle 230 90 240 250 -fill green")
  (tk ".hw.can create rectangle 350 90 360 250 -fill green")
  (tk-yield)
  (while (> n 0) 
   (push n pole1) 
   (make-disk n 'pole1 (length pole1)) 
   (dec 'n)))

(define (tk-yield )
  (tk "after 15; update idletasks"))


(context 'MAIN)

(Hanoi:run 6)

