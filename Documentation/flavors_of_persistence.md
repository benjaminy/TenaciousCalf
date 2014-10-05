# The Many Flavors of Persistence

Persistent data structures come in at least a few different flavors
including:

- Full
- Partial
- Semi
- On demand
- Confluent
- Thread-safe

This page gives quick descriptions of each flavor.  Note that when one
talks about the operations and patterns that a particular flavor of
persistence _can support_, what that really means is "can perform
asymptotically more efficiently than what you can get with a
non-persistent data structure".  For example, you can make a complete
clone of just about any data structure, but that's going to be an O(n)
operation without some fancy engineering.

### Full

"Fully persistent" is what most people mean most of the time when when
they talk about persistent structures.  All updates to a structure give
you back both the old (pre-update) and new versions.  Both old and new
versions can be queried and updated without restrictions.  You can
visualize these updates creating a tree of versions.

### Partial

With partially persistent structures, client code can query old
versions, but not update them.  With partial persistence, the version
"tree" is just a linear chain.

### Semi

With semi-persistent structures, old versions can be queried and
updated.  However, updating an old version invalidates any "more recent"
versions.  Visually, you can think of all of the branches of the version
tree other than the one with the most recent version being "dead".  This
is particularly useful for backtracking algorithms.

### On-demand

With on-demand persistence, the client code chooses for each access
whether it should be done in persistent mode or transient mode.  You can
think of each version in the version tree being tagged with a
persistent/transient bit.  Old transient versions cannot be queried or
updated, but all old persistent versions can be.

### Confluent

"Confluently persistent" means that versions from different branches of
the version tree can be merged asymptotically more efficiently than a
na&iuml;ve merge of unrelated instances of that type.

### Thread-safe

This one might seem odd, since one of the selling points of persistent
structures is that they make it easier to write reliable multithreaded
software.  The issue is that some persistent structures use lazy
evaluation or memoization to pull off their magic.  This means that
while logically old versions are immutable, physically old versions may
refer to unevaluated expressions that get filled in at some later time.

Thread safety cannot be taken for granted when you're talking about lazy
evaluation/memoization.  So if you're working in a language/framework
where laziness is built in, you can probably rely on the language
implementers to get it right.  If you're rolling your own lazy
evaluation in a language like C, you'd better be really careful.
