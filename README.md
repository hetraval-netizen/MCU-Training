# MCU-Training

This Repo consists of all the exercises performed by Het Raval during his the Baremetal and Freertos training Program

# STM32CubeMX Project for STM32L476RG

This repository contains an STM32CubeMX generated project configured for the **NUCLEO-L476RG** development board. Follow the steps below to import, build, and run this project using **STM32CubeIDE**.

## Prerequisites

Before you begin, ensure you have installed:
* [STM32CubeIDE](https://st.com) (Version 1.10.0 or higher recommended)
* ST-LINK Drivers (typically bundled with STM32CubeIDE)

---

## 1. Import the Project into STM32CubeIDE

1. Launch **STM32CubeIDE**.
2. Select your preferred workspace directory and click **Launch**.
3. Click **File** in the top menu bar, then select **Import...**
4. Expand the **General** folder, select **Existing Projects into Workspace**, and click **Next**.
5. Choose **Select root directory** and click **Browse...**
6. Navigate to and select the root folder of this repository (where the `.project` and `.cproject` files are located).
7. Ensure the project is checked in the **Projects** list.
8. Leave **Copy projects into workspace** unchecked to work directly within your cloned repository folder.
9. Click **Finish**.

---

## 2. Build the Project

1. Select the imported project in the **Project Explorer** pane on the left.
2. Click the **Hammer icon** (Build) in the top toolbar, or press `Ctrl + B` (`Cmd + B` on Mac).
3. Monitor the **Console** tab at the bottom. 
4. Verify that the build completes successfully with `0 errors`. This generates the output `.elf`, `.bin`, and `.hex` files in the `Debug` or `Release` folder.

---

## 3. Connect the Hardware

1. Locate your **NUCLEO-L476RG** board.
2. Connect the board to your computer using a Mini-USB cable via the onboard ST-LINK debugger port.
3. Verify that the power LED (`PWR`) lights up and the ST-LINK LED begins flashing or turns steady.

---

## 4. Run the Project

1. Right-click the project name in the **Project Explorer**.
2. Select **Run As** > **STM32 Cortex-M C/C++ Application**.
3. If the *Edit Configuration* window appears, leave the default settings and click **OK** or **Run**.
4. STM32CubeIDE will flash the binary onto the STM32L476RG microcontroller.
5. Once flashing is complete, the program will execute automatically.

---

## Project Structure

* **Core/Src/**: Contains application source code (`main.c`, `stm32l4xx_it.c`, etc.).
* **Core/Inc/**: Contains application header files.
* **Drivers/**: Contains STM32L4 HAL and CMSIS peripheral drivers.
* **[ProjectName].ioc**: The original STM32CubeMX configuration file.

