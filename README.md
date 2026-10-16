# STM32F103C8T6 + MPU6050 姿态解算 / OLED 显示 / 串口通信

## 1. 项目简介

基于 STM32F103C8T6 的 IMU 姿态测量小系统，运行 FreeRTOS（CMSIS-RTOS2 封装）。

主要功能：

- 通过 I2C2 读取 MPU6050 的加速度计 / 陀螺仪原始数据（200 Hz）；
- 用「加速度计解算角度 + 陀螺仪积分」的**二维状态卡尔曼滤波**解算 Roll / Pitch，Yaw 由陀螺仪积分得到；
- 通过 I2C1（DMA）在 0.96 寸 SSD1306 OLED 上显示欧拉角；
- 通过 USART1（DMA + IDLE 空闲中断）接收数据，收到以 `\n` 结尾的一行后原样回显。

开发方式：STM32CubeMX 生成 HAL + FreeRTOS 框架，用户代码放在 `User/` 目录，用 VSCode(EIDE) 开发和编译、Keil MDK调试。

---

## 2. 硬件清单与芯片型号

| 器件 | 型号 / 规格 | 说明 |
| --- | --- | --- |
| 主控 MCU | STM32F103C8T6 | Cortex-M3，72 MHz，64 KB Flash，20 KB SRAM，LQFP48 |
| 六轴 IMU | MPU6050 | 3 轴加速度计 + 3 轴陀螺仪，I2C 接口，7 位地址 `0x68` |
| 显示屏 | 0.96 寸 OLED | SSD1306 驱动，128×64，I2C 接口，7 位地址 `0x3C` |
| 调试器 | ST-Link | SWD 调试 / 下载 |
| 串口工具 | USB-TTL + 串口助手 / VOFA+ | 115200 8N1 |

---

## 3. 引脚接线表

### 3.1 电源

| 网络 | 说明 |
| --- | --- |
| 3V3 | STM32、MPU6050、OLED、USB-TTL(3.3V 侧) 共用 |
| GND | **所有器件必须共地**（含 USB-TTL、ST-Link） |

### 3.2 STM32F103C8T6 引脚占用

| MCU 引脚 | 功能 | 连接对象 |
| --- | --- | --- |
| PA9 | USART1_TX | USB-TTL 的 RX |
| PA10 | USART1_RX | USB-TTL 的 TX |
| PB6 | I2C1_SCL | OLED 的 SCL |
| PB7 | I2C1_SDA | OLED 的 SDA |
| PB10 | I2C2_SCL | MPU6050 的 SCL |
| PB11 | I2C2_SDA | MPU6050 的 SDA |
| PA13 | SWDIO | ST-Link 的 SWDIO |
| PA14 | SWCLK | ST-Link 的 SWCLK |
| PD0 / PD1 | OSC_IN / OSC_OUT | 8 MHz 晶振（HSE，板载） |
| PC14 / PC15 | OSC32_IN / OSC32_OUT | 32.768 kHz 晶振（LSE，本工程未用） |
| 3V3 / GND | 电源 / 地 | 全部器件 |

### 3.3 模块接线

**MPU6050（GY-521，I2C2）**

| MPU6050 | STM32 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SCL | PB10 |
| SDA | PB11 |
| AD0 | GND（决定地址为 0x68） |
| INT / XDA / XCL | 不接 |

**0.96" OLED（SSD1306，I2C1）**

| OLED | STM32 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SCL | PB6 |
| SDA | PB7 |

**USB-TTL（接电脑，USART1，115200 8N1）**

| USB-TTL | STM32 |
| --- | --- |
| TXD | PA10（MCU 的 RX） |
| RXD | PA9（MCU 的 TX） |
| GND | GND |
| 3V3 | 仅在没有其它供电时接 |

**ST-Link（SWD 下载调试）**

| ST-Link | STM32 |
| --- | --- |
| SWDIO | PA13 |
| SWCLK | PA14 |
| GND | GND |
| 3.3V | 3V3（供电，可选） |

---

## 4. CubeMX 配置说明

### 4.1 芯片与时钟

