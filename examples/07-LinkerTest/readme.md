Linker Test

This kernel tests a loader for separately linked executables.

By default the kernel is loaded at 0x8000 and is limited to about 2 megs
upto 0x200000

This kernel occupies about 250K.

So I found a cheezy, yet simple, way to load additional code into the system.

It currently just makes the assumption that there is one loadable executable,
and that it will be loaded at 0x80000, allowing for a 0.5 meg (0x80000-0x8000)
sized kernel.

This constant address is the problem.

The idea is to expand this example so that multiple programs can be dynamically
loaded, as well as to implement sharaed libraries of some sort.

The paradigm for timeslicing between the "programs" is based on the kernel
event system, which is yet to be developed, but this example does load an
executable that creates a window within the existing ui (wsWindows) system.

The main problem with dynamic run time linking and loading is the amount of
bytes I have to transfer back and forth to the rPi, which in turn, wants me
to implement a user interface / remote interface to the sdcard file system,
which is just getting to be a pain in the butt.

Though I could use ethernet, I am still using serial because I want backwards
compatability with the zero that doesn't have an ethernet connector.

So the whole project was just to break the code up into smaller chunks
so that I'm not uploading a full kernel for each minor change.  However,
that means that I'd have to write a raft of stuff, and due to the symbols
associated with run time linking, I'd end up having to move more bytes,
in total, than I would without the whole thing.

Nonetheless, I sort of got it working, so I thought I'd save it.

This project builds the kernel.  Please see the testProgram project
for the program that is uploaded.
