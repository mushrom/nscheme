; TODO: remove these functions once a libary system is
;       implemented
(define (print x)
  (display x)
  (newline))

(define (for-each func xs)
  (if (null? xs)
    ()
    (begin
      (func (car xs))
      (for-each func (cdr xs))))) 

; functions to test arithmetic operations
(define (add a b)
  (print (+ a b)))

(define (subtract a b)
  (print (- a b)))

(define (multiply a b)
  (print (* a b)))

(define (divide a b)
  (print (/ a b)))

(define (add-one n)
  (print (+ n 1)))

; addition
;; => 2
(add 1 1)

;; => 12345
(add 10000 2345)

;; => 5
(add 10 -5)

; subtraction
;; => 9013
(subtract 10000 987)

;; => 0
(subtract 1234 1234)

;; => 0
(subtract 0 0)

;; => 5
(subtract -5 -10)

; multiplication tests
;; => 9
(multiply 3 3)

;; => 144
(multiply 12 12)

;; => -50
(multiply -5 10)

;; => 0
(multiply 0 0)

; diviision tests
;; => 3
(divide 9 3)

;; => 1
(divide 123 123)

;; => -5
(divide 25 -5)

; TODO: implement a good way of testing division by zero
;(divide 10 0)

;; => 2
;; => 3
;; => 4
;; => 5
(for-each add-one '(1 2 3 4))
