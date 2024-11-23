

# InkCal

If you are into the micro-checkpoint lifestyle, then this tool is for you.
This is an e-calendar to show your Google calendar data for the day. It can sit on your desk and tell your next upcoming event anytime.

## Features
 - Able to display: invited and your own calendar events (all-day and short events)
 - It updates every day 5 mins after midnight
 - It runs for months with a single charge

##  Hardware
- [Inkplate 5](https://soldered.com/product/soldered-inkplate-5-gen2/). You can buy the board with 3D-printed enclosure and battery as well, so you do not have to thinker. It will probably work with other  InkPlate products as well, but you may need to adjust the UI.

## How It Works
TODO add images and description

## Limitations
- If is only able to display english characters because of the restriction of the [Adafruit GFX library](https://github.com/adafruit/Adafruit-GFX-Library). Drop me a message if it is solved and I look into it

## How to set it up
1. Use the 'calendarHelper.gs' file to set up a new Google App Script project
2. Copy the content of the the script to a new [GoogleScript](https://script.google.com/) project 
3. Adjust the parameters at the top of the script
4. Add "Calendar" service to the project. The editor may ask for authorization -> Allow
5. Deploy the project. Create a real deployment, not just a 'test deployment'
6. Copy the trigger URL after the deployment. It should appear after a successful deployment
7. Test the request from a browser or other API testing tool, like Postman -> You should see your calendar events for the day with the correct timestamp
8. Paste the URL to the NetworkConfig.h
9. Open the InkCal.ino with Android studio
10. Plug your Inkplate5 via USB and upload the code
11. Test it and adjust the config if needed
12. Enjoy

## Dependencies
- InkplateLibrary@10.0.0

## TODO
- dynamic update based on current time -> Account time can be used from the response

## Arduino IDE setup and common mistakes
- To find out the correct COM port just check the Device Manager in Windows -> Ports -> unplug and plug your Inkplate
