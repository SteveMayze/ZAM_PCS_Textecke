# ZAM PCS Textecke

The [ZAM](https://betreiberverein.de/?doing_wp_cron=1649430713.6483170986175537109375) Post Corona Stadt (PCS) Textecke project is an idea to engage the passer by to leave a message.

Further deatils can be found on the project page (German) [41 Textecke](https://wiki.betreiberverein.de/books/projekte-aktuell/page/41-textecke-639)

# Python Screen Marquee Effect
Since revision 1. was not working the way it was intended to, an alternative was set up to run a Python Marquee type program on the screen only. The usage is still the same - the user presses the return key, enters a message and then presses the return key again and the text on the screen is replaced with the new message.

## Setup


```shell
git clone ...
cd ZAM_PCS_Textecke
cd src/python
python3 -m venv .venv
pip install -r requirements.txt

.venv/bin/python marquee.py
```

## Usasge
Once started, a default message will show. This message will coninually scroll across the screen until the user presses the return key.
When the return key is pressed, a pop-up dialog will appear and prompt for a messages. The user presses the return key once more and the pop-up will disapear and the scrolling text will be replaced with the new message.

To exit the marquee - press Ctrl-Shift-X

# Revision 1. Setup

The main setup is required on the Raspberry Pi to act as the key reader. Once set up, the keys captured on the Raspberry Pi are passed onto the Teensy which is acting as a keyboard emulator.
The Teensy needs to be programmed with the Arduino program in `src/teensy/TextDisplayDriver.ino`

## Raspberry Pi

```shell
git clone ...
cd ZAM_PCS_Textecke
cd src/raspi
python3 -m venv .venv
pip install -r requirements.txt
```

# Execution

## Raspberry Pi

There are two ways to start the keyreader script. Once is via Python, the other is to use the helper bash script. Using the bash shell script does not required activating the python virtual env.

### bash
```shell
./keyreader.sh
```

### Python

```shell
. .venv/bin/activate
python keyreader.py
```