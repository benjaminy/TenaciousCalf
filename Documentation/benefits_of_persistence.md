# Benefits of Persistence

This page is going to be very brief; there is a ton of material online about what's good about persistent data structures.
In particular the Clojure community has been advancing the cause of persistent data structures in mainstream software recently.
Search for talks by Rich Hickey and David Nolen as a starting point.

A quick highlight reel of what's good about persistent data structures:

* They simplify the implementation of undo functionality
* They simplify the implementation of backtracking algorithms and "what-if" analysis
* They generally have good asymptotics for fancy operations like concatenation and slicing
* They simplify the implementation of some parallelism patterns
* They simplify the implementation of some multitasking patterns

The latter two have increased in importance a lot recently, with the rise of multicore processors and distributed applications (i.e. sophisticated web apps).
