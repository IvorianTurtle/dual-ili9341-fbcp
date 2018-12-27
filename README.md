# dual-ili9341-fbcp
=============================
A framebuffer copy for a dual-ili9341 screens setup

This is based on https://github.com/tasanakorn/rpi-fbcp
 
I modified it so that the Raspberry Pi could be used with dual ili9341 screen


                       320 px
          ******************************
          *                            *
          *                            *
          *                            *
          *      screen 1              *   240px
          *                            *
          *                            *
          *                            *
          ******************************
          *                            *
          *                            *
          *                            *
          *     screen 2               *  240px
          *                            *
          *                            *
          *                            *
          ******************************
          
The main screen is set as 320px by 480px. then the program copy the first 320 x 240 to framebuffer one and the second 320 x 240 to framebuffer two.

Tested with yocto on Raspberry Pi 0 
========================


# MORE DETAIL SOON 
