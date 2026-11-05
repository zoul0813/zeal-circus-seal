#!/usr/bin/env python3
"""Pack SDK-exported tiles and metadata from git-tracked assets/acts/*.tmx."""
from tmx_helpers import *
a=options(); maps=load_maps('acts/act[0-9][0-9].tmx','ACT_COUNT',(20,15))
write(a.out/'acts.ztm',b''.join(m.tiles(a) for m in maps))
objects_file(a,maps,['Player','Clown'],'acts')
write(a.out/'map_data.c','#include "game.h"\nconst char *const ACT_NAMES[ACT_COUNT] = {\n'+''.join('    '+c_string(m.props['name'])+',\n' for m in maps)+'};\n')
