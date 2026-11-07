# 测试数据

放 CSV 回放数据，建议三类：

| 文件 | 场景 | 用途 |
| --- | --- | --- |
| `walk_100.csv` | 平地走 100 步 | 计步误计/漏计 |
| `static_10min.csv` | 静止 | 误计与零漂 |
| `sit_45min.csv` | 静坐 | 久坐触发时间 |

IMU CSV 表头：

```csv
ts_us,ax,ay,az,gx,gy,gz
1000000,0.01,0.02,-0.98,0.1,0.0,0.2
```

分钟记录 CSV 表头（给 `tools/csv_to_bin.py`）：

```csv
ts,steps,hr,spo2,temp_x100,sit_min,flags
1761091200,62,78,97,3650,45,1
```
