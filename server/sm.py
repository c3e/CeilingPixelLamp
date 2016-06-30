 #import smbus
import can
import time
#import commands
from flask import Flask
from flask import request
from SimpleWebSocketServer import SimpleWebSocketServer, WebSocket
import json 
import threading
#bus = smbus.SMBus(0)

#
# [ pwm, r, g, b ]hping
#

class doit(WebSocket):

	def handleConnected(self):
		for client in clients:
			client.sendMessage(self.address[0] + u' - connected')
		clients.append(self)

	def handleClose(self):
		clients.remove(self)
		for client in clients:
			client.sendMessage(self.address[0] + u' - disconnected')


class panel:

	def __init__(self, *args):
		self.panel = []
		self.wpanel = []
		for j in range(4) :
			if len(args) == 0:
				self.panel.append([[0,0,0],[0,0,0],[0,0,0],[0,0,0]])
				self.wpanel.append([0,0,0,0])

			elif len(args) == 1:
				self.panel.append([[0,0,0],[0,0,0],[0,0,0],[0,0,0]])
				self.wpanel.append([args[0],args[0],args[0],args[0]])
			elif len(args) == 3:
				self.panel.append([[args[0],args[1],args[2]],[args[0],args[1],args[2]],[args[0],args[1],args[2]],[args[0],args[1],args[2]]])
				self.wpanel.append([0,0,0,0])	

	def setpixel (self,x,y,r,g,b,pwm): 
		#self.wpanel[x][y][0] = pwm
		self.panel[x][y][0] = r
		self.panel[x][y][1] = g
		self.panel[x][y][2] = b
	
	def setpanel (self,rgb ):
		for i in range(0,4):
			self.panel[i] = [rgb[4*i+0],rgb[4*i+1],rgb[4*i+2],rgb[4*i+3]]

	def get_panel(self):
		return self.panel

	#returns a list of different pixels
	def compare_to(self,rgb ):
		updates = []
		for i in range (0,4):
			for j in range(0,4):
				if ( not (rgb[i*4+j].__eq__(self.panel[i][j])) ):
					updates.append(i*4+j)
		return updates


#sudo modprobe vcan
# Create a vcan network interface with a specific name
#sudo ip link add dev vcan0 type vcan
#sudo ip link set vcan0 up

bus = can.interface.Bus("vcan0",bustype="socketcan_native")

ceiling = [ panel() for i in range (64)]

app = Flask(__name__)

clients = []

#
# RGB values and pixel_address are arrays of length 3,2,1
# Next version will reduce shit
#	
def update_pixel3( panel_address, pixel_address, update_mode, rgb0, rgb1, rgb2): 
	id = 	rgb2[0] + rgb2[1] * 0x100 + rgb2[2] * 0x10000 + pixel_address[2] * 0x1000000
	data = []
	data.append ( panel_address * 4 + update_mode ) 
	data.append ( pixel_address[0] * 0x10 + pixel_address[1] )
	data.append( rgb0[0] ) #2
	data.append( rgb0[1] ) #3
	data.append( rgb0[2] ) #4
	data.append( rgb1[0] ) #5
	data.append( rgb1[1] ) #6
	data.append( rgb1[2] ) #7
	msg = can.Message(arbitration_id=id, data=data, extended_id=True)
	bus.send(msg)

def update_pixel2( panel_address, pixel_address, update_mode, rgb0, rgb1): 
	id = 	rgb1[0] + rgb1[1] * 0x100 + rgb1[2] * 0x10000 + pixel_address[1] * 0x1000000
	data = []
	data.append( panel_address * 4 + update_mode )
	data.append( pixel_address[0] * 0x10 + pixel_address[1] )
	data.append( rgb0[0] ) #2
	data.append( rgb0[1] ) #3
	data.append( rgb0[2] ) #4
	data.append( rgb1[0] ) #5
	data.append( rgb1[1] ) #6
	data.append( rgb1[2] ) #7
	msg = can.Message(arbitration_id=id, data=data, extended_id=True)
	bus.send(msg)

