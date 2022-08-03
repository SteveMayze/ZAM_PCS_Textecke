from tkinter import *

from tkinter import ttk
import requests
import time
import logging
import os

rest_url = os.getenv('REST_URL')

logger = None
debug = False

class App( Frame ):
    logger = None
    message = None
    new_message = None
    canvas = None
    status = None
    scroll_frame = None
    fps = 120

    default_message = "TEXTECKE - ZAM Post Corona Stadt (PCS) Projekt 41   "

    def post_message(self, message):
        if not debug:
            payload = {"action":"message","param":message}
            headers = {"Connection":"keep-alive", "Accept":"*/*" }
            s = requests.Session()
            response = s.post(rest_url+"/api/v1/", headers=headers, json=payload, timeout=30)
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


    def __init__(self, master, logger):
        super().__init__(master)

        self.logger = logger

        self.message = StringVar()
        self.new_message = StringVar()
        self.status = StringVar()

        self.mainframe = ttk.Frame(master, padding="3 3 12 12")
        self.mainframe.grid(column=0, row=0, sticky=(N, S, E, W))
        self.mainframe.pack(fill=BOTH, expand=1)
        master.columnconfigure(0, weight=1)
        master.rowconfigure(0, weight=1)

        self.message.set(self.default_message)

        help_frame = ttk.Frame(self.mainframe, padding="3 3 12 12")
        help_frame.grid(column=0, row=0)
        help_message = ttk.Label(help_frame, padding="3 3 12 12", text="Schreibe doch was")
        help_message.grid(column=0, row=0, sticky=(E, W))
        help_message.config(font=("Helvetica", 32))

        self.scroll_frame = ttk.Frame(self.mainframe)
        self.scroll_frame.grid(column=0, row=1)


        self.canvas = Canvas(self.scroll_frame,bg='black')
        self.canvas.grid(column=0, row=0, sticky=(N, S, E, W))
        x1 = int(master.winfo_screenwidth() ) # self.canvas.winfo_width()
        y1 = self.canvas.winfo_height()//2
        self.text_id = self.canvas.create_text(x1, y1, text=self.message.get(),font=('Helvetica',48,'normal'),fill='white',tags=("marquee",),anchor='w')
        x1,y1,x2,y2 = self.canvas.bbox("marquee")
        width =  int(master.winfo_screenwidth() * 0.95)
        height = int(master.winfo_screenheight() * 0.8)
        self.canvas['width']=width
        self.canvas['height']=height
        self.fps=60    #Change the fps to make the animation faster/slower
        self.shift()

        entry_frame = ttk.Frame(self.mainframe, padding="3 3 12 12")
        entry_frame.grid(column=0, row=2, sticky=(E, W))


        message_entry = ttk.Entry(entry_frame, width=100, font=('Helvetica 24'), textvariable=self.new_message)
        message_entry.grid(column=2, row=1, sticky=(E, W))

        message_entry.focus()

        master.bind('<Return>', self.handle_message)
        master.bind('<Escape>', self.close_and_end)

        root_x = master.winfo_rootx()
        root_y = master.winfo_rooty()
        root_width = int(master.winfo_screenwidth() * 0.95)
        root_height = int(master.winfo_screenheight() * 0.30)

        win_x = root_x + (root_width // 2 ) - ( root_width // 2)
        win_y = root_y + (root_height // 2)+75
        master.geometry(f'{root_width}x{root_height}+{win_x}+{win_y}')

        self.post_message(self.message.get())


def main():
    logging.basicConfig(filename='marquee.log', level=logging.INFO, format='%(asctime)s %(message)s')
    logger = logging.getLogger('ZAM_marquee')
    root = Tk()
    root.attributes('-fullscreen', True)
    root.title("Textecke")
    mq = App(root, logger)
    logger.debug('Starting')
    mq.mainloop()
    mq.close_and_end(None)


if __name__ == '__main__':
    main()
