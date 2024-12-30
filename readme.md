# wlinspector
needlessly overcomplicated and hacky way to grab window info on wayland (crippled version of xwininfo) as an experiment for my other project: [obsplay](https://github.com/lolepop/obsplay)

proof of concept: does not include any service units, install scripts, etc.

# what it do
1. proxy library is loaded into all running processes via ld.so.preload or LD_PRELOAD
2. proxy hooks wayland functions and stores various registry info (handles for internal registered interfaces)
3. all relevant info is relayed to and retrieved from a central dbus daemon

# what it dont do
we can only retrieve what a client knows/sets (title/window size/fullscreen state). this implies that info like window focused state (might be) impossible to determine.

this would have to be handled by the wm/compositor/whatever its called on wayland and results in xdg portal types of horrendous fragmentation