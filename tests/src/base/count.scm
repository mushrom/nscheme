(define (countdown n)
  (if (> n 0)
    (begin
      (display n)
      (newline)
      (countdown (- n 1)))
    0))

;; => 5
;; => 4
;; => 3
;; => 2
;; => 1
(countdown 5)
