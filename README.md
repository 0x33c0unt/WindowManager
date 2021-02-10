# Window Manager

Window manager for Windows Operating System. 

Shortcuts;

ALT+LCLICK : MOVE/DRAG

ALT+RCLICK : RESIZE

ALT+DOUBLE LCLICK : Full Screen and Restore

ALT+CTRL+T : Set on Top and Vice Versa

ALT+CTRL+D : DLL Inject 

ALT+CTRL+P : PAUSE and RESUME Process 

ALT+CTRL+K : KILL Process

ALT+CTRL+I : KILL Window Manager Itself

ALT+CTRL+L : Show Logs

## Further description
- It should be running with elevated privileges.
- On the very first run, It asks for running on startup. In case of yes response, puts "WindowManager" key to "\HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" and sets it value to executable path.
- Checks any other instance running. 
- Works by setting a callback for mouse and keyboard events.
- Cancels mouse clicks to prevent misclicks while moving/resizing.
- Exits from full screen on move/resize events.
- Has a config file with 3 options

    1. dll: 32-bit dll's path (for dll injection feature)
    2. dll64: 64-bit dll's path (for dll injection feature)
    3. first_run: indicator of first run or not


## PREVIEW
![DEMO](demo.gif)