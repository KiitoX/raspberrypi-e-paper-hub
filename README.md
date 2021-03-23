# Raspberry Pi Info Hub

## Hardware

This project uses a Raspberry Pi and a three colour 5.83" 648x480 e-paper display.

More specifically, this model: [ER-EPD0583-1R-5103](https://www.buydisplay.com/red-5-83-inch-e-paper-display-raspberry-pi-hat-e-ink-648x480) from EastRising, with an included Pi HAT.

I assume it can easily be swapped out for an equivalent model from another vendor using the same library code (e.g. WaveShare), or even one of different size, by replacing the `lib/epd/ER-EPD0583-1` files with the specific interface provided for your particular model, and then refitting the code in `main.c`.

## Third-party Licenses

### E-paper interface
The code in `lib/epd` is based on the example code from the vendor with a decent amount of modifications.  
Sadly it is provided without any license, but with a bit of research I found this [repository](https://github.com/waveshare/e-Paper/blob/master/RaspberryPi_JetsonNano/c/lib/) from WaveShare, which contains – beside support for other displays, code which is almost identical to the one used here.  
The only difference being parts of the (sadly undocumented) cryptic init sequence for the display.  
More importantly though, it is licensed under the MIT license. I'm gonna take that as reason enough to provide this code fully, with my modifications in this repository.

## Fonts

I've looked at a variety of bitmap fonts to find something that is to my liking, but this project supports any font in [`.bdf`](https://en.wikipedia.org/wiki/Glyph_Bitmap_Distribution_Format) format.  
I have included the fonts [Scientifica](https://github.com/NerdyPepper/scientifica) (11pt), [Kakwa](https://github.com/kakwa/kakwafont) (12pt), and [Cozette](https://github.com/slavfox/Cozette) (13pt) for convenience, with all three being licensed under the Open Font License.  
Further I can recommend:
* [Lode](https://github.com/hishamhm/lode-fonts/) 15pt, quite a bit of language support (though rtl is not supported in my code)
* [CtrlD](https://github.com/bjin/ctrld-font) 10pt, 13pt, 16pt
* [Creep](https://github.com/romeovs/creep) 12pt
* [Terminus](http://terminus-font.sourceforge.net/) 6pt to 16pt

For my bigger screen a lot of these smaller fonts are going to be used in 2x scale, but for a smaller screen you may well want to go for 1x.