Mu-BitMap
==
Mu-BitMap is a multi-level bitmap library providing a very efficient almost
constant time get and set operations for extremely large bitmaps of sizes in
billions bits. It also comes with a write through cache, writing the bitmap to
a persistent storage.
In the present implementation 3 levels of bitmaps are used. The entire bitmap is
assumed to be present in main memory.
