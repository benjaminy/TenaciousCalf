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

## /Abstract

## Outline:
1. What?, part 1
2. How?, part 1
   - lists
3. Why?, part 1
   - Undo
     - Firefox forget the last N minutes
   - Parallel software
   - Interactive software
4. The bad news: high constant-factor overhead
   - Tree with bytes
5. B-trees
6. Transience
7. Hash-mapped array tries
8. Remaining challenges
   - Efficient implementations
   - Verification

## What?, part 1

The data structures I'm interested in for this talk are things like
lists, arrays, sets, key/value maps and priority queues.  In most
mainstream programming ecosystems the default implementations of these
structures are ephemeral.  This means that if I have a set of Alice, Bob
and Carol and I add Dave to that set, the original set is gone.  Adding
D to the set {A, B, C} to create {A, B, C, D} makes {A, B, C} go away.

Persistent data structure def'n:
A data structure is _persistent_ if updates create new versions without
destroying previous versions.

    S1 = { "A", "B", "C" }
    S2 = add( S1, "D" )
    print S1
    print S2

## How?, part 1

Hopefully this seems a little magical.  Here's a quick preview of how
this kind of thing is possible.

S1 -> "A" -> "B" -> "C" -|

S2 -> "D" -^

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

3. Interactive software

## The Bad News

Even though persistent structures can be implemented without gratuitous
copying, they're still generally less efficient than their ephemeral
cousins.  In some cases the best known persistent structures have a
log(N) slowdown factor.  Even when the asymptotics are comparable,
persistent structures tend to have pretty bad constant-factor
inefficiencies.

Simple example: raw array of bytes versus a balanced tree.

More than a 10x memory.  log(N) access slowdown.  These kinds of
efficiency differences have been bad enough to keep persistent
structures on the margins of mainstream software development.

## How? part 2

Two major techniques:
- Diffing
- Balanced trees and path copying

## Memory Efficiency: Large Nodes

Short arrays

## Time Efficiency: High branching factor

log<sub>32</sub>(1B) &#2248; 6

