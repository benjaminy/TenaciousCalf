# Persistent Data Structures

## Abstract

Persistent data structures are structures like lists, sets, maps and
priority queues that have a curious property.  Every time a program
modifies a persistent structure (for example, inserting "42" after the
4th element in a list), it gets back both a new structure that reflects
the update and the old pre-update structure.

The naive way to implement persistent structures is make a full copy for
every update.  This would be insanely inefficient in terms of both time
and memory.  The first surprising thing about persistent structures is
that they can be quite competitive with their conventional cousins: In
many cases asymptotically equal; in the worst case logarithmically
slower/bigger.

In this talk I'll cover some classic examples of persistent structures,
explain why they have been growing in popularity recently and discuss
work that remains to be done.

## Outline:
1. What? (part 1)
2. How? (part 1)
   - Singly-linked lists
3. Why?
   - Undo
   - Parallel software
   - Interactive software
4. The bad news: high overhead
5. How? (part 2)
   - Diffing
   - Path copying
6. Chunking
7. High-degree trees
8. Transience
9. Hash-mapped array tries
10. Remaining challenges
   - Efficient implementations
   - Verification

## What? (part 1)

The data structures I'm interested in for this talk are things like
lists, arrays, sets, key/value maps and priority queues.  In most
mainstream programming ecosystems the default implementations of these
structures are _ephemeral_.  This means that if I have a set of Alice,
Bob and Carol and I add Dave to that set, the original set is gone.
Adding "D" to the set {"A", "B", "C"} to create {"A", "B", "C", "D"}
makes {"A", "B", "C"} "go away".

Persistent data structure def'n:

> A data structure is _persistent_ if updates create new versions
> without destroying previous versions.

    S1 = { "A", "B", "C" }
    S2 = add( S1, "D" )
    print S1    # prints A, B, C
    print S2    # prints A, B, C, D

If this seems strange, consider if we replaced sets in this example with
numbers:

    S1 = 42
    S2 = S1 + 17
    print S1    # prints 42
    print S2    # prints 59

In many popular languages there is a small set of primitive data types
(numbers, Booleans, strings in some cases) and everything else is
handled through references by default.  One nice slogan for persistent
data structures is _all data are values_.

## How? (part 1)

Hopefully this seems a little magical.  Here's a quick preview of how
this kind of thing is possible.

![alt text](../Images/simple_list1.png "Simple list")

![alt text](../Images/simple_list2.png "Two lists!")

I'm leaving out lots of details at this point, but a crucial fact is
that it's possible to implement persistent data structures more
efficiently than making a complete copy for every update.  That would be
a total non-starter for most applications.

## Why?

1. Undo

  In many applications the user performs a series of actions that modify
  the state of some document.  Undo is a common and extremely useful
  feature.  How can we implement undo?  Persistent data structures can
  make it quite simple.  Keep a list of all versions (or just the most
  recent N).  When the user asks to undo, just jump back.

  Example: Firefox retroactive private mode.

2. Parallel software

  The ubiquity of parallel processors (i.e. multicores) has dramatically
  increased the interest in parallel software.  Unfortuantely making
  parallel software that works properly is an extremely hard problem.
  One of the important pieces of that is multiple processors accessing
  (and potentially modifying) the same data structure at the same time.
  Persistent data structures simplify one strategy for easing the
  implementation of parallel software.  Construct-and-publish pattern.

3. Interactive software

  Similar basic argument.

## The Bad News

Even though persistent structures can be implemented without gratuitous
copying, they're still generally less efficient than their ephemeral
cousins.

1. In some cases the best known persistent structures have a log(N)
slowdown factor.
2. Even when the asymptotics are comparable, textbook persistent
structures have pretty bad constant-factor inefficiencies.

Simple example: array of bytes versus a binary tree.

1. Assume each byte is stored in a leaf node.
2. The number of internal nodes in a binary tree is equal to the number
of leaves minus 1.
3. Each internal node needs at least two pointers to its children.  On a
64-bit computer this means each internal node will be at least 16 bytes.
4. Implementing a sequence of bytes as a simple binary tree has
17&times; memory overhead.
5. Also accessing a particular byte requires dereferencing log(N)
pointers.

These kinds of efficiency differences have been bad enough to keep
persistent structures on the margins of mainstream software development.

## How? part 2

Two major techniques:

1. Diffing
2. Balanced trees and path copying

## Memory Efficiency: Chunking

Instead of storing a single value in each leaf, store a short array of
values (a _chunk_).

Short arrays

N + N(16/C)

1.5&times; memory overhead.

Depth of the tree is still log(N / C) = log(N) - log(C)

## Time Efficiency: High branching factor

Instead of using a binary tree we can build a tree where each internal
node has many children.  This is called a B-tree.  Historically B-trees
have been associated with on-disk databases ("external" storage).
Recently they have gained some popularity for in-memory data structures.

log<sub>32</sub>(1B) &#8776; 6

## Copying big nodes is no fun

Chunking and high-degree trees are generally a good idea, but there is
tension with path copying.  Recall that each update to a tree-based
persistent structure requires copying a (very) small number of paths
from a leaf up to the root.  An array of 32 64-bit pointers is 256Bytes.
In other words, 1/4 of a kB.  That's a non-trivial amount of data to be
copying frequently.

"Transient" data structures to the rescue!  In many applications it's
not necessary to save a version of the structure after every little
update.  Updates naturally cluster together into "transactions".  So the
idea is to update the structure in-place (i.e. treat it like an
ephemeral structure) most of the time, and only do persistent updates
when the application wants to save a particular version for later use.

## Hash-Mapped Array Tries

Hash-based data structures are extremely widely used because they have
excellent performance characteristics for many applications.
Conventional hash-based structures are based on ephemeral arrays and
until fairly recently noone knew if it was possible to make efficient
persistent hash-based structures.

Enter hash-mapped array tries!

### First lets talk about tries

The name comes from re<strong>trie</strong>val, but most people ponounce it "try" to
avoid confusion with "tree".

Tries are special kinds of trees that efficiently store collections of
data that have common prefixes.

![alt text](../Images/simple_trie.png "Simple trie")

Tries were discovered in the process of making a Scrabble-playing
program back in the 1970s.

## What Left To Do?

Data structures is an area of computer science that has received a lot
of research attention, so what's left?

### Still fancy variants to work out.

### Proofs of efficiency.

### Formally verified implementations.
