# Why
I wanted to play some music for everybody in games using my microphone. An obvious and nasty approach is to just put the microphone in front of the speakers but the quality will be awful. Another way is to switch the game's input from microphone to the main sink's "monitor" source. The downside is that you'll send all the desktop audio including the game's sounds to the mic. The most accurate way to achieve the desired result is to create a secondary sink that sends all the input to the master sink (your audio card). This sink will also have a monitor source that you can connect to the game instead of the microphone source.

# Usage
First, add a secondary sink to `/etc/pulse/default.pa` like this:
```
load-module module-remap-sink sink_name=secondary master=alsa_output.pci-0000_00_1f.3.analog-stereo
```

You can find the master sink name using `pacmd list-sinks`. With this sink (named `secondary` in this case) you also get a monitor source. Now all you need to do is restart PulseAudio and launch this program with parameters like this:
```
paswitch -i 'My Pulse Output' -k secondary -o 'ALSA Capture' -c secondary.monitor
```

Here, `My Pulse Output` is the audio player's sink input name (check `pavucontrol` to easily find these names, this one will be on the `Playback` tab), `secondary` is our intermediate sink, `ALSA Capture` is the game's source output name (on the `Recording` tab) and `secondary.monitor` is the intermediate source. If you used the aforementioned `load-module` line the `-k` and `-c` parameters will be the same all the time, all you need is find the sink input and source output names when your player and game are running. Make sure your player has opened the sink (i.e. it's playing or paused, check `pavucontrol`) or else it won't work. After the program connects your player to the game you can turn the mic on in game and play the song.

To switch it all back and use your actual microphone use:
```
paswitch -i 'My Pulse Output' -k alsa_output.pci-0000_00_1f.3.analog-stereo -o 'ALSA Capture' -c alsa_input.pci-0000_00_1f.3.analog-stereo
```

The `-i` and `-o` parameters are the same as in the previous command, these are your player's and game's sink input and source output respectively. The `-k` and `-c` parameters correspond to the hardware sink and source respectively, you can find them using `pacmd list-sinks` and `pacmd list-sources`.

You can also add `-n` parameter to see desktop notifications if the switch was successful or an error happened (requires `libnotify-dev` installed).

Now bind those commands on some hotkeys to easily switch back and forth and entertain other players in the pregame or actual game!
