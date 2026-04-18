# Alkaline-Refresher_Web-UI
My own fork of alkaline-refresher standalone, but with Web-UI and extended functionality.

If you have built an alkaline-refresher for AA and AAA alkaline batteries, you almost have 80% of the Arduino sketch, you need. There are no changes in hardware (see KiCad-link an the end of this article). If you are interested in a PCB, please send me an email: michaelklein495(at)gmail.com. I'm offering PCBs and kits for makers, hobbyists and HAM radio friends (non commercial).

Here you will find the three .ino-Files you need. 


**How to finalize the sketch**
The sketch needs your WLAN SSID and your PASSWORD to hook up to your WLAN and deliver a WebSite using the mDNS: "alkafresh_1.local". Feel free to change the mDNS,if you like.
When the MCU connects to your WLAN, you get a serial message that includes the IP-address, that the DHCP-server gave to the Alkaline-Refresher. Some users reported, that with their Android cellular phones, they could not connect to "alkafresh_1.local". They are using the IP-address (port 80). There is no https provided at the moment.


**How does it work?**

1) Power up and start your Alkaline-Refresher and place batteries for refresh 1 ... 4.
2) You see the LEDs flashing one after the other. That shows, that there is a refreshing pulse going through each battery.
3) After the sketch has connected your project to your WLAN, you will find the website of the project at "alkafresh_1.local".
4) You see the actual voltage of each battery, numbered 1 ... 4. You may refresh 1,2,3 oder 4 batteries at the same time.
5) After quite a short time you realize, that the voltage of each battery is increasing. As soon, as the voltage has hit 1.6 v for 10 times, this battery is finally refreshed and the process stops.
6) If a battery is really bad, you see that the voltage does not move up. There is a 2h time out for this batteries. The voltage, at which the refreshing process starts, says little about the quality of the battery. We had batteries, that showed 0.5V initially. After refreshing, some of the are working now for 12 months.
7) As soon as refresh is finished, you get a second table on you Web-UI. It shows the battery number, the last voltage before refresh was finished and the voltage at the moment. The rightmost column shows you, how many minutes ago refresh was finished. This gives you a hint, how much charge the battery is loosing without load. It is a measure for the inner resistance of the battery, which is the only useful information to find out how good the battery will be working in the future. This table is for 60 min. After 60 min it updates the battery voltages again and the process continues for another 60 min period.
