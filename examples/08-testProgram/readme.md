Test Program for Linker Test

This directory builds a "program" and links it to the kernel built in "Linker Test".

The trick is that we link directly against the kernel.elf file with the
LD --just-symbols flag, so that the linker resolves any references to
addresses in the kernel (functions, variables, etc) to their absolute,
0x8000, based, addresses.

Unfortunately, in this limited scheme, we have to tell the linker
exactly where we, the program, are in memory, because the compiler
generates relative calls to the absolute addresses in the kernel,
in spite of the "prositition independent" settings used in the
compiler and linker.

Nonetheless, with this simple trick, given that we just arbritrarily
select an address of 0x80000 (above the kernel) as where we
will be loaded in memory, with a minimum of fuss, we can link a program,
and ram it into memory at run time, and execute it, and in turn, it
can make sophisticated calls back to the kernel as if it were part
of it (new'ing objects, registering methods, etc).

The "program" should not "do" anything, or very much, anyways.

A more sophisticated incarnation would at least define a life cycle
for the "program", with the startup being called "onLoad()" or something
like that, and then you have the whole question of timeslicing, as well
onUnload().

I have not tried much with this. It also would at least need formalization
vis-a-vis standard C concepts, like clearing a .bss segment, caalling
static ctors, and sysinit functions.

Nonetheless, it is interesting, so I am keeping it.
