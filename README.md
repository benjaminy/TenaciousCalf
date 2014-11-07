# Ligament

Chunky, On-Demand Persistent Data Structures

---

_First they ignore you.  Then they ridicule you.  And then they attack
you and want to burn you.  And then they build monuments to you.  And
that, is what is going to happen to the functional style of
programming._

(Apologies to the ghost of Nicholas Klein.)

[TL;DR: This project is an experiment in making persistent data
structures that can compete with their transient/mutable cousins in
terms of asymptotic and constant-factor performance.  The main
techniques used to get there are chunking to improve memory efficiency
and on-demand hybrid persistence/transience.]

Persistent data structures have been studied for decades, but had little
impact on the lives of most working programmers until recently.  The
principal reasons for the changing tide are:

* Mainstream applications are increasingly interactive compared to their
  forebears (network communication, UI, physical controls, etc).
  Interactivity is directly related to complex dynamic internal
  dependencies, and persistent data makes it less likely that a change
  made in one corner of a large application will have unintended
  consequences somewhere else down the road.
* Parallel processors have gone mainstream.  Persistent data makes it
  easier to write parallel software without going insane in the pit of
  data races, deadlocks and atomicity violations.

Of course persistent structures have always had other cool tricks up
their sleeve:

* They simplify the implementation of undo functionality
* They simplify the implementation of backtracking algorithms and
  "what-if" analysis
* They generally have better asymptotics than their transient cousins
  for fancy operations like concatenation and slicing
* &hellip;

If you're not familiar with persistent data structures, Rich Hickey has
some great introduction presentations up on youtube and infoq.

Researchers have done amazing work discovering persistent data
structures that match (or come close to) their transient cousins in
terms of asymptotics.  Unfortunately, textbook persistent structures
typically have very high constant-factor overhead compared to the
aforementioned cousins.  The reasons for this overhead are:

* Persistent data tend to be pointer-heavy.  This is essential, not
  incidental.  Large persistent structures must be split up into
  relatively small objects to keep the cost of persistent updates down
  to a dull roar.  This has two related but distinct consequences:
  * Application data density in persistent structures is low; in some
    cases _very_ low.  Application data density matters a lot, because
    it has a direct impact on how efficiently an application uses every
    level of the memory hierarchy, from L1 cache to secondary storage
    paging.
  * It typically takes a least a few sequentially-dependent memory loads
    to get to the data your application is interested in.  Each of these
    loads is an opportunity to miss at each level of the memory
    hierarchy, and cache misses are terrible for performance.  Also,
    because the accesses are sequentially dependent, it is not possible
    to pipeline them.  Modern architectures love pipelining.
* Every update to persistent data involves copying a small handful of
  objects.
  * This work is totally unnecessary for mutable data.
  * It is a fairly expensive kind of work: allocating and freeing lots
    of small-ish objects.

[A cute little microbenchmarking exercise that illustrates the
importance of data density and pointer
chasing](/Documentation/linked_list.md).

How bad are these performance costs?  It's hard to give a simple answer
to general performance questions like that, but it's not at all hard to
cook up a microbenchmark that shows well-implemented textbook persistent
structures performing 10&times; slower than array-based cousins.
Two caveats:

* This is not an apples-to-apples comparison.  Persistent data gives
  your software the superpower of using multiple versions of a structure
  simultaneously; mutable data can't do that.
* As with any microbenchmarking, the impact on application performance
  will be diluted by all the other stuff that your application does.

Caveats notwithstanding, the performance overhead of these structures is
high enough to be a real liability for lots of applications.  (Of course
the details matter a lot.  "Persistent data structures are fast enough"
and "Persistent structures are too slow" are both laughably simplistic
slogans.)  This project is an attempt to spread the use of persistence
by implementing high-performance hybrid persistent collections like
sets, vectors, maps and graphs in C.  The two main techniques used to
achieve high performance are:

* _Chunking_.  Textbook persistent data structures have lots of very
  small nodes.  Chunking is the strategy of grouping together a small
  number of "nearby" nodes into a modestly-sized raw array (or similarly
  efficient encoding).  Structures that exemplify this strategy are
  B-trees and hash array mapped tries.  Chunking improves data density
  and reduces the amount of pointer chasing.
* _Local transience_.  The structures in this library are not purely
  persistent, but rather hybrid persistent/transient.  Each structure is
  either in persistent or transient mode at any given time.  Critically,
  converting between modes is extremely cheap.  Converting to transient
  mode logically copies the entire structure, but in implementation
  terms only a copy of the root node is required.  Other pieces of the
  structure are copied lazily, as needed.  Updates in transient mode are
  cheap, because it is usually not necessary to make copies of nodes.
  Converting to persistent mode is even cheaper: just set the
  persistent/transient flag in the root.

