## Hex版本

### 20230508

- SAU-G031G8U6_Nothing

单独测试主板用（支持OLED、按键、加速度计唤醒、电池电压测量），可以不接扩展板

### 20230509

- SAU-G031G8U6_GL：扩展板是PCF8563用
- SAU-G031G8U6_PL：扩展板是PCF2129用

实现所有基础功能

### 20240407

- SAU-G031G8U6_GL：扩展板是PCF8563用
- SAU-G031G8U6_PL：扩展板是PCF2129用

BUG修复：

- 菜单-->数据采集-->删除任务：进入该页面后，即使按取消，还是会删除任务

修改：

- 菜单-->数据采集-->删除任务：进入该页面后，按确认，删除任务后，菜单-->数据采集-->任务设置-->采集对象-->温度、湿度还是为开启的。菜单-->辅助功能-->恢复出厂，点确认后。采集对象-->温度、湿度还是为开启的，不需要每次都手动开启

## 20240410

BUG修复：

- 菜单-->数据采集-->任务进度：待采数量的计算问题

```c
void columsDataCollected_Schedule(){
...
	drawNumber((OLED_WIDTH - numXOffset) - places * 6, y + 32,
			systemSto.data.NumDataWillCollect - systemSto.data.NumDataCollected.uint_16, places);
...
}
```

