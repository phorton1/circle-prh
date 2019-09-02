// program.cpp
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// Circle Squared - Copyright (C) 2019 Patrick Horton

#include "../07-LinkerTest/kernel.h"

#define log_name "program"


extern "C"
{
	// #define PRINTF  0xeff8
	// $define PRINTF  &printf
	// typedef void (*printfMethod)(const char *f, ...);
	// extern void printf(const char *f,...);
	// ((printfMethod)PRINTF)(format,i++);
	
	int onLoad()
		// This method is called by the kernel when the program is loaded.
		// It remains a simple, though time consuming, excersize, to expand
		// this example to include the UI system, so that this "program"
		// could instantiate windows and get timeslices and have behaviors.
		//
		// The real exersize though, would be to define the life cycle,
		// do real position-independent loading of multiple "programs",
		// manage them within the (ui?) system, and implement shared
		// libraries, a file system user interface, development tools,
		// and so on .. 
	{
		int i=0;
		LOG("onLoad(%d)",i++);
		printf("PROGRAM: onLoad(%d)\n",i++);
		return 237;
	}
	
	
}