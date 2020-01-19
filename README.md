# syncwatch-desktop

Syncs the position and pause state of a the video player between connected peers to watch videos together with friends. 
Works with magnet links and starts playback instantly. Other supported video sources can be found 
[here](http://ytdl-org.github.io/youtube-dl/supportedsites.html).
Requires [mpv](https://mpv.io/) and optionally [youtube-dl](https://youtube-dl.org/) to be installed on the system. 
[Windows installer](https://github.com/utkuce/syncwatch-desktop/releases/download/v0.5.6/syncwatch-0.5.6.exe) includes both.

## Build
`npm install` then
- `npm start` to run the app
- `npm run build` to create an executable and `makensis Syncwatch.nsi` to create a windows installer

## TODO
- [ ] Multiple peers in a room
- [ ] Mac and Linux installers
