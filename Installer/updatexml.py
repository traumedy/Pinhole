#!/usr/bin/env python3

import datetime
import xml.etree.ElementTree

def updateXml(xmlName, versionString, dateString):
	# Open the xml file
	xmlFile = xml.etree.ElementTree.parse(xmlName)

	# Open the root of the xml tree
	root = xmlFile.getroot()
	
	versionElem = root.find("Version")
	versionElem.text = versionString
	releaseDateElem = root.find("ReleaseDate")
	releaseDateElem.text = dateString
	xmlFile.write(xmlName)

with open('../common/Version.h', 'r') as f:
	for line in f:
		if line.startswith('#define PINHOLE_VERSION'):
			firstQuote = line.find('"')
			secQuote = line.rfind('"')
			versionString = line[firstQuote + 1 : secQuote]
			continue

dateString = datetime.date.today().isoformat()

print("Pinhole version:", versionString)
print("Release date:", dateString)

# Write version to a file for later use
versionFile = open('Version', 'w')
versionFile.write(versionString)
versionFile.close()

updateXml('packages/com.obscura.root/meta/package.xml', versionString, dateString)
updateXml('packages/com.obscura.root.console/meta/package.xml', versionString, dateString)
updateXml('packages/com.obscura.root.server/meta/package.xml', versionString, dateString)

exit()

