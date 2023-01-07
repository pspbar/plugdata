---
title: logistic~
description: Logistic chaotic generator

categories:
 - object

pdcategory: General

arguments:
- type: float
  description: sets frequency in hertz
  default: 0
- type: float
  description: sets p
  default: 0.5

inlets:
  1st:
  - type: float/signal
    description: frequency in hertz (negative values accepted)
  2nd:
  - type: float/signal
    description: 'p' parameter (from 0 to 1)

outlets:
  1st:
  - type: signal
    description: logistic chaotic signal

---

[logistic~] is a chaotic generator based on the difference equation: y[n] = (3 + p) * y[n-1] * (1- y[n-1]).