| 项 | 配置 |
| --- | --- |
| MCU | STM32F103C8Tx（LQFP48，Cortex-M3，64 KB Flash / 20 KB SRAM） |
| RCC | HSE 8 MHz 外部晶振；PLL 源 = HSE，PLLMUL = ×9 |
| SYSCLK / HCLK | 72 MHz |
| APB1 / APB2 | 36 MHz（HCLK÷2）/ 72 MHz |
| SYS → Debug | Serial Wire（SWD，PA13/PA14） |
| SYS → Timebase Source | **TIM4**（SysTick 留给 FreeRTOS） |
| NVIC Priority Group | 4 |

### 4.2 外设配置

| 外设 | 模式 / 参数 | 引脚 | DMA | NVIC（优先级） |
| --- | --- | --- | --- | --- |
| USART1 | Asynchronous，115200 bit/s，8-N-1，无硬件流控 | PA9 = TX，PA10 = RX | TX：DMA1_Channel4（Normal）；RX：DMA1_Channel5（**Circular**） | USART1_IRQn(5)、DMA1_Channel4_IRQn(5)、DMA1_Channel5_IRQn(5) |
| I2C1 | **Fast Mode（400 kHz）**，7 位地址 | PB6 = SCL，PB7 = SDA | TX：DMA1_Channel6（Normal） | I2C1_EV_IRQn(5) |
| I2C2 | **Fast Mode（400 kHz）**，7 位地址 | PB10 = SCL，PB11 = SDA | 无（中断方式读） | I2C2_EV_IRQn(5) |

### 4.3 FreeRTOS（CMSIS-RTOS2，V2）

- **任务**：仅配置 `Init`（优先级 `osPriorityRealtime`，栈 128 words）；其余 4 个任务由 `Task_Init` 在运行时用 `xTaskCreate` 创建（详见第 5 节）。
- **二值信号量**：`UART_DMA_Lock`、`I2C1_DMA_Lock`、`I2C2_IT_Lock`（初始可用）、`RawDataReady`（初始为空）。
- **队列**：`UART_Receive_Size`，长度 8，元素 `uint16_t`。
- **堆**：`FREERTOS.configTOTAL_HEAP_SIZE = 8192`（字节）。

### 4.4 引脚分配

| 引脚 | 信号 |
| --- | --- |
| PA9 / PA10 | USART1_TX / USART1_RX |
| PB6 / PB7 | I2C1_SCL / I2C1_SDA |
| PB10 / PB11 | I2C2_SCL / I2C2_SDA |
| PA13 / PA14 | SYS_JTMS-SWDIO / SYS_JTCK-SWCLK |
| PD0 / PD1 | RCC_OSC_IN / RCC_OSC_OUT（HSE 8 MHz） |


---


## 5. FreeRTOS 任务结构

| 任务 | 创建方式 / 位置 | 栈 | 优先级 | 职责 |
| --- | --- | --- | --- | --- |
| `Init` | `osThreadNew`，`MX_FREERTOS_Init()` | 128 | `osPriorityRealtime`(48) | 初始化串口接收/DMA、OLED、MPU6050 并校准；创建其余任务；最后 `vTaskDelete(NULL)` |
| `UART_Send` | `xTaskCreate`，`Task_Init` 末尾 | 128 | `osPriorityNormal`(24) | 串口回显 |
| `MPU6050_Read` | `xTaskCreate`，`Task_Init` 末尾 | 128 | `osPriorityHigh`(40) | 200 Hz 采样 MPU6050，发出 `RawDataReady` |
| `MPU6050_Handle` | `xTaskCreate`，`Task_Init` 末尾 | 128 | `osPriorityHigh`(40) | 卡尔曼姿态解算，更新 `EulerAngles` |
| `OLED_ShowData` | `xTaskCreate`，`Task_Init` 末尾 | 128 | `osPriorityLow`(8) | 50 Hz 刷新 OLED |
| `IDLE` / `Tmr Svc` | 内核自动创建 | 静态 | 0 / 2 | 系统任务 |

设计要点：

