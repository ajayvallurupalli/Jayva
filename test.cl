(define apply
  (lambda (f x) 
    (f x)))

(define apply2
  (lambda (f x y) 
    (f x y)))

(define compose
  (lambda (f g)
    (lambda (x) (f (g x)))))

(define add1 
  (lambda (x) 
    (+ x 1)))

(define sub1 
  (lambda (x)
    (- x 1)))

(define print
  (lambda (x) 
    (spy x 'unit)))

(force (print "hello world"))
