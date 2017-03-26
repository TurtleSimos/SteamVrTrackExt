# SteamVrTrackExt

Basic SteamVR tracking component with late update support for Unreal.

The project is a modified version of the standard Unreal VR template, where the MotionControllerPawn has ben modified:
1. The right motion controller is no longer spawned.
2. BeginPlay outputs a list of controller Ids to the log. SteamVR controller Id's include the HMD and the basestations. Controllers are usually id 3 and 4.
3. TestTrackRight has been added - this is the SteamVrTrack component added by the plugin. It has a child cube you can see. 

See the SteamVrTrackExt.txt under the plugin folder for more details on the plugin.

Note: Currently this has been built and tested using Visual Studio 2015 and Unreal 4.15 / 4.15.1 only.

To build:

Make sure you have Visual Studio 2015 installed. Visual Studio 2017 may work, but has not been tested.
Right click on SteamVrTrackDev.uproject and select "generate project files"
Open the Visual Studio project, make sure the build is set to "Development Editor" then build.

To add the plugin to your own projects, make sure the project has C++ support. If it was blueprint only, then add a dummy class.
Copy the plugin into the plugins directory on your project.
Regenerate the Visual Studio project files and build.