# Remote Module

TL;DR: Keywish RF-NANO. With a button and [soft-latch circuit](#power-consumption).

## Technical challenges

### Size

TODO

### Power consumption

As this is a pocket device and should comfortably fit into pocket (see [#size](#size)), the space for battery is also very limited. However,
a lifetime of such device is very essential question of the design.  
It's quite clear that it's just not enough here to simply run `loop` and wait for button press when the communication would be initiated.
Running Arduino consumes quite a big current (~40 mAh) when doing nothing but _looping_ which it far too much when we need to power it from
~200 mAh battery (approx. capacity of common CR2032).  
There are [several ways](http://kratkadobapouziti.sweb.cz) how to reduce current consumption of running Arduino by removing (desoldering)
some unnecessary parts (LEDs, stabilizer, programmer) from it and you can quite easily get to power consumption around 15 mAh. Not great,
not terrible. Let's continue.  
Another way how to save some power is to let the Arduino sleep for some time (using
e.g. [LowPower library](https://github.com/rocketscream/Low-Power)) and wake it up only by timer or some HW interrupt. This might be the
way! Removing the parts from previous paragraph and using deep sleep, you can easily get to power consumption under 1 mA. Great!
Unfortunately, that still means only few days (say two weeks) of life which is... useless. Fortunately for this project, the Arduino doesn't
have to run at all for the most of time, if:

1. There exists a circuit able to start powering it on a button click and able to stop powering it when the Arduino itself decides (to cut
   its own power).
1. We learn the Arduino to start quickly enough to get some reasonable UX (who wants to wait few seconds before the remote control do its
   work after button press?)

The circuit really exists and is usually called a _soft latch_. You can learn how it works and how to create it on YouTube in
[many videos](https://www.youtube.com/watch?v=g1rbIG2BO0U). Only few components is needed.

As for the second problem, you may have noticed that Arduino start quite slowly. Usually you just don't care but in this project, this is
important for some user-friendliness. If we only knew what slows the Arduino startup...  
Again, thanks to some [YouTube videos](https://www.youtube.com/watch?v=lsa5h2tVM9w) we can learn not only that it's the _bootloader_ what
slows the startup but also how to hack it and make the startup almost instant. Arduino without the bootloader can't be programmed by just
sticking USB cable into it, so nothing friendly for development, but who cares about that for final setup? (it **can** be programmed, it's
just little bit more annoying)

Because the program itself ([radio exchange](#signal-protocol-and-security) between the remote
and [main module](https://alarm-garage.github.io/module-main/)) runs only a second, it doesn't matter that it consumes tens of mAh during
that. In other words, now we are ready to power the Arduino with just 200 mAh battery and it will last possibly for YEARS (cca 2500 button
clicks), even without the HW _adjustments_ (like removing the PWR diode). The consumption in during "sleep" is just **0** mAh.

### Signal reliability

Described in [main module](https://alarm-garage.github.io/module-main/#remote-switch-connectivity).

### Signal protocol and security

Described in [main module](https://alarm-garage.github.io/module-main/#remote-switch-protocol-and-security).
