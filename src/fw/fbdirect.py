#!/usr/bin/env python3
# karabas: FB=direct build copy of upstream i_video.c: Doom draws straight into the SRAM frame
# buffer (I_VideoBuffer = FB_BASE, a fixed window; on the board the hardware maps the back
# buffer behind it and flips at vblank), I_FinishUpdate only marks the frame (no 64 KB copy).
import sys
src, dst = sys.argv[1], sys.argv[2]
s = open(src).read()
old = 'I_VideoBuffer = (byte*)Z_Malloc (SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);'
assert s.count(old) == 1
s = s.replace(old, 'I_VideoBuffer = (byte*)FB_BASE;')
old = 'Z_Free (I_VideoBuffer);'
assert s.count(old) == 1
s = s.replace(old, '')
old = 'void I_FinishUpdate (void)\n{\n'
assert s.count(old) == 1
s = s.replace(old, old + '    DG_DrawFrame(); return;\n')
s = '#include "mmio.h"\n' + s
open(dst, 'w').write(s)
