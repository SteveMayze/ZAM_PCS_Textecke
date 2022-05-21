

import tkinter as tk
import logging

logger = None

class MessageWindow(object):
    
    ref = None
    
    def __init__(self,master):
        ref = master
        root_x = master.winfo_rootx()
        root_y = master.winfo_rooty()
        root_width = master.winfo_width()

        win_width = int(master.winfo_width() * 0.8)
        win_x = root_x + (root_width // 2 ) - ( win_width // 2)
        win_y = root_x + (master.winfo_height() // 2)+75

        top = self.top = tk.Toplevel(master)
        top.geometry(f'{win_width}x150+{win_x}+{win_y}')

        self.l = tk.Label(top,text="Teile dich doch mit")
        self.l.config(font=("Helvetica", 32))
        self.l.pack()
        self.e = tk.Entry(top, width=400, font=('Helvetica 24'))
        self.e.pack()
        self.e.focus_set()
        top.bind('<Return>', self.cleanup)

    def cleanup(self, event):
        self.value=self.e.get()
        self.top.destroy()
        

class App( tk.Frame ):
   
    canvas = None
    text_message = 'ZAM Post CORONA Stadt (PCS) Projekt 41 - Textecke'
    text_id = None
    
    def close_and_end(self, event):
        logging.info('Exiting')
        exit()
        
    def request_message(self, event):
        self.w = MessageWindow(self.master)
        self.master.wait_window(self.w.top)
        self.master.focus_set()
        if self.w.value:
            logging.info(f'{self.w.value}')
            self.text_message = self.w.value
            self.canvas.itemconfig(self.text_id, text=self.text_message)
        else:
            logging.info('empty message')
            print('An empty message')
            
            
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

    def __init__(self, master):
        super().__init__(master)
        self.pack()

        master.title('Textecke')
        ## master.geometry("1500x400")
        ## self.canvas = tk.Canvas(master,bg='black',height=400, width=1500)
        self.canvas = tk.Canvas(master,bg='black')
        self.canvas.pack(fill=tk.BOTH, expand=1)
        text_var =  self.text_message
        
        x1 = self.canvas.winfo_width()
        y1 = self.canvas.winfo_height()//2        
        
        self.text_id = self.canvas.create_text(self.canvas.winfo_width(), y1,text=text_var,font=('Helvetica',64,'normal'),fill='white',tags=("marquee",),anchor='w')
        x1,y1,x2,y2 = self.canvas.bbox("marquee")
        width = x2-x1
        height = y2-y1
        self.canvas['width']=width
        self.canvas['height']=height
        self.fps=120    #Change the fps to make the animation faster/slower
        self.shift()
        master.bind('<Return>', self.request_message)
        master.bind('<Control-X>', self.close_and_end)


def main():
    logging.basicConfig(filename='marquee.log', level=logging.DEBUG, format='%(asctime)s %(message)s')
    logger = logging.getLogger('ZAM_marquee')
    root = tk.Tk()
    root.attributes('-fullscreen', True)
    mq = App(root)
    logging.info('Starting')
    mq.mainloop()


if __name__ == '__main__':
    main()
    