Hybrid persistence allows applications to use the following pattern:

1. Make a local transient copy of a data structure
2. Perform a raft of updates quickly in transient mode
3. Make the structure persistent again and share it with the rest of the
application

This pattern mostly preserves the software engineering benefits of pure
persistence, and brings substantial performance benefits.  As far as I
know, Rich Hickey pioneered this use of transience (I'd love to hear
about prior work).  You can read his thoughts on the matter here:
http://clojure.org/transients.

These two features (chunking and transience) have fairly strong synergy.
Chunking tends to improve read performance by prefetching nearby data
(i.e. exploiting spatial locality) and reducing the number of pointer
hops, but it hurts write performance because copying chunks is more
expensive than copying very small nodes.  Transience dramatically
reduces the number of node copies needed.  So if an application can
arrange to do most of its updates in transient mode bursts, it can get
the best of both worlds.

_A note to functional programming enthusiasts_: transient updates return
a root that may or may not refer to the same memory as the root before
the update.  After the update the "old" root can no longer be used
safely.  Various kinds of static and dynamic checking can help ensure
that programs don't violate this rule, but it is a potential pitfall.

## Here are the parameters of the project in Q&amp;A format:

#### What is a persistent data structure?

_Note_: The relevant definition of _persistent_ has nothing to do with
serialization, databases, secondary storage, etc.

A (purely) persistent data structure is one that cannot be changed once
it is created.  "Updating" a persistent structure means creating a new
version that is like the old one but with some change applied.  For
those who are totally unfamiliar with this programming style, here is a
tiny taste of what it looks like:

* **Conventional update-in-place style**. The <span
  style="font-family:monospace;">items</span> structure is actually
  changed:

  ```
  add( items, new_item );
  ```

* **Persistent style**. <span
  style="font-family:monospace;">items</span> is not changed and both
  <span style="font-family:monospace;">items_with_item</span> and <span
  style="font-family:monospace;">items</span> can be used going forward:

  ```
  items_with_item = add( items, new_item );
  ```

#### Whoa!  Does the library have to copy the whole structure?

No!  Copying a large data structure for every update would be insanely
inefficient.  The central trick is that under the hood everything is
some linked collection of nodes (often trees).  Modifying a structure
(i.e. making a new version) only requires copying a very small number of
paths from one or a few nodes to the root.  Most of the new structure is
shared with the old structure.

#### Trees?!?  Won't that create a log-slowdown?

Yes, but&hellip; There is a lot of very clever research out there on
fancy ways to do persistent structures with good asymptotics.  Things
like fingers and tries and whatnot.  So sometimes we can actually get
worst-case or amortized constant time.  Then we throw in chunking which
effectively makes the base of the logarithm high.  And a high-base
logarithm is darn close to constant FMIAP (for most intents and
purposes).

(See for example the work of Phil Bagwell which has been taken up
enthusiastically in the Clojure ecosystem.)

#### Every persistent update creates a new structure; what happens to the old ones?

All extensive persistent data structure libraries I am familiar with are
implemented in languages with garbage collection, so old versions can
just be collected when they are no longer reachable.

Obviously the assume-garbage-collection strategy doesn't work in C, so
the structures in this library use reference counting to know when old
versions can be deallocated.  By default the modification procedures
decrement the reference count of the old version, so the common case of
no longer needing the old version has little code overhead.  If you
actually need both the old and new versions, you need to increment the
reference count of the old version before modifying it.

This is one of the areas of novelty in this project, so it will need
serious scrutiny and experimentation.

#### This sounds like writes are really expensive [angry face].

Yes, but&hellip; Updates to persistent data structures generally _are_
more expensive than update-in-place data structures, but:

* Transient mode is much cheaper
* Write performance might not be as important as you think
* You're buying something pretty valuable

_The workaround_.  The data structures in this library can be converted
between persistent mode and transient mode.  Transient mode doesn't mean
that we throw away all the fancy trees and just make a giant array.  So
writing in mutable mode might still have a little overhead, but it
should be substantially lower than persistent mode updates.  More on
this below.

_The evasion_.  In most applications, reads vastly outnumber writes.  In
that case, overhead on writes has relatively little impact on
application performance.

_The excuse_.  Persistent data structures have benefits that are not
available to update-in-place structures.  A few are mentioned at the top
of this page.  There are lots of research papers, blog posts and
presentations devoted to these benefits.  So your application can buy
something useful with the price that it pays in modest performance
overhead.

#### Okay, maybe the time overhead is tolerable.  What about memory?