- **初始化期隔离**：除 `Init` 外的任务都由 `Task_Init` 在初始化完成后才创建，保证 `Init` 完成前没有其它业务任务运行（避免初始化与采样并发访问 I2C2 等）。
- **中断驱动 + 信号量**：I2C/USART/DMA 都在中断回调里 `Give` 对应信号量/队列，任务阻塞等待，从而让出 CPU。

任务间同步对象：

| 对象 | 类型 | 作用 |
| --- | --- | --- |
| `UART_DMA_Lock` | 二值信号量 | 串口 TX DMA 完成标志 |
| `I2C1_DMA_Lock` | 二值信号量 | OLED TX DMA 完成标志 |
| `I2C2_IT_Lock` | 二值信号量 | MPU6050 中断读完成标志 |
| `RawDataReady` | 二值信号量 | 新采样就绪，通知解算任务 |
| `UART_Receive_Size` | 队列（8×u16） | 空闲中断上报的累计接收长度 |

---

## 6. 代码目录说明

```
Project/
├─ Core/                       CubeMX 生成代码
│  ├─ Inc/  main.h, FreeRTOSConfig.h, usart.h, i2c.h, dma.h, gpio.h ...
│  └─ Src/  main.c, freertos.c, usart.c, i2c.c, dma.c, gpio.c,
│           stm32f1xx_it.c, stm32f1xx_hal_msp.c, stm32f1xx_hal_timebase_tim.c
├─ User/                       用户代码
│  ├─ Inc/  UART.h, MPU6050.h, OLED.h, OLED_D.h
│  └─ Src/  Init.c, UART.c, MPU6050.c, OLED.c, OLED_D.c
├─ Drivers/                    STM32F1 HAL 驱动 + CMSIS
├─ Middlewares/                FreeRTOS 内核 + CMSIS-RTOS2 封装
├─ MDK-ARM/                    Keil MDK 工程（Project.uvprojx）
├─ build/                      EIDE(VSCode) 编译输出
├─ .eide/ .vscode/             EIDE 工程与任务配置
└─ Project.ioc                 CubeMX 工程文件
```

各用户文件职责：

- `User/Src/Init.c`：`Task_Init`，系统初始化 + 创建其余任务。
- `User/Src/UART.c`：串口回显任务、串口接收/发送中断回调。
- `User/Src/MPU6050.c`：MPU6050 读写、零偏校准、卡尔曼滤波、采样与解算任务。
- `User/Src/OLED.c` / `OLED_D.c`：SSD1306 驱动与 8×16 字库。

---

## 7. 串口通信收发处理流程

USART1，115200-8-N-1。

### 接收（RX）

1. `Task_Init` 中调用 `HAL_UARTEx_ReceiveToIdle_DMA(&huart1, ReceiveBuf, 64)`：以**循环 DMA** 方式接收，并开启 IDLE 空闲中断；随后用 `__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT)` 关掉半传输中断。
2. 收到数据后总线空闲（或 DMA 传满）触发 `HAL_UARTEx_RxEventCallback`，回调把 **累计接收长度** `Size` 通过 `UART_Receive_Size` 队列发给回显任务。
3. 由于是循环 DMA，回调里的 `Size` 是「缓冲区内的累计位置」而非本次长度，任务端用 `ReceiveBufPointer` 做差值还原本次新增数据。

### 发送（TX）

1. `Task_UART_Send` 阻塞等待队列 `UART_Receive_Size`；
2. 取到长度后获取 `UART_DMA_Lock`；
3. 判断 `ReceiveBuf[Size-1] == '\n'`：
   - 成立：把本次新增数据拷到 `SendBuf`，调用 `HAL_UART_Transmit_DMA` 回显，锁在 `HAL_UART_TxCpltCallback` 中释放；
   - 不成立：直接释放锁，等待下一次。
4. `HAL_UART_TxCpltCallback` 在发送完成中断里释放 `UART_DMA_Lock`，实现两次发送之间的串行化。

> 使用串口助手 / VOFA+ 时，请在发送设置里**追加 `\n`（LF）**，否则不会触发回显。

---

## 8. IMU 型号、接口、校准与姿态解算方法

### 型号与接口

