# ZAM PCS Textecke

The [ZAM](https://betreiberverein.de/?doing_wp_cron=1649430713.6483170986175537109375) Post Corona Stadt (PCS) Textecke project is an idea to engage the passer by to leave a message.

Further deatils can be found on the project page (German) [41 Textecke](https://wiki.betreiberverein.de/books/projekte-aktuell/page/41-textecke-639)

# Setup

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