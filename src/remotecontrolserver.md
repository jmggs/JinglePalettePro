# RemoteControlServer

A minimal HTTP server for Qt/C++ that allows remote triggering of actions via browser (e.g., http://localhost:8000/01 for play, /stop, /pause, /automix, /autorepeat).

- Listens on a configurable port (default 8000)
- Can be enabled/disabled via checkbox
- Emits signals for each command
- Threaded to not block UI

## Usage
- Integrate with MainWindow to connect signals to play/stop/pause/etc.
- Add to settings dialog for configuration.
