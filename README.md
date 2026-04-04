[简体中文](./README_cn.md)

## Qt TodoList
A lightweight TodoList built with Qt6, featuring a countdown timer and theme switching.


### Usage
#### Todo
- Click the Add button to create a new todo (Esc to cancel, Enter to confirm)
- Right-click a todo item to edit its content (Esc to cancel)
- Double-click to mark a todo as completed
- Long-press with the left mouse button to drag and reorder

![todo.gif](./docs/images/todo%20(1080p).gif)

#### Done
Completed items are sorted by completion time, and can be restored or deleted. The top button supports one-click deletion.

![done.gif](./docs/images/done%20(1080p).gif)

#### Timer
Countdown timer. You can click the digits to change the time. A reminder popup appears when the countdown ends.

![timer.gif](./docs/images/timer%20(1080p).gif)

#### Other Features
##### Theme Switch
Click the button to switch between dark/light mode.

##### Window Lock
Drag the top area of the window to move it. Resize from the window edges. After clicking the lock button, window position and size can no longer be changed, and mouse-click-through is enabled. Click again to unlock.

![features.gif](./docs/images/features%20(1080p).gif);

### Configuration
#### `config.json`

`borderRadius`: Window corner radius

`theme`: Theme mode
- `light`: Light mode (default)
- `dark`: Dark mode

`todoWrapMode`: Wrapping mode
- `force`: Force wrap when exceeding length (default)
- `word`: Wrap by words (very long words may be clipped)

`windowHeight`: Window height (default 600)

`windowWidth`: Window width (default 500)

`windowX`: Window x coordinate

`windowY`: Window y coordinate

`timerInitialSeconds`: Initial timer value

### Custom Build
- `Qt6.5.3+`
- `C++17`
