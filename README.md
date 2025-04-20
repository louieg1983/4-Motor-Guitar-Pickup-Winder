# 4-Motor-Guitar-Pickup-Winder

This project uses 4 stepper motors to wind guitar pickups. 

How it works:

There are two motors that will turn the bobbins and there are two motors that serve as traverse motors. The code has Winder Motor 2 and Traverse Motor 2 mirror the main motors. You can edit the code to control the motors individually if you would like but my goal was to wind a set of humbuckers in one shot to save time and resources. 

All motors are NEMA 17 stepper Motors. They are being driven by TB6600 Stepper Motor Drivers. The Winder Motors are set to 32 micro steps while the Traverse Motors are set to 16 micro steps. All are being powered by a 36V 10A Switch Power Supply. So far there have been no issues with this configuration. 

The TB6600 motors are then connected to the pins on the ESP32 board. You can use an Arduino board if you would like. You will just need to edit the code accroding to the pins that you are using. 

I built a wire feeder using Tinkercad. I can provide that STL file as well. You can also design your own if you would like. So far, no issues at all. Though I would like to come up with a more elegant solution. At the moment, I am using two rollers, custom made tensioner (two metal washers with felt washers, nylon screws and nuts to hold everything to the wire feeder plate, and two Neodymium magnets with felt pads. The theory or thought process was to use the washers with felt to keep the wire from bouncing all over the place and have some tension and then have the magnets keep the constant tension as the wire is feed to the bobbin. 

Amazon links for the parts used:


4 - https://www.amazon.com/dp/B07PQ5KNKR?ref=ppx_yo2ov_dt_b_fed_asin_title

2 - https://www.amazon.com/dp/B09KRJDRVH?ref=ppx_yo2ov_dt_b_fed_asin_title 

2- https://www.amazon.com/dp/B00PNEQKC0?ref=ppx_yo2ov_dt_b_fed_asin_title

1 - https://www.amazon.com/dp/B01EV70C78?ref=ppx_yo2ov_dt_b_fed_asin_title 

1 - https://www.amazon.com/dp/B08H292YZV?ref=ppx_yo2ov_dt_b_fed_asin_title 

1 - https://www.amazon.com/dp/B097R2JMJT?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1 

1- https://www.amazon.com/dp/B0CLK7J4NB?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1 

1- https://www.amazon.com/dp/B0B82BBKCY?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1 

1 - https://www.amazon.com/dp/B082TQ11P2?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1 

1 - https://www.amazon.com/dp/B01NBE2XIR?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_1  

Total cost is below $200. Great deal considering CNC Guitar Pickup Winders are about 6 to 7 times more. I could be wrong, I no math goods. 

I used the videos as well as resrouces from other builders. I drew a lot of inspiration from all these resources. Lots of great stuff out there. Hopefully you find something that you click with and you can use this design to build your own design. If you do, please do me the honors and share your work of art with me. I would love to see if this helps!

Resources/Inspirations/3D Parts

https://github.com/sandy9159/DIY-Arduino-based-Guitar-Pickup-Coil-Winder

https://github.com/fps/OPWF 

https://github.com/speccyfan/wm/blob/master/wm1.1/stl/bracket-right.stl  

https://www.youtube.com/watch?v=pJiwjPVOXLA  

https://www.youtube.com/watch?v=IrS8BBjuDdw  

https://www.youtube.com/watch?v=xya4ipu7bTk

https://www.youtube.com/watch?v=YcfYJsYc00k

https://www.youtube.com/watch?v=phCA-h2uJGQ&t=526s

https://www.youtube.com/shorts/mcej-a_-2Q8

https://www.youtube.com/watch?v=2DeUdFgyJEU

https://www.youtube.com/watch?v=DuSMk63nonc&t=84s   

Also, if you do not have a 3D printer you may want to invest in a cheap one just to print out the parts otherwise you will need to buy something comparable or you can enlist the services of someone with a 3D printer. 

I hope this helps! And HAVE FUN!
