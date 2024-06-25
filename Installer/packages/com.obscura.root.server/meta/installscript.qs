
function Component()
{
    //installer.setValue("AllUsers", true);
	
	if (installer.isInstaller())
	{
		installer.installationFinished.connect(this, launchPrograms);
	}
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {

		// Install as a service to start at boot
		
		// Should match PINHOLE_SERVICENAME in PinholeCommon.h?
		var serviceName = "Pinhole";
		
		var serverDoArgs = [
		"sc.exe",
		"create",
		serviceName,
		"type=",
		"own",
		"start=",
		"auto",
		"binPath=",
		"@TargetDir@\\PinholeServer.exe --service",
		"DisplayName=",
		"Pinhole Server"
		];
		
		var serverUndoArgs = [
		"sc.exe",
		"delete",
		serviceName
		];
		
		component.addElevatedOperation("Execute", serverDoArgs, "UNDOEXECUTE", "{0,1060}", serverUndoArgs);
		component.addElevatedOperation("Execute", ["sc.exe", "start", serviceName], "UNDOEXECUTE", "{0,1060,1062}", ["sc.exe", "stop", serviceName]);
		
		/*
		// Add to registry entries to run as user
		// Make sure to run the 64 bit version of reg.exe 
		var regPath = installer.environmentVariable("WINDIR") + "\\sysnative\\reg.exe"
		
		var serverDoArgs = [
		regPath,
		"ADD",
		"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
		"/v", 
		"PinholeServer",
        "/d",
		"@TargetDir@\\RunDetached.exe PinholeServer.exe",
		"/f"
		];
		
		var serverUndoArgs = [
		regPath,
		"DELETE",
		"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
		"/v", 
		"PinholeServer",
		"/f"
		];
		
        component.addElevatedOperation("Execute", serverDoArgs, "UNDOEXECUTE", serverUndoArgs);
		*/
    } else if (systemInfo.productType === "osx") {
		// todo - make server run at startup
		// In case the directory doesn't already exist
		component.addElevatedOperation("Execute", "{0,1}", ["mkdir", "@HomeDir@/Library/LaunchAgents"]);

		component.addElevatedOperation("Copy", "@TargetDir@/Pinhole.Server.plist_autostart", "@HomeDir@/Library/LaunchAgents/Pinhole.Server.plist");

		// Replace __EXEPATH__ in file with path to PinholeServer executable
		component.addElevatedOperation("Execute", ["sed", "-i.bak", "s-__EXEPATH__-@TargetDir@/PinholeServer.app/Contents/MacOS/PinholeServer-g", "@HomeDir@/Library/LaunchAgents/Pinhole.Server.plist"]);
		// OsX doesn't support -i but does support -i.bak which creates a .bak file so now delete the .bak file
		component.addElevatedOperation("Execute", ["rm", "@HomeDir@/Library/LaunchAgents/Pinhole.Server.plist.bak"]);
		
    } else {
		/*
		// Set PinholeServer to launch as user via autostart shortcut
		// In case the directory doesn't already exist
		installer.execute("mkdir", ["@HomeDir@/.config/autostart"]);
        component.addOperation("CreateDesktopEntry", "@HomeDir@/.config/autostart/pinhole-server.desktop", "Name=Pinhole Server\nComment=Pinhole Background Server\nPath=@TargetDir@\nExec=@TargetDir@/launch.sh ./PinholeServer\nTerminal=false\nType=Application");
		*/
		
		// Check for presence of systemd
		result = installer.execute("whereis", ["systemctl"]);
		if (result[0].length < 15)
		{
			// Set PinholeServer to launch via Upstart
			component.addElevatedOperation("Copy", "@TargetDir@/pinhole.conf", "/etc/init/pinhole.conf");
			component.addElevatedOperation("Execute", ["initctl", "reload-configuration"]);
			component.addElevatedOperation("Execute", ["service", "pinhole", "start"], "UNDOEXECUTE", ["service", "pinhole", "stop"]);
		}
		else
		{
			// Set PinholeServer to launch via systemd
			component.addElevatedOperation("Copy", "@TargetDir@/pinhole.service", "/etc/systemd/system/pinhole.service");
			component.addElevatedOperation("Execute", ["systemctl", "enable", "pinhole"], "UNDOEXECUTE", ["systemctl", "disable", "pinhole"]);
			component.addElevatedOperation("Execute", ["systemctl", "start", "pinhole"], "UNDOEXECUTE", ["systemctl", "stop", "pinhole"]);
		}
    }
}

launchPrograms = function()
{
	console.log("Starting Pinhole Server...");
	
    if (systemInfo.productType === "windows") {
		//installer.executeDetached("@TargetDir@\\RunDetached.exe", ["PinholeServer.exe"], "@TargetDir@");
    } else if (systemInfo.productType === "osx") {
		//installer.executeDetached("@TargetDir@/PinholeServer.app/Contents/MacOS/PinholeServer", undefined, "@TargetDir@");
		installer.execute("launchctl", ["load", "@HomeDir@/Library/LaunchAgents/Pinhole.Server.plist"]);
    } else {
		//installer.executeDetached("@TargetDir@/launch.sh", ["./PinholeServer"], "@TargetDir@");
		//installer.execute("systemctl", ["start", "pinhole"]);
		// Service is started by component because apparently only the component can execute things elevated?
	}
}

