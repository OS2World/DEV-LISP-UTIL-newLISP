;; Turtle.lsp - graphics demo for newLISP-tk
;;
;; to run: (Turtle:run)
;;
;; 1.1 changed floats to integers in (draw) to make it work 
;;     in locales, which use the decimal comma, which confuses Tcl/Tk
;;
;; 1.2 eliminated decimal constants with points for safe execution 
;;     under locale with comma as decimal separator
;;
;;
;;


(context 'Turtle)

(set 'color "blue")

(define (dragon sign level)
  (if (= 0 level) 
   (turtle-forward 4) 
   (begin 
    (dec 'level) 
    (turtle-right (sign 45)) 
    (dragon - level) 
    (turtle-left (sign 90)) 
    (dragon + level) 
    (turtle-right (sign 45)))))

(define (dragon-curve n clr)
  (set 'color clr)
  (dragon + n))

(define (draw )
  (set 'points (map int points))
  (tk ".tw.can create line " (join (map string points) " ") " -fill " 
   color)
  (tk "update idletasks")
  (set 'points (list newX newY)))

(define (draw-rect n)
  (turtle-forward n)
  (turtle-right 90)
  (turtle-forward n)
  (turtle-right 90)
  (turtle-forward n)
  (turtle-right 90)
  (turtle-forward n))

(define (draw-squirl n)
  (dotimes (x (/ n 3)) 
   (turtle 'forward n) 
   (turtle 'right 90) 
   (set 'n (- n 2)))
  (turtle-go-to 0 0))

(define (rose clr)
  (set 'color clr)
  (dotimes (x 90) 
   (draw-rect 60) 
   (turtle-right 2)))

(define (run )
  (tk "if {[winfo exists .tw] == 1} {destroy .tw}")
  (tk "toplevel .tw")
  (tk "canvas .tw.can -width 500 -height 400 -bg #FFFEC0")
  (tk "pack .tw.can")
  (tk "wm geometry .tw +100+160")
  (tk "wm title .tw { Turtle.lsp}")
  (tk ".tw.can create text 380 70 -fill navy -font {Times 12} -text {Dragon Fractal}")
  (tk ".tw.can create text 100 350 -fill navy -font {Times 16} -text {Turtle Graphics}")
  (tk "after 300; update idletasks")
  (set 'pi2 (acos 0))
  (set 'dp2 (div pi2 90))
  (turtle-start 300 50)
  (dragon-curve 12 "red")
  (draw)
  (turtle-start 120 200)
  (rose "blue"))

(define (turtle-center )
  (set 'lastX 150)
  (set 'lastY 60)
  (set 'newX 150)
  (set 'newY 60)
  (set 'direction pi2))

(define (turtle-forward d)
  (set 'newX (add lastX (mul (cos direction) d)))
  (set 'newY (add lastY (mul (sin direction) d)))
  (push newX points -1)
  (push newY points -1)
  (if (= (length points) 100) 
   (draw))
  (set 'lastX newX)
  (set 'lastY newY))

(define (turtle-go-to x y)
  (set 'lastX x)
  (set 'lastY y))

(define (turtle-left d)
  (set 'direction (add direction (mul d dp2))))

(define (turtle-right d)
  (set 'direction (sub direction (mul d dp2))))

(define (turtle-start x y)
  (set 'lastX x)
  (set 'lastY y)
  (set 'newX x)
  (set 'newY y)
  (set 'points (list x y))
  (set 'direction pi2))

(context 'MAIN)

(Turtle:run)
