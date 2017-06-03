# World of Warcraft TBC Field of View Fix

A World of Warcraft client service that optimizes the game's client for wide screen resolutions. It brings the modern field of view behavior of the WotLK client back to the TBC 2.4.3 client. Unlike the previous tools, it is fully automatic and does not require any user input. The game window is monitored and the field of view is kept updated whenever the game world is reloaded or the resolution changes.

Illustrations, comparisons and the code this is based on can be found [here](https://www.reddit.com/r/wowservers/comments/5i3fwa/field_of_view_fix_for_1121_pics_explanation/).

### Usage

This tool is very simple to use. You only need to copy the `WoWFoVFix.exe` next to the `Wow.exe` file in the game folder and use that instead of the game client to start the game. The program will run the WoW client by itself with the field of view values modified to better match your resolution.

When used, the program creates a `WoWFoVFix.log` text file in the same directory that contains debug information of the last run. If you feel like the fix isn't working, check this log for any errors. Make sure it is used with the 2.4.3 version of the game client.

To compile this yourself, either use the included VS 2015 project, or create one yourself. No special project options are required, but to hide the console window, set the subsystem to `WINDOWS` and entry point to `mainCRTStartup`.

### How it works

After starting the client, the program runs as a service that periodically checks for the in-game camera to be created. This happens when the user logs in and the world gets loaded. It then calculates the appropriate field of view based on the aspect ratio of the window the game is running in. This value is inserted into the game's process through memory editing and is kept updated at all times. The process exits when the game is closed.

### Safety

Great care was taken to ensure the program does not do anything it shouldn't. Only the field of view value in the game's process is edited and nothing else. That is enforced by checking for the current value in the process and comparing it with the expected one. That way, if something unexpected happens, the program stops working rather than writing data to some unknown memory location.

Therefore, as long as the server you are playing on does not directly check for the camera's field of view, it shouldn't be detected by the Warden anti-cheat. Still, I do not guarantee the safety of your characters, so use this at your own risk.
