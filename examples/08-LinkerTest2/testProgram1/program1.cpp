// program1.cpp
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// Circle Squared - Copyright (C) 2019 Patrick Horton

#include <system/std_kernel.h>

#define log_name "program1"


void onUnload()
{
	LOG("onUnload",0);
}


extern "C"
{
	u32 onLoad()
		// the onLoad method, which can be called at the origin of the program memory,
		// returns the address of the onUnload method() which can be anywhere
	{
		LOG("onLoad",0);
		return (u32) &onUnload;
	}
}