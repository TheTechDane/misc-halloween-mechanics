# Small Halloween projects and tests
This is for a collection of smaller light and animatronics projects for halloween, I might split more in the future, now it is just to collect my early tests and projects in one place.

They are mainly based on ESP32-C3 Supermini as it is the cheapest ESP32 I could fine here is a good reference:https://www.espboards.dev/esp32/esp32-c3-super-mini/
You can buy the many places I got them on Aliexpress https://www.aliexpress.com/.

### Table of Contents
1. [Monster eye](#Monster-eye)
2. [T-Rex in the distance](#T-Rex-in-the-distance)
3. [Spooky eyes](#Spooky-eyes)



## Monster eye
Early start on a Eye that will be part of a Jurassic Park Themes installation 

<video controls src="images/DragonEye.mp4" title="Title"></video>
Artifacts:
- The simple ESP32 code [Sketch](Dragon_Eye/Dragon_Eye.ino)
- 3-D Model on [Maker World](https://makerworld.com/en/models/1428108-dragon-eye-light-for-maker-s-supply-puck-light#profileId-1484523)

## T-Rex in the distance
My first ever full Animatronics A T-Rex appears in a flashlight on occasions and makes sounds when it appears - while not appearing it plays some Jurassic Park alike sounds.
![alt text](images/t-Rex-Shadow.png)


It has 3 modes:
- **idle:** no flashlight and background sounds
- **Peak:** the t-rex appears in a flash for 2 seconds on a random interval between 30 and 60 seconds.
- **Walk-by:** The flashlight switches on at the t-rex slowly passes by in the light (5 seconds) on a random interval between 20 and 50 seconds.

More details here: [T-Rex Readme](Full-Jurassic-park-anamatronic/Readme.md)

## Spooky eyes
A stand alone set of eyes on AA batteries , with various light scenes to make it more lively.![alt text](<Spooky eyes/image.png>)

Automatically cycles through 4 scenes:

- Breathe Scene: 10 seconds (10% to 100% brightness, 2s per pulse)
- Solid Green: 10 seconds (100% brightness)
- Breathe Scene: 10 seconds (10% to 100% brightness, 2s per pulse)
- Off (Dark): 15 seconds (Strip completely dark)

See more details here: [Spooky Eyes Readme](<Spooky eyes/README.md>)