- 型号：MPU6050（7 位地址 `0x68`）
- 接口：I2C2（PB10/PB11），400 kHz，**中断方式读取**（`HAL_I2C_Mem_Read_IT`）
- 一次读取 14 字节：0x3B 起连续读加速度(6) + 温度(2) + 陀螺仪(6)

### 初始化（`MPU6050_Init`）

| 寄存器 | 值 | 含义 |
| --- | --- | --- |
| 0x6B | 0x80 | 复位 |
| 0x6B | 0x00 | 退出睡眠 |
| 0x1B | 0x18 | 陀螺量程 ±2000 °/s |
| 0x1C | 0x00 | 加速度量程 ±2 g |

换算系数：加速度 `6.1035e-5 g/LSB`（1/16384），角速度 `6.1035e-2 (°/s)/LSB`（1/16.4）。

> 目前未配置采样率分频寄存器 `0x19` 与数字低通 `0x1A`（DLPF）。

### 校准（`MPU6050_Zero`）

上电后静置采样 **1000 次**角速度求平均，得到三轴陀螺零偏：

- `RollKalman.ZeroBias`、`PitchKalman.ZeroBias` 作为卡尔曼状态中的零偏初值；
- `YawZeroBias` 用于 Yaw 积分去除零偏。

### 姿态解算

采用经典的**二维状态卡尔曼滤波**，Roll、Pitch 各跑一个独立实例：

- 状态：`x = [角度, 陀螺零偏]ᵀ`（2 维状态，故 P 为 2×2，K 为 2×1）
- 预测：`角度 ← 角度 + (角速度 − 零偏)·dt`
- 观测：加速度计解算角度
  - `Roll_acc  = atan2(ay, az)`
  - `Pitch_acc = atan2(ax, √(ay²+az²))`
- 参数：`RatelNoise(Q_angle)=0.001`、`BiasNoise(Q_bias)=0.003`、`MeasureNoise(R)=0.5`
- Yaw：无绝对参考，直接对 `(gz − YawZeroBias)` 积分

采样节拍：`DT = 0.005 s`（标称 200 Hz），`MPU6050_Read` 任务用 `vTaskDelayUntil` 维持该周期。

---

## 9. OLED 显示说明

- 型号：0.96 寸 SSD1306（128×64），7 位地址 `0x3C`；接口 I2C1（PB6/PB7），400kHz，**DMA 发送**。
- 字库：`OLED_D.c` 中的 `OLED_F8x16`，8×16 点阵，一行 16 字符、共 4 行。
- 底层：`OLED_WriteCommand` / `OLED_WriteData` 每次发送 2 字节，用 `I2C1_DMA_Lock` 串行化，发送完成由 `HAL_I2C_MasterTxCpltCallback` 解锁。

`Task_OLED_ShowData`（50 Hz）：

- 从 `EulerAngles` 取 Roll / Pitch / Yaw；
- 行 1 显示标题，行 2/3/4 分别显示 `Roll:` / `Pitch:` / `Yaw:`，数值显示在第 9~16 列。

---

## 10. 编译烧录步骤

本工程同时存在两套工具链，github里只上传了Keil MDK的配置，避免改了代码但跑的是另一份固件。

### 方式 A：Keil MDK

1. 打开 `MDK-ARM/Project.uvprojx`；
2. **Rebuild**（改过头文件后建议全量重编，不要只用 Build）；
3. `Download`（ST-Link）烧录；

### 方式 B：VSCode + EIDE

1. 用 VSCode 打开工程；
2. 运行任务 `build`（或 `rebuild`）；产物在 `build/Project/`；
3. 运行任务 `flash` / `build and flash`（上传器为 OpenOCD + ST-Link）。

---

## 11. 遇到的问题与解决方法

### 11.1 FreeRTOS 堆不足，任务创建静默失败导致"任务不运行"

- **现象**：`Task_OLED_ShowData` 一直不运行；`uxCurrentNumberOfTasks` 比预期少。
- **原因**：`configTOTAL_HEAP_SIZE = 3072`，而 `Task_Init` 运行时要创建 4 个任务（每个约 512 B 栈 + 92 B TCB + 开销 ≈ 620 B），堆不够；`xTaskCreate` 返回失败但**未检查返回值**，任务被静默丢弃。由于堆只减不增，失败的必然是**最后创建**的那个任务。
- **解决**：把 `configTOTAL_HEAP_SIZE` 调到 **8192**；建议给每个 `xTaskCreate` 加返回值判断（失败进 `Error_Handler`）。

