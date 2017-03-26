# SteamVrTrackExt

Basic SteamVR tracking component with late update support for Unreal.

The project is a modified version of the standard Unreal VR template, where the MotionControllerPawn has ben modified:
1. The right motion controller is no longer spawned.
2. BeginPlay outputs a list of controller Ids to the log. SteamVR controller Id's include the HMD and the basestations. Controllers are usually id 3 and 4.
3. TestTrackRight has been added - this is the SteamVrTrack component added by the plugin. It has a child cube you can see. 

See the SteamVrTrackExt.txt under the plugin folder for more details on the plugin.


