This is where I keep my softwares, and tools and scrap code to make sure things at least work before I start adding things to the sidbox firmware.
So far ALL these projects are written in QT6's Creator, I have VisualCode but for essentially ANY GUI work, QTCreator is on point.

If you're following this, you should be able to cloan this compile it with your own system to see what they do. Sorry there isn't much else at the moment. 

Added tools to help me do things and test
- SIDBOX GRAPHICS TEST: (very early stages at the moment)
  Essentially this is the testing ground for any new Sidbox Graphics routines. It's designed to work like the VRAM in the sidbox's pixel array arrangement.
  I can test new features without burning it to ROM first. 

- SIDBOX TERMINAL:
  This allows me to communicate directly to the sidbox, it's a little bit like terminal, since the sidbox internally sends commands to this, giving it an almost perfect commandline system.
  The sidbox terminal still under development with the more things I need to test. I'll add its function here.
  This uses QT6, and is being coded in the Linux FEDORA KDE environment (the source code should still compile in windows, tested and it does with minor tweaks)
  
- the HID stuff is mainly trying to get the USB data packets to be sent directly though to the sidbox, but this is causing more issues, and they DON'T WORK! 

- SIDBOX5.BIN is the actual firmware for the sidbox hardware. This MIGHT make things easier for people to download the update. Turns out for me its highly convenient, and since its on a cloud, if I ever lose my hosting package the file should be available.
  I'll make a "older versions" though just incase, with some also experimental firmwares later: One of them is just a barebones OS: literally all it will have is sound/display/sdcard and an OS to launch software from. 
  SIDBOX OS :)
