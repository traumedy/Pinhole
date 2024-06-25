var upgrading = false;

function Component()
{
    //installer.setValue("AllUsers", true);
	component.loaded.connect(this, Component.prototype.installerLoaded);
	installer.setDefaultPageVisible(QInstaller.TargetDirectory, false);
	
	if (installer.isInstaller())
	{
		installer.installationStarted.connect(this, Component.prototype.launchUninstaller);
	}
}

Component.prototype.createOperations = function()
{
	component.createOperations();

	if (systemInfo.productType === "windows") {
		component.addElevatedOperation("Execute", "{0,1638,3010}", "@TargetDir@\\vcredist_x64_2017.exe", "/quiet", "/norestart");
	}
	else if (systemInfo.productType === "osx") {
		// No Mac OS dependencies THANK GOO
	} else {
		console.log("productType: '" + systemInfo.productType + "' productVersion: " + systemInfo.productVersion);
		var manager = "apt-get";
		var pkg = "libssl1.0-dev";
		// Handle different package managers
		if (systemInfo.productType == "ubuntu")
		{
			if (installer.versionMatches(systemInfo.productVersion, "<17.0"))
			{
				pkg = "libssl-dev";
			}
		}
		else
		{
			manager = "dpkg";
			// todo
		}
		
		component.addElevatedOperation("Execute", manager, "install", "-y", pkg);
	}
}

// Utility function like QString QDir::toNativeSeparators(const QString & pathName) [static]
var Dir = new function () 
{
	this.toNativeSparator = function (path) 
	{
		if (installer.value("os") == "win")
			return path.replace(/\//g, '\\');
		return path;
	}
};

// Called as soon as the component was loaded
Component.prototype.installerLoaded = function()
{
	if (installer.addWizardPage(component, "TargetWidget", QInstaller.TargetDirectory)) 
	{
		var widget = gui.pageWidgetByObjectName("DynamicTargetWidget");
		if (widget != null) 
		{
			widget.targetDirectory.textChanged.connect(this, Component.prototype.targetChanged);
			widget.targetChooser.clicked.connect(this, Component.prototype.chooseTarget);

			widget.windowTitle = "Installation Folder";
			widget.targetDirectory.text = Dir.toNativeSparator(installer.value("TargetDir"));
		}
	}
}

// Callback when one is clicking on the button to select where to install your application
Component.prototype.chooseTarget = function () 
{
	var widget = gui.pageWidgetByObjectName("DynamicTargetWidget");
	if (widget != null) 
	{
		var newTarget = QFileDialog.getExistingDirectory("Choose your target directory.", widget.targetDirectory.text);
		if (newTarget != "") 
		{
			widget.targetDirectory.text = Dir.toNativeSparator(newTarget);
		}
	}
}

Component.prototype.targetChanged = function (text) 
{
	var widget = gui.pageWidgetByObjectName("DynamicTargetWidget");
	if (widget != null) 
	{
		if (text != "") 
		{
			widget.complete = true;
			installer.setValue("TargetDir", text);
			if (installer.fileExists(text + "/components.xml")) 
			{
				var warning = "<font color='red'>" + qsTr("A previous installation exists in this folder.<br/>If you continue, the previous version will be uninstalled first.") + "</font>";
				widget.labelOverwrite.text = warning;
				upgrading = true;
			} 
			else 
			{
				widget.labelOverwrite.text = "";
				upgrading = false;
			}
			return;
		}
		widget.complete = false;
	}
}

function sleep(ms) {
  var now = new Date().getTime();
    while(new Date().getTime() < now + ms){ /* do nothing */ } 
}

Component.prototype.launchUninstaller = function ()
{
	if (upgrading == true)
	{
		if (systemInfo.productType === "windows") {
			var maintenancetool = installer.value("TargetDir") + "\\PinholeUninstaller.exe";
		} else if (systemInfo.productType === "osx") {
			var maintenancetool = installer.value("TargetDir") + "/PinholeUninstaller.app/Contents/MacOS/PinholeUninstaller";
		} else { 
			var maintenancetool = installer.value("TargetDir") + "/PinholeUninstaller";
		}
		
		console.log("Uninstalling previous version of Pinhole...")
		installer.execute(maintenancetool, ["uninstall=auto"]);

		// Hack because DLLs get deleted otherwise
		sleep(2000);
		
		console.log("Finished uninstalling previous version of Pinhole")
	}
}

