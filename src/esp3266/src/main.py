### D7 RXD2
### D8 RXD2

### from machine import UART

# Complete project details at https://RandomNerdTutorials.com


def web_page():
  html = 'success'
  return html


def byteToInt(byte):
    """
    byte -> int

    Determines whether to use ord() or not to get a byte's value.
    """
    if hasattr(byte, 'bit_length'):
        # This is already an int
        return byte
    return ord(byte) if hasattr(byte, 'encode') else byte[0]

def intToByte(i):
    """
    int -> byte

    Determines whether to use chr() or bytes() to return a bytes object.
    """
    return chr(i) if hasattr(bytearray(), 'encode') else bytearray([i])

def stringToBytes(s):
    """
    string -> bytes

    Converts a string into an appropriate bytes object
    """
    return s.encode('ascii') if sys.version_info >= (3, 0) else s

def send_error_response(conn, response):
  conn.send('HTTP/1.1 405 ERROR\n')
  conn.send('Content-Type: text/plain\n')
  conn.send('Connection: close\n\n')
  conn.sendall(response)

def send_ok_response(conn, response):
  conn.send('HTTP/1.1 200 OK\n')
  conn.send('Content-Type: text/plain\n')
  conn.send('Connection: close\n\n')
  conn.sendall(response)

def get_colour_byte(colour_name):
  colour_byte = -1
  if colour_name == 'black':
    colour_byte = 0x00
  if colour_name == 'red':
    colour_byte = 0x01
  elif colour_name == 'orange':
    colour_byte = 0x02
  elif colour_name == 'yellow':
    colour_byte = 0x03
  elif colour_name == 'green':
    colour_byte = 0x04
  elif colour_name == 'blue':
    colour_byte = 0x05
  elif colour_name == 'indigo':
    colour_byte = 0x06
  elif colour_name == 'violet':
    colour_byte = 0x07
  elif colour_name == 'white':
    colour_byte = 0x08

  return colour_byte

def calc_chksum(frame):
  chksum = 0
  for bb in frame[2:]:
    chksum += bb
  chksum = chksum & 0xFF
  return intToByte( 0xFF - chksum)


s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind(('', 80))
s.listen(5)

uart = UART(1, baudrate=9600, bits=8, parity=None, stop=1)

while True:
  print('Waiting for a connection')
  conn, addr = s.accept()
  print('Got a connection from %s' % str(addr))
  time.sleep(1)
  request = conn.recv(1024)
  request = str(request)
  print('Content = %s' % request)

  ## bodyIdx = request.find('\\r\\n\\r\\n') + 8
  ## body = request[bodyIdx:-1]
  request_array = request.split('\\r\\n')
  ## print('Request array %s' % request_array)
  bodyIdx = 0
  for ll in request_array:
    print("%d - %s" % (bodyIdx, ll))
    bodyIdx += 1
    if ll == '':
      break;
  print('Body start line %d' % bodyIdx)
  body_array = request_array[bodyIdx-1:]
  body = ''
  for bb in body_array:
    body += bb
  body = body[:-1]
  print('located body: %s ' % body)

  if body == None or body == '':
    response = 'error - The body is empty'
    send_error_response(conn, response)
    conn.close()
    continue

  jbody = json.loads(body)
  action = jbody['action']
  if action == 'message':
    text = jbody['param']
    data_length = len(text)+1
    frame = bytearray()
    frame.append(0xFE)
    frame.append(data_length)
    frame.append(0x01)
    frame += stringToBytes(text)

    chksum = calc_chksum(frame)
    frame += chksum

    print('Frame: %s ' % frame)
    sent = uart.write(frame)
    print("Sent %d" % sent)

  if action == 'colour':
    colour_array = jbody['param'].split(':')
    frame = bytearray()
    frame.append(0xFE)
    frame.append(len(colour_array)+1)
    frame.append(0x02)
    for c in colour_array:
      colour_byte = get_colour_byte(c)
      if colour_byte < 0:
        send_error_response(conn, 'The colour specification is incorrect')
        continue
      frame.append(colour_byte)

    chksum = calc_chksum(frame)
    frame += chksum

    print('Frame: %s ' % frame)
    sent = uart.write(frame)
    print("Sent %d" % sent)

  if action == 'speed':
    speed = jbody['param']

    frame = bytearray()
    frame.append(0xFE)
    frame.append(0x02)
    frame.append(0x03)
    if speed == 'stop':
      frame.append(0x00)
    elif speed == 'start':
      frame.append(0x7F)
    else:
      frame.append(0x7F)
    
    chksum = calc_chksum(frame)
    frame += chksum

    print('Frame: %s ' % frame)
    sent = uart.write(frame)
    print("Sent %d" % sent)

  if action == 'reset':
    frame = bytearray()
    frame.append(0xFE)
    frame.append(0x01)
    frame.append(0x04)

    chksum = calc_chksum(frame)
    frame += chksum

    print('Frame: %s ' % frame)
    sent = uart.write(frame)
    print("Sent %d" % sent)

  if sent > 0:
    ## uart_response = uart.read()
    uart_response = 'void'
    if uart_response == None:
      response = 'error - send to UART failed - timeout'
      send_error_response(conn, response)
    else:
      response = 'success %s' % uart_response
      send_ok_response(conn, response)
  else:
    send_error_response(conn, response)

  conn.close()
