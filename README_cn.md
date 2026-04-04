## Qt TodoList

### 配置项
#### `config.json`

`borderRadius`: 窗口圆角大小

`theme`： 主题色
- `light`：亮色（默认）
- `dark`：暗色

`todoWrapMode`：换行方式
- `force`：超出长度强制换行（默认）
- `word`：按词换行（过长的词可能显示不全）

`windowHeight`: 窗口高度（默认600）

`windowWidth`: 窗口宽度（默认500）

`windowX`: 窗口x坐标

`windowY`: 窗口y坐标

`timerInitialSeconds`: Timer设置的时间

### 自定义构建
- `Qt6.5.3+`
- `C++17`

#### Windows
使用下面的命令进行Release构建
```ps
cmake -S . -B release
cmake --build release --config Release
# cmake --install release --config Release --prefix "./release"
# cd ./release/bin
windeployqt todolist.exe
```