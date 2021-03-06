The data structures in this library are in persistent/functional/immutable family.
"Tenacious" is a play on "persistent", because the Tenacious Calf (TC) structures are a little different from the most common kind of persistent structures.
Copy-on-write is an important part of how the TC structures work.
Copy-on-write is sometimes abbreviated "COW", and the TC structures apply the concept in a smaller/more fine-grained way that is common; therefore "calf".

#### Why are you using "multi-version"? I prefer X.

I don't like the values of X that I'm familiar with:

* **Immutable**. I think this is the most popular right now, but I don't
  like the connotations. It sounds like you can't change the data, which
  is _kinda_ true but it's much nicer to focus on the fact that
  "changing" a piece of data is really making a new version while
  leaving the old version intact. "Immutable" might win the branding
  race, but I dislike it.
* **(Purely) Functional**. This name is inherited from the fact that
  multi-version data structures are popular in functional languages.  I
  don't think it's a particularly descriptive name and C is decidedly
  unfriendly to functional programming.  I'm trying to make it easier to
  make mainstream software parallel, and a lot of developers of such
  software have a badittude about the "functional" brand, so it doesn't
  seem like a good choice.
* **Persistent**. I kind of like this name, but unfortunately it clashes
  with the use of "persistent" from databases.  This library has nothing
  to do with secondary storage durability, so that makes "persistent" a
  bad choice.
* **Value-oriented/focused**. (As opposed to reference-oriented.) I kind
  of like this one too, but I think it's pretty cryptic to most
  programmers.

So I'm going with multi-version until someone convinces me that it's a
bad idea.
