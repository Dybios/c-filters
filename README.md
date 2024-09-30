# Implementation of various filters in C

Ensure GCC is installed along with the libmath library.

To compile for LPF demo, use the following command:

gcc main_app.c iir/lpf/lpf.c -o main_app -lm


NOTE: This is only an initial proof of concept. Following characteristics still present.

* Only first order implemented.

* Rolloff is not very steep.

* ~Glitches seen between frames.~
