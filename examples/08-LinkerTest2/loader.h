// loader.h
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// Circle Squared - Copyright (C) 2019 Patrick Horton


#ifndef _loader_h
#define _loader_h

#include <circle/types.h>
#include <fatfs/ff.h>


class CLoader
{
public:
	
	CLoader(FATFS *fs);
	~CLoader();
	
	int loadProgram(const char *filename);
    void unloadProgram();

protected:
    
	FATFS *m_pFS;
    
    u32 m_location;
    u32 m_unload_location;
	
};

#endif
