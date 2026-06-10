
# generateMemoryMap.py

# DESCRIPTION :
#	Python module to generate a memory map
#	(for initializing memory in a processor model) 
#	from a hex dump of the executable.
#	This module is imported in sscript compileToSparc.py
#
# AUTHOR :
#	Neha Karanjkar (24 April 2015)



import string, sys, os
#check if a given string 
#is a valid hex number 
def is_hex(s):
	try:
		int(s, 16)
		return True
	except ValueError:
		return False

def printLine(address, data_tokens):
	byte_addr = address
	byte_index = 0
	for token in data_tokens:
		token = token.strip()
		if token.startswith("0x") or token.startswith("0X"):
			token = token[2:]
		if len(token) == 0:
			continue
		if (len(token) % 2) != 0:
			token = "0" + token
		for k in range(0, len(token), 2):
			print(("%x" % (byte_addr + byte_index)) + ("\t%s" % token[k:k+2].lower()))
			byte_index += 1
	return

#function to generate a byte-wise
#memory map from a hex dump file
# format of a line in the hexdump file is :
# <address> word <word> <word> <word> <ascii characters to be ignored>

def generate_memorymap(inputfile,outputfile) :
	try:
		of= open(outputfile, 'w')
	except IOError:
		print ("Error in opening file ", outputfile, "for writing")
		return True
	try:
		f = open(inputfile, 'r')
	except IOError:
		print ("Error in opening file ", inputfile, "for reading")
		return True
	remember_stdout=sys.stdout
	#set stdout to outputfile
	sys.stdout=of

	# Read the first line 
	line = f.readline()
	# keep reading line one at a time
	# till the file is empty
	
	while line:
		address=0
		data=[]
		words=line.split()
		if is_hex(words[0]) :
			address = int(words[0],16)
			for word in words[1:5]:
				if is_hex(word) :
					data.append(word)
			# print the line read:
			#print"\n ---","%08x"%address,data
			printLine(address,data)
		line = f.readline()
	f.close()
	sys.stdout=remember_stdout
	of.close()
	return False

