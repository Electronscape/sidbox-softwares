installing audio!! had some issues with WINE, uninstalled reinstalled,killed ALSA! Whoops

NOTE TO SELF:

Install the 64-bit version of the plugin
sudo dnf install alsa-plugins-pulseaudio.x86_64

verify:
ls /usr/lib64/alsa-lib/libasound_module_pcm_pulse.so


should compile again then
