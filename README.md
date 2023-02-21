# ZAM PCS Textecke

The [ZAM](https://betreiberverein.de/?doing_wp_cron=1649430713.6483170986175537109375) Post Corona Stadt (PCS) Textecke project is an idea to engage the passer by to leave a message.

Further deatils can be found on the project page (German) [41 Textecke](https://wiki.betreiberverein.de/books/projekte-aktuell/page/41-textecke-639)

# Python Screen Marquee Effect
Since revision 1. was not working the way it was intended to, an alternative was set up to run a Python Marquee type program on the screen only. The usage has changed somewhat from the original concept. The is now an input field at the bottom of the screen where the user can enter the text and send it off by pressing the Enter key. There is now no more a seperate window for the entering of the text.

## Setup

The <nodemcu> should be substitued with the IP address of the NodeMCU unit to receive the messages.

```shell
git clone ...
cd ZAM_PCS_Textecke
cd src/python
python3 -m venv .venv
pip install -r requirements.txt

export REST_URL=http://<nodemcu>
.venv/bin/python marquee.py
```

If using this on Windows i.e. Powershell, the set the REST_URL with

```shell
$Env:REST_URL = "http://<nodemcu>"
```

## Usasge
Once started, a default message will show. This message will continually scroll across the screen.
The user enters their message at the bottom of the screen and presses the Enter key and the message will be updatd on the screen and on the text display.

To exit the marquee - press Ctrl-Shift-X or Escape

There is now a HTML page that enables the text display to be used as a standalone unit. The Python program will still operate on the Raspberry Pi. The HTML version has the added benefit of being able set the colour for the text and background.