def update_pixel1( panel_address, pixel_address, update_mode, rgb0): 
	id = 	rgb0[0] + rgb0[1] * 0x100 + rgb0[2] * 0x10000 + pixel_address * 0x1000000
	data = []
	data.append( panel_address * 4 + update_mode)     	#data[0]
	#data.append( pixel_address * 0x10 + pixel_address)	#data[1]
	#left out because updates are all on the same address and update through id comes last
	#data.append( rgb0[0] ) #2
	#data.append( rgb0[1] ) #3
	#data.append( rgb0[2] ) #4
	#data.append( rgb0[0] ) #5
	#data.append( rgb0[1] ) #6
	#data.append( rgb0[2] ) #7 
	msg = can.Message(arbitration_id=id, data=data, extended_id=True)
	bus.send(msg)

def update_pixel(panel_address, pixel_address, update_mode, rgb, n):
	if (n == 1):
		update_pixel1(panel_address, pixel_address, update_mode, rgb)
	elif ( n == 2):
		update_pixel2(panel_address, pixel_address,update_mode,rgb[0],rgb[1])
	else:
		update_pixel3(panel_address, pixel_address, update_mode, rgb[0], rgb[1], rgb[2])
#
# Sends WhiteOut
# parameters can be arrays of 6 at max
#
def send_white( panel_address, pwm):
	data[0] = 3
	data[1] = panel_address[0] * 0x100 + panel_address[1] >> 6
	data[2] = panel_address[1] & 15 * 0x1 + panel_address[2] >> 2
	data[3] = panel_address[2] & 3 * 0x10
	for i in range (0,pwm.length):
		data[i+4] = pwm[i]




# i2c specific initialization
#
#def initilization():
#	(status, output) = commands.getstatusoutput("/bin/sh -c i2cdetect -y 1 | sed -e 's/ --//g' -e 's/^[0-9]*: //g' -e 's/^    .*//g' | tr --delete '\n'")
#
#	if ( status == 0 ): 
#		addresses = []
#
#		foreach i in output.split(' '):
#			addresses.append((i)	 	


def sendAllClients( msg):
	for i in range(0,len(clients)):
		clients[i].sendMessage(msg)

@app.route('/')
def index():
	return "nya"

@app.route('/api/panel/<int:id>/pixel/<int:x>/<int:y>',methods=['POST'])
def update_single_pixel( id, x, y):
	rgb = request.get_json(force=True)
	print (rgb)
	ceiling[id].setpixel(x,y,rgb[0],rgb[1],rgb[2],0)
	update_pixel1(id,x*4+y,1,rgb)
	return "Ok"

@app.route('/api/panel/dumb/<int:id>',methods=['POST'])
def update_panel(id):
	rgbs = request.get_json(force=True)
	print ( rgbs)
	for i in range(0,13,3):
		update_pixel3(id, [i,i+1,i+2], 1, rgbs[i], rgbs[i+1], rgbs[i+2] )
	update_pixel1(id,15,2,rgbs[15])
	ceiling[id].setpanel(rgbs)

	return "Ok"

@app.route('/api/panel/<int:id>',methods=['POST'])
def update_panel_opt( id):
	rgbs = request.get_json(force=True)
	changed = ceiling[id].compare_to(rgbs)
	print(changed)
	size = len(changed) // 3

	for i in range (0,size):
		update_pixel3(id, [changed[i],changed[i+1],changed[i+2]], 1, rgbs[changed[i]], rgbs[changed[i+1]], rgbs[changed[i+2]] )
	if ( len(changed) % 3 == 1 ):
		update_pixel1(id,changed[len(changed)-1],2,rgbs[changed[len(changed)-1]])
	elif ( len(changed) % 3 == 2 ):
		update_pixel2(id,[changed[len(changed)-2],changed[len(changed)-1]],2,rgbs[changed[len(changed)-2]],rgbs[changed[len(changed)-1]])
	ceiling[id].setpanel(rgbs)
	sendAllClients(json.dumps(rgbs))
	return "Ok"

@app.route('/api/panel/get/<int:id>', methods=['GET'])
def get_panel(id):
	return json.dumps(ceiling[id].get_panel())

if __name__ == '__main__':
	app.debug = True
	app.run(host='0.0.0.0', port=8000)

## sort
## [0, 1, 2, 3, ... 9, ]
## [10, ..         19, ]
## [20, ..         29, ]
## [ ...               ]
## [90, ..         99  ]



