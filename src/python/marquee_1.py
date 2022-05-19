
from tkinter import *

def shift():
   x1,y1,x2,y2 = canvas.bbox("marquee")
   if(x2<0 or y1<0): #reset the coordinates
      x1 = canvas.winfo_width()
      y1 = canvas.winfo_height()//2
      canvas.coords("marquee", x1, y1)
      x1,y1,x2,y2 = canvas.bbox("marquee")
   else:
      canvas.move("marquee", -2, 0)
   canvas.after(1000//fps, shift)

############# Main program ###############

root=Tk()
root.title('Textecke')
root.geometry("1500x400")
canvas=Canvas(root,bg='black',height=400, width=1500)
canvas.pack(fill=BOTH, expand=1)
text_var="Hey there Delilah!,What's it like in New York City."
text=canvas.create_text(1500, 200,text=text_var,font=('Helvetica',64,'normal'),fill='white',tags=("marquee",),anchor='w')
x1,y1,x2,y2 = canvas.bbox("marquee")
width = x2-x1
height = y2-y1
canvas['width']=width
canvas['height']=height
fps=120    #Change the fps to make the animation faster/slower
shift()
root.mainloop()


