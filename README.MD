# InkCal

If you are into the micro-checkpoint lifestyle, then this tool is for you.
This is a dynamic e-calendar to show your Google calendar data. It can sit on your desk and tell your next upcoming event anytime.

## Features
TODO add list

##  Hardware
- [Inkplate 5](https://soldered.com/product/soldered-inkplate-5-5-2%E2%80%B3-e-paper-board/). You can buy the board with 3D-printed enclosure and battery as well, so you do not have to thinker.

## How It Works
TODO add images and description

## How to set it up
1. Set up a new Google App Script project
2. Copy the script from calendarHelper.gs
3. Add "Calendar" service to the project. The editor may ask for authorization
4. Deploy the project
5. Copy the trigger URL after the deployment. It should appear after a successful deployment
6. Paste it to the NetworkConfig.h
7. Open the InkCal.ino with Android studio
8. Plug your Inkplate5 via USB and upload the code
9. Test it and adjust the config if needed
10. Enjoy