import serial, time
import re
import subprocess
import requests

arduino = serial.Serial('/dev/ttyACM0',9600, timeout=0.01)
time.sleep(5) #give the connection a second to settle
print "Become active"
arduino.write("R") # Let Arduino know we are running

while True:
	data = arduino.readline()
	
	if re.match('.. .. .. ..', data):
		# send finn request
		cupID = data.rstrip('\n')
		cupID = cupID.replace(' ', '')
		cupID = cupID[:4]
		
		with open("log.txt", "a") as myfile:
    			myfile.write("making request for cupID: " + cupID + "\n")

		arduino.write("1")
                #r = requests.post("http://localhost:3001/actions", json={"actionID": "58467254-F1F9-4B6A-8B62-E15C9F294407", "alternativeID": cupID})

                #if ("Error" in r.json()["message"]):
                #        print "error"
                #        arduino.write("0")
                #else:
                #        print "success"
                #        arduino.write("1")
        else:
                with open("log.txt", "a") as myfile:
    			myfile.write("Listening for validation requests\n")
