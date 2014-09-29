In case you haven't noticed, figuring out how to get everyday software
to run fast on multicore processors has been a pretty hot topic since
the mid-aughts.  This graph explains why more succinctly than I can with
words.

![Processor performance graph](/Images/performance_graph.png)

(Credits: C. Moore, M. Horowitz, F. Labonte, O. Shacham, K. Olukotun,
L. Hammond and C. Batten)

At least four ideas about how regular programmers should write parallel
software have excited various researchers. At the risk of offending good
people, I believe these techniques are all at best stalled right now:

* **Automatic Parallelization**. This one has come in lots of flavors
   since at least the 1970s. Such an appealing idea. So hard. Still
   working on it.
* **Functional Programming**. Some functional language enthusiasts
   believe that it would all work out if we just used Haskell, OCaml,
   Scala, Clojure, ... Maybe it's true, and some of those languages have
   made serious progress in the developer mindshare market, but they're
   still a distant afterthought in most mainstream development circles.
   (And just switching to a functional language doesn't automagically
   solve the parallel software problem.)
* **Transactional Memory**. Cool idea. Hung up on thorny implementation
   issues. Forecast cloudy.
* **Parallel Collections**. Works great for simple software patterns.
   Scales badly with application complexity.


#### What is the connection between multi-version data and parallelism?

Once a particular version of a multi-version data structure is created,
it is never modified. This means that multiple threads can read it with
no possibility of data races. This is huge in terms of avoiding
concurrency bugs.



The simple process for modifying multi-version data is that one thread
builds a new version in private memory, then "publishes" it by
atomically updating a shared reference. This pattern is described in
more detail below. Again, this leaves no possibility of data races.


_The excuse_.  The whole _raison d'&ecirc;tre_ of this library is use in
parallel software.  Update-in-place structures are extremely hard to use
correctly in parallel software without falling into the pit of data
races, deadlocks, atomicity violations, etc.  The more common approaches
of using locks and/or crazy lock-free structures come with their own
overheads and problems.  You can think of the write overhead as the
price we pay for sane parallel use of collections.

#### How do threads actually share data?

Modifications create new structures, so how does one thread "see"
versions created in other threads?  The normal pattern is to have one
shared reference to the root of a particular shared collection.  To
update in a way that other threads can see, a thread first makes a new
version then performs an atomic write on the shared reference.  Like so:

```
updated = False
while( !updated )
{
    items = atomic_deref( shared_items );
    ref_count_incr( items );
    items_with_item = add( items, new_item );
    updated = atomic_compare_and_set( shared_items, items_with_item, items );
    ref_count_decr( items );
}
```


If multiple threads are trying to update the same collection
simultaneously, only one of them will "win" and others will have to
retry.  This can make it tricky to ensure fairness, but it's not hard to
ensure some progress is made.  On the plus side, a thread can make
multiple changes to a structure and just perform one atomic set on the
reference.  This is an easy way to provide a kind of application-level
atomicity.
