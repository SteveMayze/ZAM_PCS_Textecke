from tkinter import *

from tkinter import ttk
import requests
import time
import logging
import os

from pathlib import Path
import re

rest_url = os.getenv('REST_URL')

logger = None
debug = False

default_message = "Willkommen im ZAM   "

def get_last_message( log_file_name):
    log_file = Path(log_file_name)
    if log_file.exists():
        ## Get the last message
        with open(log_file_name, "rb") as f:
            try:
                f.seek(-2, os.SEEK_END)
                while f.read(1) != b'\n':
                    f.seek(-2, os.SEEK_CUR)
            except OSError:
                f.seek(0)
            last_line = f.readline().decode()
        last_line = re.sub("\n+", "", last_line)
        if last_line:
            message = re.sub("^[0-9-]{10} [0-9:,]{12} ", "", last_line)
        else:
            message = default_message
    else:
        message = default_message
    return message

class App( Frame ):
    logger = None
    message = None
    new_message = None
    canvas = None
    status = None
    scroll_frame = None
    fps = 120

    def post_message(self, message):
        if not debug:
            payload = {"action":"message","param":message}
            headers = {"Connection":"keep-alive", "Accept":"*/*" }
            s = requests.Session()
            response = s.post(rest_url+"/api/v1/message", headers=headers, json=payload, timeout=30)
        else:
            time.sleep(1)

    def handle_message(self, event):
        tmp_msg = self.new_message.get()
        if tmp_msg:
            if len(tmp_msg) > 200:
                tmp_msg = tmp_msg[0:190]
                tmp_msg += "..."
            tmp_msg += "   "

            self.status.set("Senden: ")
            self.message.set(tmp_msg)
            self.new_message.set("")
            self.logger.info(f'{tmp_msg}')
            self.canvas.itemconfig(self.text_id, text=self.message.get())
            self.post_message(tmp_msg)
            self.status.set("Aktuell: ")
        else:
            self.logger.debug("Empty message")

    def close_and_end(self, event):
        self.logger.debug('Stopping')
        exit()

    def shift(self):
        x1,y1,x2,y2 = self.canvas.bbox("marquee")
        if( x2<0 or y1<0 ): #reset the coordinates
            x1 = self.canvas.winfo_width()
            y1 = self.canvas.winfo_height()//2
            self.canvas.coords("marquee", x1, y1)
            x1,y1,x2,y2 = self.canvas.bbox("marquee")
        else:
            self.canvas.move("marquee", -2, 0)

        self.canvas.after(1000//self.fps, self.shift)


    def __init__(self, master, logger, default_message):
        super().__init__(master)

        self.logger = logger

        self.message = StringVar()
        self.new_message = StringVar()
        self.status = StringVar()

        master.columnconfigure(0, weight=1)
        master.rowconfigure(0, weight=1)

        self.mainframe = ttk.Frame(master, padding="5 5 12 12")
        self.mainframe.grid(column=0, row=0, sticky=(N, S, E, W))
        self.mainframe.columnconfigure(0, weight=1)
        self.mainframe.rowconfigure(0, weight=1)

        self.message.set(default_message)

        title_frame = ttk.Frame(self.mainframe)
        title_frame.grid(column=0, row=0)
        title = ttk.Label(title_frame, text="Textecke")
        title.grid(column=0, row=0, sticky=(E, W))
        title.config(font=("Helvetica", 32))
        self.canvas = Canvas(self.mainframe,bg='black')
        self.canvas.grid(column=0, row=1, rowspan=2, sticky=(N, S, E, W))
        x1 = int(self.canvas.winfo_width()) 
        y1 = self.canvas.winfo_height()//2
        self.text_id = self.canvas.create_text(x1, y1, text=self.message.get(),font=('Noto Mono',48,'normal'),fill='white',tags=("marquee",),anchor='w')
        height = int(master.winfo_screenheight() * 0.8)
        self.canvas['height']=height
        self.fps=60    #Change the fps to make the animation faster/slower
        self.shift()

        entry_frame = ttk.Frame(self.mainframe)
        entry_frame.grid(column=0, row=3, sticky=(S, E, W))

        title = ttk.Label(entry_frame, text="Schreibe doch was")
        title.grid(column=0, row=0, sticky=(E), padx=5)
        title.config(font=("Helvetica", 16))

        message_entry = ttk.Entry(entry_frame, width=100, font=('Helvetica 24'), textvariable=self.new_message)
        message_entry.grid(column=1, row=0,  sticky=(S, E, W))

        message_entry.focus()

        master.bind('<Return>', self.handle_message)
        master.bind('<Escape>', self.close_and_end)


        self.post_message(self.message.get())


def main():
    logging.basicConfig(filename='marquee.log', level=logging.INFO, format='%(asctime)s %(message)s')
    logger = logging.getLogger('ZAM_marquee')
    root = Tk()
    root.attributes('-fullscreen', True)
    root.title("Textecke")

    message = get_last_message('marquee.log')
    logger.debug(f"message: {message}")

    mq = App(root, logger, message)
    logger.debug('Starting')
    mq.mainloop()
    mq.close_and_end(None)


if __name__ == '__main__':
    main()
