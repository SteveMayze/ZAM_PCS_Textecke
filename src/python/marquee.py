from tkinter import *

from tkinter import ttk
import requests
import time
import logging

rest_url = "http://10.233.2.108/"

logger = None
debug = True
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
            response = s.post(rest_url, headers=headers, json=payload, timeout=30)
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
            self.logger.info("Empty message")

    def close_and_end(self, event):
        self.logger.info('Exiting')
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
        ## self.pack()
        
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
        ## self.canvas.pack(fill=BOTH, expand=1)
        x1 = self.canvas.winfo_width()
        y1 = self.canvas.winfo_height()    
        self.text_id = self.canvas.create_text(self.scroll_frame.winfo_width(), 40, text=self.message.get(),font=('Helvetica',40,'normal'),fill='white',tags=("marquee",),anchor='w')
        x1,y1,x2,y2 = self.canvas.bbox("marquee")
        width = x2-x1
        height = (y2-y1)+40
        self.canvas['width']=width
        self.canvas['height']=height
        self.fps=60    #Change the fps to make the animation faster/slower
        self.shift()


        # self.status.set("Aktuell: ")
        # status_message  = ttk.Label(scroll_frame, padding="3 3 12 12", textvariable=self.status)
        # status_message.grid(column=0, row=0, sticky=(E, W))
        # status_message.config(font=("Helvetica", 24))

        # current_message = ttk.Label(scroll_frame, padding="3 3 12 12", textvariable=self.message)
        # current_message.grid(column=1, row=0, sticky=(E, W))
        # current_message.config(font=("Helvetica", 24))

        entry_frame = ttk.Frame(self.mainframe, padding="3 3 12 12")
        entry_frame.grid(column=0, row=2, sticky=(E, W))


        message_entry = ttk.Entry(entry_frame, width=80, font=('Helvetica 24'), textvariable=self.new_message)
        message_entry.grid(column=2, row=1, sticky=(E, W))

        message_entry.focus()

        master.bind('<Return>', self.handle_message)
        master.bind('<Escape>', self.close_and_end)

        root_x = master.winfo_rootx()
        root_y = master.winfo_rooty()
        root_width = int(master.winfo_screenwidth() * 0.8)
        root_height = int(master.winfo_screenheight() * 0.30)

        win_x = root_x + (root_width // 2 ) - ( root_width // 2)
        win_y = root_y + (root_height // 2)+75
        master.geometry(f'{root_width}x{root_height}+{win_x}+{win_y}')

        self.post_message(self.message.get())
        

def main():
    logging.basicConfig(filename='marquee.log', level=logging.DEBUG, format='%(asctime)s %(message)s')
    logger = logging.getLogger('ZAM_marquee')
    root = Tk()
    root.title("Textecke")
    # root.attributes('-fullscreen', True)
    mq = App(root, logger)
    logger.info('Starting')
    mq.mainloop()
    mq.close_and_end(None)


if __name__ == '__main__':
    main()
        
