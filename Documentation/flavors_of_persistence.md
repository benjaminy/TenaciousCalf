# The Many Flavors of Persistence

Persistent data structures come in at least a few different flavors
including:

- Full
- Partial
- Semi
- On-demand
- Confluent
- Thread-safe

This page gives quick descriptions of each flavor.  Note that when one
talks about the operations and patterns that a particular flavor of
persistence _supports_, what that really means is "performs
asymptotically more efficiently than na&iuml;vely doing a ton of
copying."

### Full

"Fully persistent" is what most people mean most of the time when they
talk about persistence.  All updates to a structure give you back both
the old (pre-update) and new versions.  Both old and new versions can be
queried and updated without restrictions.  You can imagine a collection
of operations creating a tree of versions of the data structure.

### Partial

Partially persistent data structures support querying old versions, but
not updating.  In other words, the version "tree" is just a linear
chain.

### Semi

Semi-persistent data structures support querying and updating old
versions, with a catch: Updating an old version invalidates any "more
recent" versions.  You can think of all of the branches of the version
tree other than the "most recent" one being "dead".  This is useful for
backtracking algorithms.

### On-demand

On-demand-persistent data structures can be updated in persistent mode
or transient mode, according to the whims of client code.  You can think
of each version in the version tree being flagged as persistent or
transient.  Old transient versions cannot be queried or updated, but all
old persistent versions can be.

### Confluent

Confluently persistent data structures are actually "stronger" than
plain fully persistent structures.  In addition to regular persistent
updates, versions from different branches of the version tree can be
combined more efficiently than combining unrelated instances of that
type.  Confluence turns the version "tree" into a directed acyclic
graph.

### Thread-safe

This one might seem odd, since one of the selling points of persistent
structures is that they make it easier to write reliable multithreaded
software.  The issue is that some persistent structures use lazy
evaluation or memoization to pull off their magic.  This means that
while old versions are logically immutable, they may refer to
unevaluated expressions that get filled in later.  This means that under
the hood, the bits referred to by old versions can actually change.

Thread safety cannot be taken for granted when you're talking about lazy
evaluation/memoization.  So if you're working in a language/framework
where laziness is built in, you can probably rely on the language
implementers to get it right.  If you're rolling your own lazy
evaluation in a language like C, you'd better be _really_ careful.
