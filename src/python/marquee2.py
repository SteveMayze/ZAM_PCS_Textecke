
from nis import match
import tkinter as tk


class MessageWindow(object):
    def __init__(self,master):
        top = self.top = tk.Toplevel(master)
        self.l = tk.Label(top,text="Sag was!")
        self.l.pack()
        self.e = tk.Entry(top)
        self.e.pack()
        self.e.focus_set()
        ## self.b=tk.Button(top,text='Ok',command=self.cleanup)
        ## self.b.pack()
        top.bind('<Return>', self.cleanup)

    def cleanup(self, event):
        self.value=self.e.get()
        self.top.destroy()
        

class App( tk.Frame ):
   
    canvas = None
    text_message = 'Lorem ipsum '
    text_id = None
        
    def request_message(self, event):
        print('Return pressed...')
        self.w = MessageWindow(self.master)
        self.master.wait_window(self.w.top)
        self.text_message = self.w.value
        self.canvas.itemconfig(self.text_id, text=self.text_message)
        self.text

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
        master.geometry("1500x400")
        self.canvas = tk.Canvas(master,bg='black',height=400, width=1500)
        self.canvas.pack(fill=tk.BOTH, expand=1)
        text_var =  self.text_message
        self.text_id = self.canvas.create_text(1500, 200,text=text_var,font=('Helvetica',64,'normal'),fill='white',tags=("marquee",),anchor='w')
        x1,y1,x2,y2 = self.canvas.bbox("marquee")
        width = x2-x1
        height = y2-y1
        self.canvas['width']=width
        self.canvas['height']=height
        self.fps=120    #Change the fps to make the animation faster/slower
        self.shift()
        master.bind('<Return>', self.request_message)


root = tk.Tk()
mq = App(root)
mq.mainloop()

