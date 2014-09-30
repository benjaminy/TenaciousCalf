# A Little Adventure in List Microbenchmarking

To drive home the importance of memory overhead, give this little
experiment a try.  Write a program that does the following:

- Create an array and a singly-linked list of equal length
  - Preferably make the length big enough that the array/list can't fit
    in on-chip cache
- Fill up the array/list with some arbitrary numbers
- Iterate over the array/list accumulating some simple result (like the
  sum)

In my implementation of this little experiment iterating over the linked
list took almost 10&times; the time of iterating over the array.  That's
a fairly striking number, but what comes next is the really intersting
part:

- Make the list doubly-linked

The only thing that changes between the two versions of this experiment
is the density of application data in the list (it goes from 1/2 to
1/3).  In my implementation, this change nearly doubled the run time of
iterating over the list!  Data density is super important!
