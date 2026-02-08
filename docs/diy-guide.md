# DIY Guide

## :gear: Required Hardware

- Arduino UNO R4 (WiFi or Minima)
- This exact [LCD display](https://www.waveshare.com/1.69inch-lcd-module.htm)
- Arduino [input shield](https://thepihut.com/products/input-shield-for-arduino?variant=27740616849)
- Any USB-C power bank ([this one](https://www.amazon.co.uk/dp/B08T1JCXR5?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1) is compatible with the current version of the travel case)
- Any USB-C to USB-C cable (e.g. [this one](https://www.amazon.co.uk/dp/B0CN6CKDN5?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1))
- any 3D Printer to [print the enclosure](./3d-printing.md).
- minimum 8 8x3mm magnets (e.g. [those ones](https://www.amazon.co.uk/dp/B0CXF3R1MD?ref=ppx_yo2ov_dt_b_fed_asin_title))
- soldering iron (You will need to solder the LCD display wires onto the input shield).
- Small screws and washers to mount the display on the input shield.

## Steps

0. Ensure you have some space to work with.
   <p align="center">
   <img src="/assets/images/soldering-process.jpg"alt="" height="300px"/>
   </p>
1. Attach the LCD display to the input shield. For this you will need some screws and washers. See reference pictures below, the washers between the display and the input shield should be about 2mm thick to work with the case. For the ones at the back, use whatever is required to match your screws.
   <p align="center">
   <img src="/assets/images/washers.jpg"alt="" height="200px"/>
   <img src="/assets/images/bottom-washer-placement.jpg"alt="" height="200px"/>
   <img src="/assets/images/washer-placement-angled-view.jpg"alt="" height="200px"/>
   <img src="/assets/images/washer-placement-side-view-2.jpg"alt="" height="200px"/>
   <img src="/assets/images/washer-placement-side-view.jpg"alt="" height="200px"/>
   </p>

2. Before soldering, you will need to remove tips of the jumper wires. See pictures below on how to do this. Note that this is a permanent operation and it will be difficult to attach those back to your display.
   <p align="center">
   <img src="/assets/images/jumper-wire-before-tip-removal.jpg"alt="" height="200px"/>
   <img src="/assets/images/jumper-wire-during-tip-removal.jpg"alt="" height="200px"/>
   <img src="/assets/images/jumper-with-its-tip-half-removed.jpg"alt="" height="200px"/>
   <img src="/assets/images/jumper-with-plastic-tip-removed.jpg"alt="" height="200px"/>
   <img src="/assets/images/cutting-jumper-wire-metal-tip.jpg"alt="" height="200px"/>
   </p>
3. After the tips are removed, you need to thread the wires through the holes in the input shield like so:
   <p align="center">
   <img src="/assets/images/cable-threading.jpg"alt="" height="200px"/>
   <img src="/assets/images/cable-threading-2.jpg"alt="" height="200px"/>
   <img src="/assets/images/cable-threading-3.jpg"alt="" height="200px"/>
   <img src="/assets/images/wires-wrapped-in-groups-bottom-view.jpg"alt="" height="200px"/>
   </p>
4. Solder the LCD wires to the bottom pins of the input shield. This requires peeling off some of the rubber around each wire and tying the wire to the pin of the input shield. For the exact pin wiring, please refer to the code & pictures below (I know it is not ideal, a pinout guide is coming soon).
   <p align="center">
   <img src="/assets/images/bottom-wires-soldered.jpg"alt="" height="200px"/>
   <img src="/assets/images/wire-contact-before-soldering.jpg"alt="" height="200px"/>
   <img src="/assets/images/top-wires-soldered.jpg"alt="" height="200px"/>
   <img src="/assets/images/wire-contact-after-soldering.jpg"alt="" height="200px"/>
   </p>
5. After soldering the wires, you can put some tape over them to secure them in place.
   <p align="center">
   <img src="/assets/images/tape-over-soldered-wires.jpg"alt="" height="300px"/>
   <img src="/assets/images/wire-groups-with-tape-bottom-view.jpg"alt="" height="300px"/>
   </p>
6. Print out the parts of the MicroBox case.
   <p align="center">
   <img src="/assets/images/travel-case-disassembled.jpg"alt="" height="200px"/>
   <img src="/assets/images/inner-case-disassembled.jpg"alt="" height="200px"/>
   <img src="/assets/images/cyan-case-disassembled-side-view.jpg"alt="" height="200px"/>
   </p>
7. Glue magnets into the top part of the inner case.
   <p align="center">
   <img src="/assets/images/inner-case-top-part.jpg"alt="" height="300px"/>
   </p>
8. Insert magnets into the bottom part of the inner case (the fit is quite snug
   so you might need to use a screwdriver to push them in.)
   <p align="center">
   <img src="/assets/images/inner-case-bottom-part.jpg"alt="" height="299px"/>
   </p>
9. Put everything together and you should have a new MicroBox. When assembling the
   inner case, it is recommended to put a piece of styrofoam / sponge behing the
   button section of the input shield to ensure that the buttons don't sink
   into the case during use.


<p align="center">
<img src="/assets/images/two-boxes-slanted-main-view.jpg"alt="assembled-case" width="800px"/>
</p>