### 11.2 代码改了但"没生效"

- **现象**：`FreeRTOSConfig.h` 已改成 8192，但运行时仍是旧行为。
- **原因**：Keil 与 EIDE 两套产物并存，调试用的固件是**修改之前**编译的那一份。
- **解决**：确认编译产物时间晚于源码修改时间；固定一套工具链编译 + 下载 + 调试。

### 11.3 OLED 数值不显示

- **现象**：OLED 上 `Roll:/Pitch:/Yaw:` 标签能显示，后面的数字始终空白。
- **原因**：`OLED_ShowAngle` 从缓冲区末尾（下标 7）向左填充，前导字节仍是 `'\0'`；而 `OLED_ShowString` 从下标 0 开始、遇到 `'\0'` 立即结束 → 什么都没打印。只有格式化结果正好 8 个字符（如 ±100.000）时才会显示。
- **建议解决**：缓冲区改为 9 字节并预填空格（`uint8_t ShowString[9] = "        ";`），末尾补 `'\0'`；同时修正负数分支里 `(uint8_t)Number` 的整型转换（负数会得到错误数值）。

### 11.4 MPU6050 中断读的并发死锁

- **现象**：上电发一段数据后卡死。
- **原因**：`MPU6050_ReadData` 中先 `xSemaphoreTake(..., 0)`、读完后 `xSemaphoreTake(..., portMAX_DELAY)`，把同一个信号量既当锁又当完成通知。当 `Task_Init`（`MPU6050_Zero`）与采样任务并发访问时，`HAL_I2C_Mem_Read_IT` 会因 `hi2c2.State` 非 READY 直接返回 BUSY（没有真正启动传输），而之后又永久等待回调给出信号量 → 采样任务永久阻塞，`RawDataReady` 再也不发。
- **解决**：所有工作任务不在CubeMX中配置，而是在Task_Init末尾通过xTaskCreate创建，防止`MPU6050_ReadData`和`MPU6050_Zero`同时运行。

### 11.5 串口发送较长数据时分多次回显

- **现象**：PC 端回显的数据不是完整的一行，而是被拆成几段，例如把 `Hello World!\n` 回显成 `Hello Wo` 和 `rld!` 两段（此时已无明显乱码）。
- **原因**：这是典型的**分包**现象。`HAL_UARTEx_ReceiveToIdle_DMA` 按**总线空闲（IDLE）**断帧，只要两段数据之间出现空闲就会触发一次回调；而 PC 端经 USB 转串口芯片（如 CH340）按 **1 ms 的 USB 帧**推送，较长的一串数据会跨帧、两帧之间存在空隙（上位机分多次写入、调试打印也一样）→ STM32 在空隙处提前触发 IDLE，把"半包"当成一包上报。
- **解决：按帧尾 `\n` 断帧**
  - `Task_UART_Send` 取到长度后，用 `ReceiveBuf[Size - 1] == '\n'` 判断一帧是否结束：
    - **未收到 `\n`**：直接释放 `UART_DMA_Lock` 并丢弃这次上报，继续等待后续数据（`ReceiveBufPointer` 不动，数据仍在缓冲区里累积）；
    - **收到 `\n`**：把从 `ReceiveBufPointer` 到当前 `Size` 之间的**本次新增数据**一次性拷到 `SendBuf`，再调用 `HAL_UART_Transmit_DMA` 整包回显；发送完成后由 `HAL_UART_TxCpltCallback` 释放 `UART_DMA_Lock`。
  - 因为**在收到 `\n` 之前不会发送任何数据**，所以即使硬件把一帧拆成多次（多个 IDLE），也会被合并成一条完整的回显。
  - **使用要求**：上位机（VOFA+ / 串口助手）发送时需在设置里**追加 `\n`**；单帧长度不要超过 `UART_BUFFER_LENGTH`（64 字节）。
