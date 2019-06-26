//
// vu_meter.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
// Copyright (C) 2019  Patrick Horton
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef _vu_meter_h
#define _vu_meter_h

#include <audio/Audio.h>
#include <ugui/uguicpp.h>


class CVuMeter : public CTextbox
{
public:
    
    // vuMeter controls should be sized so that their major dimension plus 1
    // is evenly divisible by num_divs
    
	CVuMeter(CWindow *win, u8 instance, u8 id, s16 x, s16 y, s16 xe, s16 ye,
        u8 horz, u8 num_divs);

    bool Callback(UG_MESSAGE *pMsg);
    
private:

    AudioAnalyzePeak *m_pAudioObj;
    
    CWindow *m_pWin;
    u8 m_horz;
    u8 m_num_divs;
    u8 m_last_value;          // scaled to num_divs
    u16 m_hold_red;
    bool m_running;

};


#endif  // _vu_meter_h
