import sys
from PIL import Image

defserial = "/dev/rfcomm0"

def sendimage(infile, serial = defserial):
	# Open image
	print("Opening image " + infile)
	image = Image.open(infile)

	# Check image dimentsion
	if image.width != 8 and image.height != 8:
		print("Image has size: " + str(image.width) + "x" + str(image.height) + " but size 8x8 is required")
		return
	
	# Initialize serial device
	print("Opening serial port: " + serial)
	fout = open(serial, "wb")

	# Send picture start
	print("Sending picture start")
	fout.write(chr(0b10000000))

	print("Sending picture data")
	# Send image
	for pix in image.getdata():
		print(pix)
		byte = pix[0] / 64 << 4 | pix[1] / 64 << 2 | pix[2] / 64 << 0
		fout.write(chr(byte))

	print("Sending picture end")
	# Send picture end
	fout.write(chr(0b01000000))

	fout.close()
	image.close()
	
	print("All done")

# Process argument
if len(sys.argv) == 2:
	sendimage(sys.argv[1])
elif len(sys.argv) == 3:
	sendimage(sys.argv[1], sys.argv[2])
else:
	print("Pass image as the only argument")