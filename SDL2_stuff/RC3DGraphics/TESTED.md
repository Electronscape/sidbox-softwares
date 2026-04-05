SDL2 Raycasting 3D world (a bit like DOOM)
Going to see if i can get one working with a little better than just raycasting to blocks in memory, much like the way Wolfenstein does things

This time going to try a crude, but hopefully affect way of using just vector2s, and portals and try to texture them

ALSO going to try and see if i can stick to the basic colour system
HOWEVEr with 256 colours, lets reserve the first 32 colours and THEN use the rest as shading (will try to implement a floyd style dither too as an optional graphics enhancement)

IF THIS WORKS, i'll turn this in to a nice library.
HOWEVER the level designer SHOULD allow for easy mapping, and i havent come up with a plan for BSP yet, so that will be fun

with this demo though i'll make it so that the WHOLE map vertexs are translated instead of a "camera view" I believe this while a little heavy, but muuuuuch simpler for world navication and rotations

THIS could also all just blow up in my face and force me to never program again, until I get some more chocolcate cake!
