// loader.h
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// Circle Squared - Copyright (C) 2019 Patrick Horton


#ifndef _loader_h
#define _loader_h

#include <circle/types.h>
#include <fatfs/ff.h>

#define LOAD_SUCCESS  0
#define LOAD_NO_FILE  1
#define LOAD_NO_OPEN  2
#define LOAD_NO_READ  3 
#define LOAD_BAD_READ 4


class CLoader
{
public:
	
	CLoader(FATFS *fs);
	~CLoader();
	
	u8 loadProgram(const char *filename);
	int callProgram();

protected:

	FATFS *m_pFS;

	u32 m_size;
	u8 *m_buffer;
	void *m_entry_point;
	
	void init();
	
};

#endif