With any kind of pointer-based structure we have to think about memory
overhead.  Consider an extreme case with an array of bytes on one side
and a simple reference-counted binary tree with a single byte per node
on the other.  On a 64-bit machine each node will probably cost 32 bytes:
16 for the two pointers, 8 for the reference count, and 8 for the data
(because of alignment constraints). 32&times; memory overhead is
horrendous; totally intolerable for a reasonably large collection.

The basic strategy for keeping memory efficiency high is "chunking" a
modest number (say 32) of values together into a single array.  So the
bytes of the leaves are mostly devoted to a plain array of application
data.  The goal for the project is sub-2&times; overhead compared to the
most compact reasonable implementation.  (If this sounds implausible
think about how very small the number of internal nodes is compared to
the number of leaves in a high-degree tree.)

It's hard to overstate the importance of memory efficiency, because it
is directly related to the amount of actual application data that can
fit in _every level_ of the memory hierarchy.  Efficient use of the
memory hierarchy is among the most important factors for general
application performance tuning.  (After getting the asymptotics in the
right ballpark, of course.)

#### What is this transient mode thing?

Consider the following situation.  You have a vector of values.  Some
application code wants to change the value at some index.  With a
mutable structure, you just change the value at a particular spot in
memory.  With a persistent structure you have to allocate a few new
nodes and copy over a few chunks of memory.  This is a pretty
substantial constant-factor performance difference.  If you don't need
the pre-update version of the vector and you're not worried about
concurrent access right now, it's pretty unpalatable.

All of the structures in this library can be converted to transient
mode.  In transient mode, it is allowed to actually mutate the
structure.  Simple updates like writing a new value at a particular
index are still a bit more expensive than if you were using a raw array,
because the tree structure has to be maintained.  But you save quite a
bit in memory management overhead.

The benefit of transient mode is proportional to the number of updates
you do before converting back to persistent mode.  In the extreme,
performing a single update in transient mode before converting back is
almost identical (in performance) to just doing a persistent update.

#### I smell monads.

Transient mode is unsafe in the sense that you are not "allowed" to use
a reference to a structure after using it as the input to an update
procedure, but this restriction is not necessarily enforced.  This
situation is similar to realloc.

One could imagine using something like Haskell's ST monad or Clean's
uniqueness types to catch programs that tried to reuse an "old"
transient structure at compile time.  One could also do some additional
dynamic checking for this situation.  So far I haven't done either; the
C style is to mostly operate on the honor system.

As far as I'm aware, Haskell's ST monad doesn't automatically give you
the extremely cheap switching between persistent and transient mode.  I
don't know Haskell well enough to say how hard it would be to implement
data structures with the same performance characteristics as this
library.  Here's one attempt to do something like that: [ezyang's
blog](http://blog.ezyang.com/2010/03/the-case-of-the-hash-array-mapped-trie/).

#### Can you summarize the overhead story?

So glad you asked!  The high level picture is that persistent data
structures have to pay _something_ for their superpower of having
multiple simultaneously usable versions of the same complex structure.
This library makes every effort to keep memory and read overhead as low
as possible, at the expense of write overhead.

There is lots of evidence for persistent data structures with really bad
performance overhead.  There is a lot of engineering work here to do
this well.  Here's a random collection of related links.

* http://ayende.com/blog/164740/immutable-collections-performance-take-ii

#### I don't buy it.  Conventional persistent structures are fast enough.

If you're using persistent data structures in your application and not
experiencing performance challenges, there's nothing for you here.  Move
along.

#### What is the connection with parallelism?

[Parallelism and persistence](/Documentation/parallelism.md)

#### Tell me more about the jargon used here.

[What's in a name?](/Documentation/whats_in_a_name.md)

#### Is this related to semi-persistence or partial persistance?

[The many flavors of persistence](/Documentation/flavors_of_persistence.md)

#### Why C?  C sucks.

In some important ways, C is the most portable programming language
around, which means:

* Writing a library in C gives it the best chance to get picked up in
  lots of different contexts.
* Bindings to C libraries can be written in most languages.

There's also a dash of perverse challenge in the language choice.  Is it
possible to make (hybrid) persistent data structures that can hold their
own performance-wise with raw array-based structures in C?  Not sure.
We'll see.

#### Allocation?

Modifying persistent data structures requires allocating memory.  I'd
rather not bake in a dependency on malloc and friends.  I'll have to
look into techniques for making the allocation method configurable.

#### Contained data type?

The classic approach for generic containers in C is to use void pointers
all over the place, then force client code to cast.  That kinda sucks
both for reliability and for efficiency of small plain old data types.
The other common option is preprocessor hackery to generate
type-specific code for each kind of thing you want to store.  More
thinking required.

## Status

Currently this is just a proposal.  Not sure when I'll get time to work
on it.  But if it catches your fancy feel free to email
ben.ylvisaker@coloradocollege.edu.  I am especially interested in
hearing about related projects that I've missed.
