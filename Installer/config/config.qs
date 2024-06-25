function Controller()
{
	if (installer.isInstaller())
	{
		if (installer.value("install") == "auto")
		{
			installer.installationFinished.connect(this, Controller.prototype.installFinished);
			installer.currentPageChanged.connect(this, Controller.prototype.pageChanged);
		}
	}
	
	if (installer.isUninstaller())
	{	
		installer.uninstallationStarted.connect(this, Controller.prototype.killPrograms);
		installer.setDefaultPageVisible(QInstaller.Introduction, false);
		installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
		installer.setDefaultPageVisible(QInstaller.LicenseCheck, false);
		if (installer.value("uninstall") == "auto")
		{
			installer.setDefaultPageVisible(QInstaller.ReadyForInstallation, false);
			installer.setDefaultPageVisible(QInstaller.InstallationFinished, false);
			installer.uninstallationFinished.connect(this, Controller.prototype.uninstallFinished);
		}
	}
}

Controller.prototype.killPrograms = function()
{
	var targetDir = installer.value("TargetDir");
	
	console.log("Killing Pinhole processes running in directory: " + targetDir);
	
	if (systemInfo.productType === "windows") {
		installer.killProcess(targetDir + "\\PinholeConsole.exe");
		//installer.killProcess(targetDir + "\\PinholeServer.exe");
		//installer.killProcess(targetDir + "\\PinholeHelper.exe");
	} else if (systemInfo.productType === "osx") {
		installer.execute("launchctl", ["unload", "@HomeDir@/Library/LaunchAgents/Pinhole.Server.plist"]);

		installer.killProcess(targetDir + "/PinholeConsole.app/Contents/MacOS/PinholeConsole");
		installer.killProcess(targetDir + "/PinholeServer.app/Contents/MacOS/PinholeServer");
		installer.killProcess(targetDir + "/PinholeHelper.app/Contents/MacOS/PinholeHelper");
	} else {
		installer.killProcess(targetDir + "/PinholeConsole");
		//installer.killProcess(targetDir + "/PinholeServer");
		//installer.killProcess(targetDir + "/PinholeHelper");
	}
	
	// HACK because killProcess is NOT IMPLEMENTED on platforms other than Windows!!
	if (systemInfo.productType != "windows")
	{
		installer.execute("pkill", ["PinholeConsole"]);
		//installer.execute("pkill", ["PinholeServer"]);
		//installer.execute("pkill", ["PinholeHelper"]);
	}
}

Controller.prototype.uninstallFinished = function()
{
	gui.rejectWithoutPrompt();
}

Controller.prototype.installFinished = function()
{
	gui.rejectWithoutPrompt();
}

Controller.prototype.pageChanged = function(page)
{
	console.log(page);
	gui.clickButton(buttons.NextButton, 0);
}
