import primes
import os
import sys
import subprocess

#print(primes.__doc__)
#path to directory in cyverser
files_path = '/iplant/home/shared/avra/CONUS_1.0/CC2019/Postproc_Test/Pressure/'

#try to connect to cyverse server
os.system('iinit')

#check if connected and list files
list_files = subprocess.check_output(['ils', files_path])

list_files = [x.strip() for x in list_files.split('\n') if '.pfb' in x]

if not list_files:
	print 'please check the connection with cyverse'
	sys.exit()

for ii,fi in enumerate(list_files):
	os.system('iget -K '+files_path+fi)
	if ii == len(list_files)-1:
		print 'done'
	