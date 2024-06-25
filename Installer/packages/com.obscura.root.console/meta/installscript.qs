function Component()
{
    //installer.setValue("AllUsers", true);
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {
        component.addElevatedOperation("CreateShortcut", "@TargetDir@/PinholeConsole.exe", 
            "@StartMenuDir@/Pinhole Console.lnk", "workingDirectory=@TargetDir@", 
            "description=Pinhole Console");
    } else if (systemInfo.productType === "osx") {
        component.addOperation("CreateDesktopEntry", "pinhole-console.desktop", "Name=Pinhole Console\nComment=GUI for configuring Pinhole Server\nPath=@TargetDir@\nExec=@TargetDir@/PinholeConsole.app\nTerminal=false\nType=Application");
    } else {
        component.addOperation("CreateDesktopEntry", "pinhole-console.desktop", "Name=Pinhole Console\nComment=GUI for configuring Pinhole Server\nPath=@TargetDir@\nExec=@TargetDir@/launch.sh ./PinholeConsole\nTerminal=false\nType=Application");
    }
}

