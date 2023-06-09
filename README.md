# Camera-based Photoelectric Barriers
This is the repository for the paper "A Novel Inexpensive Camera-based Photoelectric Barrier System for Accurate Flying Sprint Time Measurement". It shows how the build cost-effective photoelectric barriers based on consumer grade electronic components with an accuracy on par with professional time measurement systems. The repository contains building plans for the devices, software for the microcontroller, and the Android app required for controlling the system and display the time measurements.

<p align="center">
<table style="margin-left:auto;margin-right:auto">
    <tr>
        <td> <img src="Documentation/PhotoelectricBarrierPlaced.webp" alt="Photoelectric Barrier Placed" height="300px"/> </td>
        <td> <img src="Documentation/ScreenMeasuring.webp" alt="Screenshot of Android App" height="300px"/> </td>
    </tr>
</table>
</p>

## Building the Photoelectric Barriers
The following image shows the wiring schematics. The upper left part is the RGB-LED that indicates the current status of the barrier (standby, ready, etc.). The lower part shows the power supply including a voltage divider to measure the battery status. 
<p align="center">
    <img src="Documentation/PhotoelectricBarrierSchematic.webp" alt="Photoelectric Barriers Schematics" width="70%"/>
</p>

In order to build the photoelectric barriers you will need the depicted modules and connect them according to the schematics by soldering. 
The microcontroller software for the ESP32-Cam module is located under **DeviceDrivers** in this repository. As development environment we used Visual Studio Code and the <a href="https://platformio.org/">PlatformIO</a> plugin. Make sure you install the Espressive32 libraries.
Choose one of the *OpticalBarrier* environments and set in the *main.cpp* under *Barrier.init(CameraID)* the respective Camera ID, which determines the Bluetooth name of the device.


## Android Control App
The Android control app is located in **AndroidApp** and can be build using <a href="https://developer.android.com/studio">AndroidStudio</a>. 

## Acknowledgment 
<table style="margin-left:auto;margin-right:auto;">
    <tr>
        <td><img href="https://hybrid-societies.org/" src="Documentation/Logo_HybridSocieties.webp" height="150p"/></td>
        <td><img href="https://www.tu-chemnitz.de/" src="Documentation/LogoTUChemnitz.webp" height="150p" /></td>
    </tr>
</table>
This project was originally developed at Chemnitz University of Technology and funded by the Deutsche Forschungsgemeinschaft (DFG, German Research Foundation) – Project-ID 416228727 – CRC 1410.


## How to cite

	Uhlmann, T., Bräuer, S., Zaumseil, F., and Brunnett, G. (2023). A novel inexpensive camera-based photoelectric barrier system for accurate flying sprint time measurement. Technical Report CSR-23-04, Chemnitz University of Technology. https://nbn-resolving.org/urn:nbn:de:bsz:ch1-qucosa2-850240.
<br>

	@techreport{CBBarriers,
	  author          = {Tom Uhlmann and Sabrina Bräuer and Falk Zaumseil and Guido Brunnett},
	  journal         = {Chemnitzer Informatik-Berichte},
	  number          = {CSR-23-04},
	  title           = {A Novel Inexpensive Camera-based Photoelectric Barrier System for Accurate Flying Sprint Time Measurement},
	  year            = {2023},
	  institution     = {Chemnitz University of Technology},
	  note            = {https://nbn-resolving.org/urn:nbn:de:bsz:ch1-qucosa2-850240}
	}



